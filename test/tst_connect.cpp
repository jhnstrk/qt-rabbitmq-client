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

bool waitForFuture(const QFuture<void> &fut, int waitTimeMs = smallWaitMs)
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

class RmqConnectTest : public QObject
{
    Q_OBJECT

    // private:
private slots:
    void initTestCase()
    {
        // qDebug("Called before everything else.");
        qRegisterMetaType<qmq::Decimal>();
    }

    void testConnect()
    {
        qmq::Client client;
        QSignalSpy spy(&client, &qmq::Client::connected);
        client.connectToHost(QUrl(testUrl));
        QVERIFY(spy.wait(smallWaitMs));

        auto channel = client.createChannel();

        {
            QFuture<void> channelFut = channel->openChannel();
            // Do not use QFuture::waitForFinished() because it has no event loop.
            QTest::qWaitFor([&]() -> bool { return channelFut.isFinished(); }, smallWaitMs);

            QVERIFY(channelFut.isFinished());
            QVERIFY(!channelFut.isCanceled());
        }
        {
            QFuture<void> channelFut = channel->closeChannel(200, // constants::ReplySuccess,
                                                             "OK",
                                                             0,
                                                             0);
            QTest::qWaitFor([&]() -> bool { return channelFut.isFinished(); }, smallWaitMs);

            QVERIFY(channelFut.isFinished());
            QVERIFY(!channelFut.isCanceled());
        }

        client.disconnectFromHost();
    }
    void testDeleteExchange()
    {
        qmq::Client theClient;
        QSignalSpy connectSpy(&theClient, &qmq::Client::connected);
        theClient.connectToHost(testUrl);
        QVERIFY(connectSpy.wait(smallWaitMs));
        auto pubChannel = theClient.createChannel();

        QVERIFY(waitForFuture(pubChannel->openChannel()));
        const QString exchangeName = "my-messages";
        const QString queueName = "my-queue";
        QVERIFY(waitForFuture(pubChannel->purgeQueue(queueName)));
        QVERIFY(waitForFuture(pubChannel->deleteExchange(exchangeName)));
        QVERIFY(waitForFuture(pubChannel->closeChannel()));
    }

    void testPubSubTwoClients()
    {
        qmq::Client pubClient;
        QSignalSpy pubSpy(&pubClient, &qmq::Client::connected);
        pubClient.connectToHost(testUrl);
        QVERIFY(pubSpy.wait(smallWaitMs));
        auto pubChannel = pubClient.createChannel();

        QVERIFY(waitForFuture(pubChannel->openChannel()));

        const QString exchangeName = "my-messages";
        const QString queueName = "my-queue";
        QVERIFY(waitForFuture(
            pubChannel->declareExchange(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(pubChannel->declareQueue(queueName)));

        qmq::Client subClient;
        QSignalSpy subSpy(&subClient, &qmq::Client::connected);
        subClient.connectToHost(testUrl);
        QVERIFY(subSpy.wait(smallWaitMs));
        auto subChannel = subClient.createChannel();

        QVERIFY(waitForFuture(subChannel->openChannel()));
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

        QVERIFY(waitForFuture(pubChannel->publish(exchangeName, msg)));

        QVERIFY(subMessageSpy.wait(5000));
        qmq::Message deliveredMsg = c.dequeueMessage();
        QCOMPARE(deliveredMsg.payload(), msg.payload());
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(), ctTextPlain);
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(), ceUtf8);
        QVERIFY(subChannel->sendAck(deliveredMsg.deliveryTag()));

        QVERIFY(waitForFuture(pubChannel->closeChannel(200, "OK", 0, 0)));
        QVERIFY(waitForFuture(subChannel->closeChannel(200, "OK", 0, 0)));

        pubClient.disconnectFromHost();
        subClient.disconnectFromHost();
    }

