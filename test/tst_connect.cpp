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

template<class T>
bool waitForFuture(const QFuture<T> &fut, int waitTimeMs = smallWaitMs)
{
    return QTest::qWaitFor([&]() -> bool { return fut.isFinished(); }, waitTimeMs);
}

} // namespace

class RmqConnectTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        // qDebug("Called before everything else.");
        qRegisterMetaType<qmq::Decimal>();
    }

    void testConnect()
    {
        qmq::Client client;
        QSignalSpy spy(&client, &qmq::Client::connected);
        QSignalSpy disconnectSpy(&client, &qmq::Client::disconnected);
        client.connectToHost(QUrl(testUrl));
        QVERIFY(spy.wait(smallWaitMs));

        auto channel = client.createChannel();

        {
            QFuture<void> channelFut = channel->channelOpen();
            // Do not use QFuture::waitForFinished() because it has no event loop.
            QVERIFY(QTest::qWaitFor([&]() -> bool { return channelFut.isFinished(); }, smallWaitMs));
            QVERIFY(!channelFut.isCanceled());
        }
        {
            QFuture<void> channelFut = channel->closeChannel(200, // constants::ReplySuccess,
                                                             "OK",
                                                             0,
                                                             0);
            QVERIFY(QTest::qWaitFor([&]() -> bool { return channelFut.isFinished(); }, smallWaitMs));
            QVERIFY(!channelFut.isCanceled());
        }

        client.disconnectFromHost();
        QVERIFY(disconnectSpy.wait(smallWaitMs));
    }

    void cleanupTestCase()
    {
        //qDebug() << "Called after every test";
    }
};

QTEST_MAIN(RmqConnectTest)

#include <tst_connect.moc>
