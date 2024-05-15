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
        QVERIFY(waitForFuture(pubChannel->queuePurge(queueName)));
        QVERIFY(waitForFuture(pubChannel->exchangeDelete(exchangeName)));
        QVERIFY(waitForFuture(pubChannel->channelClose()));
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
            pubChannel->exchangeDeclare(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(pubChannel->queueDeclare(queueName)));

        qmq::Client subClient;
        QSignalSpy subSpy(&subClient, &qmq::Client::connected);
        QSignalSpy subDisconnectSpy(&subClient, &qmq::Client::disconnected);
        subClient.connectToHost(testUrl);
        QVERIFY(subSpy.wait(smallWaitMs));
        auto subChannel = subClient.createChannel();

        QVERIFY(waitForFuture(subChannel->channelOpen()));
        QVERIFY(waitForFuture(
            subChannel->exchangeDeclare(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(subChannel->queueDeclare(queueName)));
        QVERIFY(waitForFuture(subChannel->queueBind(queueName, exchangeName)));
        //Add consumer
        qmq::Consumer c;
        QVERIFY(waitForFuture(c.consume(subChannel.get(), queueName)));
        QSignalSpy subMessageSpy(&c, &qmq::Consumer::messageReady);

        qmq::Message msg;
        msg.setProperty(qmq::BasicProperty::ContentType, ctTextPlain);
        msg.setProperty(qmq::BasicProperty::ContentEncoding, ceUtf8);
        msg.setExchangeName(exchangeName);
        msg.setPayload(testMessage());

        QVERIFY(pubChannel->basicPublish(msg));

        QVERIFY(subMessageSpy.wait(5000));
        qmq::Message deliveredMsg = c.dequeueMessage();
        QCOMPARE(deliveredMsg.payload(), msg.payload());
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(), ctTextPlain);
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(), ceUtf8);
        QVERIFY(subChannel->basicAck(deliveredMsg.deliveryTag()));

        QVERIFY(waitForFuture(pubChannel->channelClose(200, "OK", 0, 0)));
        QVERIFY(waitForFuture(subChannel->channelClose(200, "OK", 0, 0)));

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
            pubChannel->exchangeDeclare(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(pubChannel->queueDeclare(queueName)));

        auto subChannel = client.createChannel();

        QVERIFY(waitForFuture(subChannel->channelOpen()));
        QVERIFY(waitForFuture(subChannel->queueBind(queueName, exchangeName)));
        //Add consumer
        qmq::Consumer c("test-consumer1");
        QVERIFY(waitForFuture(c.consume(subChannel.get(), queueName)));
        QSignalSpy subMessageSpy(&c, &qmq::Consumer::messageReady);

        qmq::Message msg;
        msg.setProperty(qmq::BasicProperty::ContentType, ctTextPlain);
        msg.setProperty(qmq::BasicProperty::ContentEncoding, ceUtf8);
        msg.setExchangeName(exchangeName);
        msg.setPayload(testMessage("testPubSubOneClientTwoChannels"));

        QVERIFY(pubChannel->basicPublish(msg));

        QVERIFY(subMessageSpy.wait(5000));
        qmq::Message deliveredMsg = c.dequeueMessage();
        QCOMPARE(deliveredMsg.payload(), msg.payload());
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(), ctTextPlain);
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(), ceUtf8);
        QVERIFY(subChannel->basicAck(deliveredMsg.deliveryTag()));

        QVERIFY(waitForFuture(pubChannel->channelClose(200, "OK", 0, 0)));
        QVERIFY(waitForFuture(subChannel->channelClose(200, "OK", 0, 0)));

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
            theChannel->exchangeDeclare(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(theChannel->queueDeclare(queueName)));
        QVERIFY(waitForFuture(theChannel->queueBind(queueName, exchangeName)));
        //Add consumer
        qmq::Consumer consumer;
        QVERIFY(waitForFuture(consumer.consume(theChannel.get(), queueName)));
        QSignalSpy subMessageSpy(&consumer, &qmq::Consumer::messageReady);

        qmq::Message msg;
        msg.setProperty(qmq::BasicProperty::ContentType, ctTextPlain);
        msg.setProperty(qmq::BasicProperty::ContentEncoding, ceUtf8);
        msg.setExchangeName(exchangeName);
        msg.setPayload(testMessage("PubSubOneClientOneChannel"));

        QVERIFY(theChannel->basicPublish(msg));

        QVERIFY(subMessageSpy.wait(5000));
        qmq::Message deliveredMsg = consumer.dequeueMessage();
        QCOMPARE(deliveredMsg.payload(), msg.payload());
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(), ctTextPlain);
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(), ceUtf8);
        QVERIFY(theChannel->basicAck(deliveredMsg.deliveryTag()));

        QVERIFY(waitForFuture(theChannel->channelClose(200, "OK", 0, 0)));

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
            pubChannel->exchangeDeclare(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(pubChannel->queueDeclare(queueName)));

        auto subChannel = client.createChannel();

        QVERIFY(waitForFuture(subChannel->channelOpen()));
        QVERIFY(waitForFuture(subChannel->queueBind(queueName, exchangeName)));
        //Add consumer
        qmq::Consumer consumer;
        QVERIFY(waitForFuture(consumer.consume(subChannel.get(), queueName)));
        QSignalSpy subMessageSpy(&consumer, &qmq::Consumer::messageReady);

        {
            // 1k message
            qmq::Message msg;
            msg.setProperty(qmq::BasicProperty::ContentType, "application/octet-stream");
            msg.setProperty(qmq::BasicProperty::ContentEncoding, ceBinary);
            msg.setExchangeName(exchangeName);
            const QByteArray payload = randomBytes(1024);
            msg.setPayload(payload);

            QVERIFY(pubChannel->basicPublish(msg));

            QVERIFY(subMessageSpy.wait(5000));
            qmq::Message deliveredMsg = consumer.dequeueMessage();
            QCOMPARE(deliveredMsg.payload(), msg.payload());
            QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(),
                     "application/octet-stream");
            QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(),
                     ceBinary);
            QVERIFY(subChannel->basicAck(deliveredMsg.deliveryTag()));
        }

        {
            // 1M message
            qmq::Message msg;
            msg.setProperty(qmq::BasicProperty::ContentType, "application/octet-stream");
            msg.setProperty(qmq::BasicProperty::ContentEncoding, ceBinary);
            msg.setExchangeName(exchangeName);
            const QByteArray payload = randomBytes(1024 * 1024);
            msg.setPayload(payload);

            QVERIFY(pubChannel->basicPublish(msg));

            QVERIFY(subMessageSpy.wait(5000));
            qmq::Message deliveredMsg = consumer.dequeueMessage();
            QCOMPARE(deliveredMsg.payload(), msg.payload());
            QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(),
                     "application/octet-stream");
            QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(),
                     ceBinary);
            QVERIFY(subChannel->basicAck(deliveredMsg.deliveryTag()));
        }
        QVERIFY(waitForFuture(pubChannel->channelClose(200, "OK", 0, 0)));
        QVERIFY(waitForFuture(subChannel->channelClose(200, "OK", 0, 0)));

        client.disconnectFromHost();
        QVERIFY(disconnectSpy.wait(smallWaitMs));
    }

    // Using the "Get" basic API.
    void testPubGetTwoClients()
    {
        qmq::Client pubClient;
        QSignalSpy pubConnectSpy(&pubClient, &qmq::Client::connected);
        QSignalSpy pubDisconnectSpy(&pubClient, &qmq::Client::disconnected);
        pubClient.connectToHost(testUrl);
        QVERIFY(pubConnectSpy.wait(smallWaitMs));
        auto pubChannel = pubClient.createChannel();

        QVERIFY(waitForFuture(pubChannel->channelOpen()));

        const QString exchangeName = "my-messages";
        const QString queueName = "my-queue";
        QVERIFY(waitForFuture(
            pubChannel->exchangeDeclare(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(pubChannel->queueDeclare(queueName)));

        qmq::Client subClient;
        QSignalSpy subConnectSpy(&subClient, &qmq::Client::connected);
        QSignalSpy subDisconnectSpy(&subClient, &qmq::Client::disconnected);
        subClient.connectToHost(testUrl);
        QVERIFY(subConnectSpy.wait(smallWaitMs));
        auto subChannel = subClient.createChannel();

        QVERIFY(waitForFuture(subChannel->channelOpen()));
        QVERIFY(waitForFuture(
            subChannel->exchangeDeclare(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(subChannel->queueDeclare(queueName)));
        QVERIFY(waitForFuture(subChannel->queueBind(queueName, exchangeName)));

        // At this point nothing is published. Get should return empty.
        QFuture<QVariantList> result = subChannel->basicGet(queueName);
        QVERIFY(waitForFuture(result));
        QCOMPARE(result.resultCount(), 0);

        const QString payload = testMessage();
        QVERIFY(pubChannel->basicPublish(payload, exchangeName));

        QVERIFY(QTest::qWaitFor(
            [&]() -> bool {
                result = subChannel->basicGet(queueName);
                return waitForFuture(result) && (result.resultCount() > 0);
            },
            smallWaitMs));

        QCOMPARE(result.resultCount(), 1);

        const QVariantList resultList = result.resultAt(0);
        QCOMPARE(resultList.size(), 2);
        QCOMPARE(resultList.at(1).toInt(), 0);

        const qmq::Message deliveredMsg = resultList.at(0).value<qmq::Message>();

        QCOMPARE(deliveredMsg.payload(), payload);
        QVERIFY(subChannel->basicAck(deliveredMsg.deliveryTag()));

        QVERIFY(waitForFuture(pubChannel->channelClose(200, "OK", 0, 0)));
        QVERIFY(waitForFuture(subChannel->channelClose(200, "OK", 0, 0)));

        qDebug() << "pubClient . disconnectFromHost..";
        pubClient.disconnectFromHost();
        qDebug() << "subClient . disconnectFromHost..";
        QVERIFY(pubDisconnectSpy.wait(smallWaitMs));
        subClient.disconnectFromHost();
        qDebug() << "waiting";
        QVERIFY(subDisconnectSpy.wait(smallWaitMs));
    }

    void cleanupTestCase()
    {
        //qDebug() << "Called after every test";
    }
};

QTEST_MAIN(RmqPubSubTest)

#include <tst_pubsub.moc>
