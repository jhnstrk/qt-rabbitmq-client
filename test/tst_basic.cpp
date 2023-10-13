#include <qtrabbitmq/qtrabbitmq.h>

#include <QDebug>
#include <QHash>
#include <QObject>
#include <QtTest>

class BasicRmqTest : public QObject
{
    Q_OBJECT

private:
private slots:
    void initTestCase()
    {
        // qDebug("Called before everything else.");
        qRegisterMetaType<qmq::Decimal>();
    }

    void testDecimalVariant()
    {
        QCOMPARE(qmq::Decimal(0, 0), qmq::Decimal(0, 0));
        QCOMPARE(QVariant::fromValue(qmq::Decimal(0, 0)), QVariant::fromValue(qmq::Decimal(0, 0)));
        qDebug() << "Decimal" << qmq::Decimal(5, 1234512345);
        qDebug() << "Decimal Variant" << QVariant::fromValue(qmq::Decimal(5, 1234512345));
    }

    void testDecimalHash()
    {
        QHash<qmq::Decimal, int> test;
        test[qmq::Decimal(0, 0)] = 100;
        test[qmq::Decimal(10, 0)] = 10100;
        test[qmq::Decimal(10, 10)] = 10110;
        QCOMPARE(test.value(qmq::Decimal(0, 0)), 100);
        QCOMPARE(test.value(qmq::Decimal(10, 0)), 10100);
        QCOMPARE(test.value(qmq::Decimal(10, 10)), 10110);
    }

    void testDecimal_data()
    {
        QTest::addColumn<qmq::Decimal>("value");
        QTest::addColumn<QString>("strValue");

        QTest::newRow("0") << qmq::Decimal(0, 0) << "0";
        QTest::newRow("1") << qmq::Decimal(0, 1) << "1";
        QTest::newRow("-1") << qmq::Decimal(0, -1) << "-1";
        QTest::newRow("0.1") << qmq::Decimal(1, 1) << "0.1";
        QTest::newRow("-0.1") << qmq::Decimal(1, -1) << "-0.1";
        QTest::newRow("0.00000000000080") << qmq::Decimal(14, 80) << "8.0e-13";
        QTest::newRow("-0.00000000000080") << qmq::Decimal(14, -80) << "-8.0e-13";
        QTest::newRow("655.36") << qmq::Decimal(2, 65536) << "655.36";
        QTest::newRow("-655.36") << qmq::Decimal(2, -65536) << "-655.36";
        QTest::newRow("6.5536e-200") << qmq::Decimal(204, 65536) << "6.5536e-200";
        QTest::newRow("-6.5536e-200") << qmq::Decimal(204, -65536) << "-6.5536e-200";
    }
    void testDecimal()
    {
        const QFETCH(qmq::Decimal, value);
        const QFETCH(QString, strValue);

        QCOMPARE(value.toString(), strValue);
    }
    void cleanupTestCase()
    {
        //qDebug("Called after myFirstTest and mySecondTest.");
    }
};

QTEST_MAIN(BasicRmqTest)

#include <tst_basic.moc>
