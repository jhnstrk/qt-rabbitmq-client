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

    void testNativeValues_data() { testValues_data(); }

    void testNativeValues()
    {
        const QFETCH(QVariant, value);
        QBuffer buffer;
        QVERIFY(buffer.open(QBuffer::ReadWrite));
        const qmq::FieldValue type = qmq::detail::Frame::metatypeToFieldValue(value.typeId());
        QVERIFY(qmq::detail::Frame::writeNativeFieldValues(&buffer, {value}, {type}));
        bool ok;
        buffer.seek(0);
        const QVariantList actual = qmq::detail::Frame::readNativeFieldValues(&buffer, {type}, &ok);

        QCOMPARE(actual.size(), 1);
        QCOMPARE(actual.first().typeId(), value.typeId());
        QCOMPARE(actual.first(), value);
    }

    void testNativeValuesBits() {}

    void testNativeValuesList_data()
    {
        QTest::addColumn<QVariantList>("values");
        QTest::addColumn<QList<qmq::FieldValue>>("types");
        QTest::addColumn<qsizetype>("packedSizeInBytes");
        for (int numBits = 0; numBits < 128; ++numBits) {
            const QVariantList values(numBits, QVariant(true));
            const QList<qmq::FieldValue> types(numBits, qmq::FieldValue::Bit);
            const qsizetype packedSizeInBytes = (numBits + 7) / 8;
            QTest::newRow(QString("%1_bits").arg(numBits).toUtf8().constData())
                << values << types << packedSizeInBytes;
        }
        QTest::newRow("bits_short_bits_int")
            << QVariantList({true, false, 123, false, true, 12323232})
            << QList<qmq::FieldValue>({qmq::FieldValue::Bit,
                                       qmq::FieldValue::Bit,
                                       qmq::FieldValue::ShortInt,
                                       qmq::FieldValue::Bit,
                                       qmq::FieldValue::Bit,
                                       qmq::FieldValue::LongInt})
            << qsizetype(8);
    }

    void testNativeValuesList()
    {
        const QFETCH(QVariantList, values);
        const QFETCH(QList<qmq::FieldValue>, types);
        const QFETCH(qsizetype, packedSizeInBytes);

        QBuffer buffer;
        QVERIFY(buffer.open(QBuffer::ReadWrite));
        QVERIFY(qmq::detail::Frame::writeNativeFieldValues(&buffer, values, types));
        bool ok;
        buffer.seek(0);
        const QVariantList actual = qmq::detail::Frame::readNativeFieldValues(&buffer, types, &ok);

        QCOMPARE(actual.size(), values.size());
        QCOMPARE(actual, values);
        QCOMPARE(buffer.size(), packedSizeInBytes);
    }

    void cleanupTestCase()
    {
        // qDebug("Called after myFirstTest and mySecondTest.");
    }
};

QTEST_MAIN(FrameIoTest)

#include <tst_frame_io.moc>
