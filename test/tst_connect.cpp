#include <qtrabbitmq/client.h>
#include <qtrabbitmq/decimal.h>

#include <QDebug>
#include <QHash>
#include <QObject>
#include <QtTest>

namespace {
const int smallWaitMs = 1000;
}
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
        client.connectToHost(QUrl("amqp://rabbit:rabbit@localhost:5672/"));
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

    void cleanupTestCase()
    {
        //qDebug("Called after myFirstTest and mySecondTest.");
    }
};

QTEST_MAIN(RmqConnectTest)

#include <tst_connect.moc>
