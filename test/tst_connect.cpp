#include <qtrabbitmq/qtrabbitmq.h>

#include <QDebug>
#include <QHash>
#include <QObject>
#include <QtTest>

class RmqConnectTest : public QObject
{
    Q_OBJECT

private:
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
        client.connectToHost(QUrl("amqp://rabbit:rabbit@localhost:5672/default"));
        spy.wait(5000);
    }

    void cleanupTestCase()
    {
        //qDebug("Called after myFirstTest and mySecondTest.");
    }
};

QTEST_MAIN(RmqConnectTest)

#include <tst_connect.moc>
