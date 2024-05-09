#include <qtrabbitmq/client.h>
#include <qtrabbitmq/decimal.h>

#include <QDebug>
#include <QHash>
#include <QObject>
#include <QRandomGenerator>
#include <QtTest>

namespace {
const int smallWaitMs = 5000;
const QUrl testUrl("amqp://rabbituser:rabbitpass@localhost:5672/");
const QString ctTextPlain("text/plain");
const QString ceUtf8("utf-8");
const QString ceBinary("binary");

template<class T>
bool waitForFuture(const QFuture<T> &fut, int waitTimeMs = smallWaitMs)
{
    return QTest::qWaitFor([&]() -> bool { return fut.isFinished(); }, waitTimeMs);
}

QString testMessage(const QString start = "Message")
{
    return QString("%1 %2").arg(start, QString::number(QRandomGenerator::system()->generate()));
}

QByteArray randomBytes(qsizetype size, quint32 seed = 0)
{
    QRandomGenerator psRand(seed);
    QByteArray res;
    res.reserve(size);
    for (qsizetype i = 0; i < size; ++i) {
        res.append(psRand());
    }
    return res;
}
} // namespace

class RmqPubSubTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        // qDebug("Called before everything else.");
        qRegisterMetaType<qmq::Decimal>();
    }

    void testDeleteExchange()
    {
        qmq::Client client;
        QSignalSpy connectSpy(&client, &qmq::Client::connected);
        QSignalSpy disconnectSpy(&client, &qmq::Client::disconnected);
        client.connectToHost(testUrl);
        QVERIFY(connectSpy.wait(smallWaitMs));
        auto pubChannel = client.createChannel();

        QVERIFY(waitForFuture(pubChannel->channelOpen()));
        const QString exchangeName = "my-messages";
        const QString queueName = "my-queue";
        QVERIFY(waitForFuture(pubChannel->purgeQueue(queueName)));
        QVERIFY(waitForFuture(pubChannel->deleteExchange(exchangeName)));
        QVERIFY(waitForFuture(pubChannel->closeChannel()));
        client.disconnectFromHost();
        QVERIFY(disconnectSpy.wait(smallWaitMs));
    }

    void testPubSubTwoClients()
    {
        qmq::Client pubClient;
        QSignalSpy pubSpy(&pubClient, &qmq::Client::connected);
        QSignalSpy pubDisconnectSpy(&pubClient, &qmq::Client::disconnected);
        pubClient.connectToHost(testUrl);
        QVERIFY(pubSpy.wait(smallWaitMs));
        auto pubChannel = pubClient.createChannel();

        QVERIFY(waitForFuture(pubChannel->channelOpen()));

        const QString exchangeName = "my-messages";
        const QString queueName = "my-queue";
        QVERIFY(waitForFuture(
            pubChannel->declareExchange(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(pubChannel->declareQueue(queueName)));

        qmq::Client subClient;
        QSignalSpy subSpy(&subClient, &qmq::Client::connected);
        QSignalSpy subDisconnectSpy(&subClient, &qmq::Client::disconnected);
        subClient.connectToHost(testUrl);
        QVERIFY(subSpy.wait(smallWaitMs));
        auto subChannel = subClient.createChannel();

        QVERIFY(waitForFuture(subChannel->channelOpen()));
        QVERIFY(waitForFuture(
            subChannel->declareExchange(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(subChannel->declareQueue(queueName)));
        QVERIFY(waitForFuture(subChannel->bindQueue(queueName, exchangeName)));
        //Add consumer
        qmq::Consumer c;
        QVERIFY(waitForFuture(c.consume(subChannel.get(), queueName)));
        QSignalSpy subMessageSpy(&c, &qmq::Consumer::messageReady);

        qmq::Message msg;
        msg.setProperty(qmq::BasicProperty::ContentType, ctTextPlain);
        msg.setProperty(qmq::BasicProperty::ContentEncoding, ceUtf8);
        msg.setPayload(testMessage());

        QVERIFY(pubChannel->publish(exchangeName, msg));

        QVERIFY(subMessageSpy.wait(5000));
        qmq::Message deliveredMsg = c.dequeueMessage();
        QCOMPARE(deliveredMsg.payload(), msg.payload());
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(), ctTextPlain);
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(), ceUtf8);
        QVERIFY(subChannel->ack(deliveredMsg.deliveryTag()));

        QVERIFY(waitForFuture(pubChannel->closeChannel(200, "OK", 0, 0)));
        QVERIFY(waitForFuture(subChannel->closeChannel(200, "OK", 0, 0)));

        qDebug() << "pubClient . disconnectFromHost..";
        pubClient.disconnectFromHost();
        qDebug() << "subClient . disconnectFromHost..";
        QVERIFY(pubDisconnectSpy.wait(smallWaitMs));
        subClient.disconnectFromHost();
        qDebug() << "waiting";
        QVERIFY(subDisconnectSpy.wait(smallWaitMs));
    }

    void testPubSubOneClientTwoChannels()
    {
        qmq::Client client;
        QSignalSpy connectSpy(&client, &qmq::Client::connected);
        QSignalSpy disconnectSpy(&client, &qmq::Client::disconnected);
        client.connectToHost(testUrl);
        QVERIFY(connectSpy.wait(smallWaitMs));
        auto pubChannel = client.createChannel();

        QVERIFY(waitForFuture(pubChannel->channelOpen()));

        const QString exchangeName = "my-messages";
        const QString queueName = "my-queue";
        QVERIFY(waitForFuture(
            pubChannel->declareExchange(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(pubChannel->declareQueue(queueName)));

        auto subChannel = client.createChannel();

        QVERIFY(waitForFuture(subChannel->channelOpen()));
        QVERIFY(waitForFuture(subChannel->bindQueue(queueName, exchangeName)));
        //Add consumer
        qmq::Consumer c("test-consumer1");
        QVERIFY(waitForFuture(c.consume(subChannel.get(), queueName)));
        QSignalSpy subMessageSpy(&c, &qmq::Consumer::messageReady);

        qmq::Message msg;
        msg.setProperty(qmq::BasicProperty::ContentType, ctTextPlain);
        msg.setProperty(qmq::BasicProperty::ContentEncoding, ceUtf8);
        msg.setPayload(testMessage("testPubSubOneClientTwoChannels"));

        QVERIFY(pubChannel->publish(exchangeName, msg));

        QVERIFY(subMessageSpy.wait(5000));
        qmq::Message deliveredMsg = c.dequeueMessage();
        QCOMPARE(deliveredMsg.payload(), msg.payload());
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(), ctTextPlain);
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(), ceUtf8);
        QVERIFY(subChannel->ack(deliveredMsg.deliveryTag()));

        QVERIFY(waitForFuture(pubChannel->closeChannel(200, "OK", 0, 0)));
        QVERIFY(waitForFuture(subChannel->closeChannel(200, "OK", 0, 0)));

        client.disconnectFromHost();
        QVERIFY(disconnectSpy.wait(smallWaitMs));
    }

    void testPubSubOneClientOneChannel()
    {
        qmq::Client client;
        QSignalSpy connectSpy(&client, &qmq::Client::connected);
        QSignalSpy disconnectSpy(&client, &qmq::Client::disconnected);
        client.connectToHost(testUrl);
        QVERIFY(connectSpy.wait(smallWaitMs));
        auto theChannel = client.createChannel();

        QVERIFY(waitForFuture(theChannel->channelOpen()));

        const QString exchangeName = "my-messages";
        const QString queueName = "my-queue";
        QVERIFY(waitForFuture(
            theChannel->declareExchange(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(theChannel->declareQueue(queueName)));
        QVERIFY(waitForFuture(theChannel->bindQueue(queueName, exchangeName)));
        //Add consumer
        qmq::Consumer c;
        QVERIFY(waitForFuture(c.consume(theChannel.get(), queueName)));
        QSignalSpy subMessageSpy(&c, &qmq::Consumer::messageReady);

        qmq::Message msg;
        msg.setProperty(qmq::BasicProperty::ContentType, ctTextPlain);
        msg.setProperty(qmq::BasicProperty::ContentEncoding, ceUtf8);
        msg.setPayload(testMessage("PubSubOneClientOneChannel"));

        QVERIFY(theChannel->publish(exchangeName, msg));

        QVERIFY(subMessageSpy.wait(5000));
        qmq::Message deliveredMsg = c.dequeueMessage();
        QCOMPARE(deliveredMsg.payload(), msg.payload());
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(), ctTextPlain);
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(), ceUtf8);
        QVERIFY(theChannel->ack(deliveredMsg.deliveryTag()));

        QVERIFY(waitForFuture(theChannel->closeChannel(200, "OK", 0, 0)));

        client.disconnectFromHost();
        QVERIFY(disconnectSpy.wait(smallWaitMs));
    }

    void testPubSubLargeMessage()
    {
        qmq::Client client;
        QSignalSpy connectSpy(&client, &qmq::Client::connected);
        QSignalSpy disconnectSpy(&client, &qmq::Client::disconnected);
        client.connectToHost(testUrl);
        QVERIFY(connectSpy.wait(smallWaitMs));
        auto pubChannel = client.createChannel();

        QVERIFY(waitForFuture(pubChannel->channelOpen()));

        const QString exchangeName = "my-messages";
        const QString queueName = "my-queue";
        QVERIFY(waitForFuture(
            pubChannel->declareExchange(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(pubChannel->declareQueue(queueName)));

        auto subChannel = client.createChannel();

        QVERIFY(waitForFuture(subChannel->channelOpen()));
        QVERIFY(waitForFuture(subChannel->bindQueue(queueName, exchangeName)));
        //Add consumer
        qmq::Consumer consumer;
        QVERIFY(waitForFuture(consumer.consume(subChannel.get(), queueName)));
        QSignalSpy subMessageSpy(&consumer, &qmq::Consumer::messageReady);

        {
            // 1k message
            qmq::Message msg;
            msg.setProperty(qmq::BasicProperty::ContentType, "application/octet-stream");
            msg.setProperty(qmq::BasicProperty::ContentEncoding, ceBinary);
            const QByteArray payload = randomBytes(1024);
            msg.setPayload(payload);

            QVERIFY(pubChannel->publish(exchangeName, msg));

            QVERIFY(subMessageSpy.wait(5000));
            qmq::Message deliveredMsg = consumer.dequeueMessage();
            QCOMPARE(deliveredMsg.payload(), msg.payload());
            QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(),
                     "application/octet-stream");
            QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(),
                     ceBinary);
        }

        {
            // 1M message
            qmq::Message msg;
            msg.setProperty(qmq::BasicProperty::ContentType, "application/octet-stream");
            msg.setProperty(qmq::BasicProperty::ContentEncoding, ceBinary);
            const QByteArray payload = randomBytes(1024 * 1024);
            msg.setPayload(payload);

            QVERIFY(pubChannel->publish(exchangeName, msg));

            QVERIFY(subMessageSpy.wait(5000));
            qmq::Message deliveredMsg = consumer.dequeueMessage();
            QCOMPARE(deliveredMsg.payload(), msg.payload());
            QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(),
                     "application/octet-stream");
            QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(),
                     ceBinary);
        }
        QVERIFY(waitForFuture(pubChannel->closeChannel(200, "OK", 0, 0)));
        QVERIFY(waitForFuture(subChannel->closeChannel(200, "OK", 0, 0)));

        client.disconnectFromHost();
        QVERIFY(disconnectSpy.wait(smallWaitMs));
    }

    void cleanupTestCase()
    {
        //qDebug() << "Called after every test";
    }
};

QTEST_MAIN(RmqPubSubTest)

#include <tst_pubsub.moc>
