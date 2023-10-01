#include <QBuffer>
#include <QDebug>
#include <QObject>
#include <QtTest>

#include <frame.h>

class FrameIoTest : public QObject
{
    Q_OBJECT

private:
private slots:
    void initTestCase()
    {
        // qDebug("Called before everything else.");
    }

    void singleString()
    {
        const QVariant tstValue = QString("a_short_string");
        QBuffer buffer;

        // Buffer is not open. Expect writing to fail.
        QCOMPARE(qmq::detail::Frame::writeFieldValue(&buffer, tstValue), false);

        QVERIFY(buffer.open(QBuffer::ReadWrite));

        QVERIFY(qmq::detail::Frame::writeFieldValue(&buffer, tstValue));
        bool ok;
        buffer.seek(0);
        const QVariant actual = qmq::detail::Frame::readFieldValue(&buffer, &ok);
        QCOMPARE(actual, tstValue);
    }

    void testValues_data()
    {
        QTest::addColumn<QVariant>("value");

        const QVariantList basiclist({QVariant(1), QVariant(2), QVariant(3)});
        const QVariantHash basichash({{QString("One"), QVariant(1)},
                                      {QString("Two"), QVariant(2)},
                                      {QString("Three"), QVariant(3)}});
        QTest::newRow("astring") << QVariant(QString::fromLatin1("HELLO"));
        QTest::newRow("bytearray") << QVariant(QByteArray("HELLO"));
        QTest::newRow("double") << QVariant(double(1234567890123.0));
        QTest::newRow("float") << QVariant(float(123456.0f));
        QTest::newRow("int") << QVariant(int(123456));
        QTest::newRow("short") << QVariant::fromValue(short(-1234));
        QTest::newRow("int64") << QVariant::fromValue(qint64(1234567890123LL));
        QTest::newRow("uchar") << QVariant::fromValue(quint8(129));
        QTest::newRow("fieldarray") << QVariant(basiclist);
        QTest::newRow("fieldtable") << QVariant(basichash);
        QTest::newRow("2k_bytearray") << QVariant(QByteArray(2048, 'h'));
    }

    void testValues()
    {
        const QFETCH(QVariant, value);
        QBuffer buffer;
        QVERIFY(buffer.open(QBuffer::ReadWrite));

        QVERIFY(qmq::detail::Frame::writeFieldValue(&buffer, value));
        bool ok;
        buffer.seek(0);
        const QVariant actual = qmq::detail::Frame::readFieldValue(&buffer, &ok);
        QCOMPARE(actual.typeId(), value.typeId());
        QCOMPARE(actual, value);
    }

    void cleanupTestCase()
    {
        // qDebug("Called after myFirstTest and mySecondTest.");
    }
};

QTEST_MAIN(FrameIoTest)

#include <tst_frame_io.moc>
