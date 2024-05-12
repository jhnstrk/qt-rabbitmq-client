#include <qtrabbitmq/client.h>
#include <qtrabbitmq/decimal.h>

#include <QDebug>
#include <QFutureWatcher>
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
    QFutureWatcher<T> watcher;
    QSignalSpy spy(&watcher, &QFutureWatcher<T>::finished);
    watcher.setFuture(fut);
    return spy.wait(waitTimeMs);
}

} // namespace

class RmqHeartbeatTest : public QObject
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
        // Connect, using a 3-second heartbeat, and wait for 10 seconds.
        // If the client doesn't send heartbeats, Rabbit will close the connection
        // since this is more than the 2-heartbeat tolerance.
        qmq::Client client;
        client.setHeartbeatSeconds(3);

        QSignalSpy spy(&client, &qmq::Client::connected);
        QSignalSpy disconnectSpy(&client, &qmq::Client::disconnected);
        client.connectToHost(QUrl(testUrl));
        QVERIFY(spy.wait(smallWaitMs));
        QTest::qWait(10000);

        auto channel = client.createChannel();
        QVERIFY(waitForFuture(channel->channelOpen()));
        QVERIFY(waitForFuture(channel->channelClose(200, "OK", 0, 0)));
        client.disconnectFromHost();
        QVERIFY(disconnectSpy.wait(smallWaitMs));
    }

    void cleanupTestCase()
    {
        //qDebug() << "Called after every test";
    }
};

QTEST_MAIN(RmqHeartbeatTest)

#include <tst_heartbeats.moc>