    void testPubSubOneClientTwoChannels()
    {
        qmq::Client theClient;
        QSignalSpy pubSpy(&theClient, &qmq::Client::connected);
        theClient.connectToHost(testUrl);
        QVERIFY(pubSpy.wait(smallWaitMs));
        auto pubChannel = theClient.createChannel();

        QVERIFY(waitForFuture(pubChannel->openChannel()));

        const QString exchangeName = "my-messages";
        const QString queueName = "my-queue";
        QVERIFY(waitForFuture(
            pubChannel->declareExchange(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(pubChannel->declareQueue(queueName)));

        auto subChannel = theClient.createChannel();

        QVERIFY(waitForFuture(subChannel->openChannel()));
        QVERIFY(waitForFuture(subChannel->bindQueue(queueName, exchangeName)));
        //Add consumer
        qmq::Consumer c("test-consumer1");
        QVERIFY(waitForFuture(c.consume(subChannel.get(), queueName)));
        QSignalSpy subMessageSpy(&c, &qmq::Consumer::messageReady);

        qmq::Message msg;
        msg.setProperty(qmq::BasicProperty::ContentType, ctTextPlain);
        msg.setProperty(qmq::BasicProperty::ContentEncoding, ceUtf8);
        msg.setPayload(testMessage("testPubSubOneClientTwoChannels"));

        QVERIFY(waitForFuture(pubChannel->publish(exchangeName, msg)));

        QVERIFY(subMessageSpy.wait(5000));
        qmq::Message deliveredMsg = c.dequeueMessage();
        QCOMPARE(deliveredMsg.payload(), msg.payload());
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(), ctTextPlain);
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(), ceUtf8);
        QVERIFY(subChannel->sendAck(deliveredMsg.deliveryTag()));

        QVERIFY(waitForFuture(pubChannel->closeChannel(200, "OK", 0, 0)));
        QVERIFY(waitForFuture(subChannel->closeChannel(200, "OK", 0, 0)));

        theClient.disconnectFromHost();
    }

    void testPubSubOneClientOneChannel()
    {
        qmq::Client theClient;
        QSignalSpy connectSpy(&theClient, &qmq::Client::connected);
        theClient.connectToHost(testUrl);
        QVERIFY(connectSpy.wait(smallWaitMs));
        auto theChannel = theClient.createChannel();

        QVERIFY(waitForFuture(theChannel->openChannel()));

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

        QVERIFY(waitForFuture(theChannel->publish(exchangeName, msg)));

        QVERIFY(subMessageSpy.wait(5000));
        qmq::Message deliveredMsg = c.dequeueMessage();
        QCOMPARE(deliveredMsg.payload(), msg.payload());
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(), ctTextPlain);
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(), ceUtf8);
        QVERIFY(theChannel->sendAck(deliveredMsg.deliveryTag()));

        QVERIFY(waitForFuture(theChannel->closeChannel(200, "OK", 0, 0)));

        theClient.disconnectFromHost();
    }

    void testPubSubLargeMessage()
    {
        qmq::Client theClient;
        QSignalSpy pubSpy(&theClient, &qmq::Client::connected);
        theClient.connectToHost(testUrl);
        QVERIFY(pubSpy.wait(smallWaitMs));
        auto pubChannel = theClient.createChannel();

        QVERIFY(waitForFuture(pubChannel->openChannel()));

        const QString exchangeName = "my-messages";
        const QString queueName = "my-queue";
        QVERIFY(waitForFuture(
            pubChannel->declareExchange(exchangeName, qmq::Channel::ExchangeType::Direct)));
        QVERIFY(waitForFuture(pubChannel->declareQueue(queueName)));

        auto subChannel = theClient.createChannel();

        QVERIFY(waitForFuture(subChannel->openChannel()));
        QVERIFY(waitForFuture(subChannel->bindQueue(queueName, exchangeName)));
        //Add consumer
        qmq::Consumer c;
        QVERIFY(waitForFuture(c.consume(subChannel.get(), queueName)));
        QSignalSpy subMessageSpy(&c, &qmq::Consumer::messageReady);

        qmq::Message msg;
        msg.setProperty(qmq::BasicProperty::ContentType, "application/octet-stream");
        msg.setProperty(qmq::BasicProperty::ContentEncoding, ceBinary);
        const QByteArray payload = randomBytes(1024);
        msg.setPayload(payload);

        QVERIFY(waitForFuture(pubChannel->publish(exchangeName, msg)));

        QVERIFY(subMessageSpy.wait(5000));
        qmq::Message deliveredMsg = c.dequeueMessage();
        QCOMPARE(deliveredMsg.payload(), msg.payload());
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentType).toString(),
                 "application/octet-stream");
        QCOMPARE(deliveredMsg.property(qmq::BasicProperty::ContentEncoding).toString(), ceBinary);

        QVERIFY(waitForFuture(pubChannel->closeChannel(200, "OK", 0, 0)));
        QVERIFY(waitForFuture(subChannel->closeChannel(200, "OK", 0, 0)));

        theClient.disconnectFromHost();
    }

    void cleanupTestCase()
    {
        //qDebug("Called after myFirstTest and mySecondTest.");
    }
};

QTEST_MAIN(RmqConnectTest)

#include <tst_connect.moc>
