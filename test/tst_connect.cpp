#include <qtrabbitmq/client.h>
#include <qtrabbitmq/decimal.h>

#include <QDebug>
#include <QHash>
#include <QObject>
#include <QtTest>

namespace {
const int smallWaitMs = 5000;
const QUrl testUrl("amqp://rabbituser:rabbitpass@localhost:5672/");

bool waitForFuture(const QFuture<void> &fut, int waitTimeMs = smallWaitMs)
{
    return QTest::qWaitFor([&]() -> bool { return fut.isFinished(); }, waitTimeMs);
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
        QTest::qWait(smallWaitMs);
    }

    void testPubSub()
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
        msg.setProperty(qmq::BasicProperty::ContentType, "text.plain");
        msg.setProperty(qmq::BasicProperty::ContentEncoding, "utf-8");
        msg.setPayload("Hello World");

        QVERIFY(waitForFuture(pubChannel->publish(exchangeName, msg)));

        QVERIFY(subMessageSpy.wait(5000));
        qmq::Message deliveredMsg = c.dequeueMessage();
        QCOMPARE(deliveredMsg.payload(), msg.payload());

        QVERIFY(waitForFuture(pubChannel->closeChannel(200, "OK", 0, 0)));
        QVERIFY(waitForFuture(subChannel->closeChannel(200, "OK", 0, 0)));

        pubClient.disconnectFromHost();
        subClient.disconnectFromHost();
        QTest::qWait(smallWaitMs);
    }

    void cleanupTestCase()
    {
        //qDebug("Called after myFirstTest and mySecondTest.");
    }
};

QTEST_MAIN(RmqConnectTest)

#include <tst_connect.moc>
