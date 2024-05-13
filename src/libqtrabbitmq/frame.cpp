#include "spec_constants.h"
#include <qtrabbitmq/frame.h>

#include <qtrabbitmq/decimal.h>

#include <QBuffer>
#include <QDateTime>
#include <QIODevice>
#include <QVariant>
#include <QtEndian>

namespace {
bool verifyTypeCompat(const qmq::FieldValue type, const QMetaType &metatype)
{
    switch (type) {
    case qmq::FieldValue::Boolean:
        return metatype.id() == QMetaType::Bool;
    case qmq::FieldValue::ShortShortInt:
        return metatype.id() == QMetaType::SChar;
    case qmq::FieldValue::ShortShortUint:
        return metatype.id() == QMetaType::UChar;
    case qmq::FieldValue::ShortInt:
        return metatype.id() == QMetaType::Short || metatype.id() == QMetaType::Int;
    case qmq::FieldValue::ShortUint:
        return metatype.id() == QMetaType::UShort || metatype.id() == QMetaType::Int;
    case qmq::FieldValue::LongInt:
        return metatype.id() == QMetaType::Int;
    case qmq::FieldValue::LongUint:
        return metatype.id() == QMetaType::UInt;
    case qmq::FieldValue::LongLongInt:
        return metatype.id() == QMetaType::LongLong;
    case qmq::FieldValue::LongLongUint:
        return metatype.id() == QMetaType::ULongLong;
    case qmq::FieldValue::Float:
        return metatype.id() == QMetaType::Float;
    case qmq::FieldValue::Double:
        return metatype.id() == QMetaType::Double;
    case qmq::FieldValue::DecimalValue:
        return metatype.id() == QMetaType::fromType<qmq::Decimal>().id();
    case qmq::FieldValue::ShortString:
        return metatype.id() == QMetaType::QString;
    case qmq::FieldValue::LongString:
        return metatype.id() == QMetaType::QByteArray;
    case qmq::FieldValue::FieldArray:
        return metatype.id() == QMetaType::QVariantList;
    case qmq::FieldValue::Timestamp:
        return metatype.id() == QMetaType::QDateTime;
    case qmq::FieldValue::FieldTable:
        return metatype.id() == QMetaType::QVariantHash;
    case qmq::FieldValue::Void:
        return metatype.id() == QMetaType::Void;
    case qmq::FieldValue::Bit:
        return metatype.id() == QMetaType::Bool;
    case qmq::FieldValue::Invalid:
        return metatype.id() == 0;
    default:
        return false;
    }
    // Unreachable
    return false;
}
template<typename T>
T readAmqp(QIODevice *io, bool *ok)
{
    const int N = sizeof(T);
    char buffer[N];
    if (io->read(buffer, N) != N) {
        qCritical() << "Error reading value";
        if (ok != nullptr)
            *ok = false;
        return T();
    };
    if (ok != nullptr) {
        *ok = true;
    }
    return qFromBigEndian<T>(static_cast<char *>(buffer));
}

template<typename T>
QVariant readAmqpVariant(QIODevice *io, bool *ok)
{
    bool isOk = false;
    const T value = readAmqp<T>(io, &isOk);
    if (isOk) {
        if (ok != nullptr) {
            *ok = true;
        }
        return QVariant::fromValue<T>(value);
    }
    if (ok != nullptr) {
        *ok = false;
    }
    return QVariant();
}

template<typename T>
bool writeAmqp(QIODevice *io, T value)
{
    const int N = sizeof(T);
    char buffer[N];
    qToBigEndian<T>(value, static_cast<char *>(buffer));
    if (io->write(static_cast<char *>(buffer), N) != N) {
        qCritical() << "Error writing value";
        return false;
    };
    return true;
}

template<typename T>
bool writeAmqpVariant(QIODevice *io, const QVariant &value)
{
    if (!value.canConvert<T>()) {
        qCritical() << "Cannot convert value to T";
        return false;
    }
    return writeAmqp<T>(io, value.value<T>());
}

bool readAmqpBool(QIODevice *io, bool *ok)
{
    char buffer[1];
    if (io->read(buffer, 1) != 1) {
        qCritical() << "Error reading value";
        if (ok != nullptr)
            *ok = false;
        return false;
    };
    if (ok != nullptr) {
        *ok = true;
    }
    return buffer[0] != 0;
}

bool writeAmqpBool(QIODevice *io, bool value)
{
    return writeAmqp<quint8>(io, value ? quint8(1) : quint8(0));
}

QVariant readAmqpVariantBool(QIODevice *io, bool *ok)
{
    bool isOk;
    const bool v = readAmqpBool(io, &isOk);
    if (ok != nullptr) {
        *ok = isOk;
    }
    if (isOk) {
        return QVariant::fromValue(v);
    }
    return QVariant();
}

qmq::Decimal readAmqpDecimal(QIODevice *io, bool *ok)
{
    char buffer[5];
    if (io->read(buffer, 5) != 5) {
        qCritical() << "Error reading value";
        if (ok != nullptr) {
            *ok = false;
        }
        return qmq::Decimal();
    };
    const quint8 scale = static_cast<quint8>(buffer[0]);
    const qint32 value = qFromBigEndian<qint32>(buffer + 1);
    if (ok != nullptr) {
        *ok = true;
    }
    return qmq::Decimal(scale, value);
}

bool writeAmqpDecimal(QIODevice *io, const qmq::Decimal &value)
{
    return writeAmqp<quint8>(io, quint8(value.scale)) && writeAmqp<qint32>(io, qint32(value.value));
}

QVariant readAmqpVariantDecimal(QIODevice *io, bool *ok)
{
    bool isOk;
    const qmq::Decimal v = readAmqpDecimal(io, &isOk);
    if (ok != nullptr) {
        *ok = isOk;
    }
    if (isOk) {
        return QVariant::fromValue<qmq::Decimal>(v);
    }
    return QVariant();
}

bool writeAmqpVariantDecimal(QIODevice *io, const QVariant &value)
{
    if (!value.canConvert<qmq::Decimal>()) {
        qWarning() << "Cannot convert to decimal";
        return false;
    }
    return writeAmqpDecimal(io, value.value<qmq::Decimal>());
}

QByteArray readAmqpShortString(QIODevice *io, bool *ok)
{
    bool isOk;
    const quint8 len = readAmqp<quint8>(io, &isOk);
    if (!isOk) {
        qCritical() << "Error reading length";
        if (ok != nullptr) {
            *ok = false;
        }
        return QByteArray();
    };
    QByteArray buffer;
    buffer.resize(len);
    if (io->read(buffer.data(), buffer.size()) != len) {
        qCritical() << "Error reading value";
        if (ok != nullptr) {
            *ok = false;
        }
        return QByteArray();
    }

    if (ok != nullptr) {
        *ok = true;
    }
    return buffer;
}

bool writeAmqpShortString(QIODevice *io, const QByteArray &value)
{
    if (value.size() > 0xFF) {
        qWarning() << "string too long";
        return false;
    }
    const quint8 len = value.size();
    bool isOk = writeAmqp<quint8>(io, len);
    if (!isOk) {
        return false;
    }
    if (io->write(value) != len) {
        qWarning() << "Error writing value";
        return false;
    }
    return true;
}

bool writeAmqpShortString(QIODevice *io, const QString &value)
{
    return writeAmqpShortString(io, value.toUtf8());
}

QVariant readAmqpVariantShortString(QIODevice *io, bool *ok)
{
    bool isOk;
    const QByteArray v = readAmqpShortString(io, &isOk);
    if (ok != nullptr) {
        *ok = isOk;
    }
    if (isOk) {
        return QVariant::fromValue(QString::fromUtf8(v));
    }
    return QVariant();
}

bool writeAmqpVariantShortString(QIODevice *io, const QVariant &value)
{
    if (value.typeId() == QMetaType::Type::QString) {
        return writeAmqpShortString(io, value.value<QString>());
    }
    if (value.typeId() == QMetaType::Type::QByteArray || value.canConvert<QByteArray>()) {
        return writeAmqpShortString(io, value.value<QByteArray>());
    }
    qWarning() << "Cannot convert to short string";
    return false;
}

QByteArray readAmqpLongString(QIODevice *io, bool *ok)
{
    char lenBuf[4];
    if (io->read(static_cast<char *>(lenBuf), 4) != 4) {
        qCritical() << "Error reading length";
        if (ok != nullptr) {
            *ok = false;
        }
        return QByteArray();
    };
    const quint32 len = qFromBigEndian<quint32>(static_cast<char *>(lenBuf));
    QByteArray buffer;
    buffer.resize(len);
    if (io->read(buffer.data(), buffer.size()) != len) {
        qCritical() << "Error reading value";
        if (ok != nullptr)
            *ok = false;
        return QByteArray();
    }

    if (ok != nullptr) {
        *ok = true;
    }
    return buffer;
}

bool writeAmqpLongString(QIODevice *io, const QByteArray &value)
{
    const quint32 len = value.size();
    if (value.size() != len) {
        qWarning() << "string too long";
        return false;
    }
    const bool isOk = writeAmqp<quint32>(io, len);
    if (!isOk) {
        qWarning() << "Error writing value";
        return false;
    }
    if (io->write(value) != len) {
        qWarning() << "Error writing value";
        return false;
    }
    return true;
}

QVariant readAmqpVariantLongString(QIODevice *io, bool *ok)
{
    bool isOk;
    const QByteArray v = readAmqpLongString(io, &isOk);
    if (ok != nullptr) {
        *ok = isOk;
    }
    if (isOk) {
        return QVariant::fromValue(v);
    }
    return QVariant();
}

bool writeAmqpVariantLongString(QIODevice *io, const QVariant &value)
{
    if (value.typeId() == QMetaType::Type::QByteArray) {
        return writeAmqpLongString(io, value.value<QByteArray>());
    }
    if (value.typeId() == QMetaType::Type::QString) {
        return writeAmqpLongString(io, value.value<QString>().toUtf8());
    }
    if (value.canConvert<QByteArray>()) {
        return writeAmqpLongString(io, value.value<QByteArray>());
    }
    qWarning() << "Cannot convert to long string";
    return false;
}

QVariantList readAmqpVariantFieldArray(QIODevice *io, bool *ok)
{
    bool isOk = true;
    const quint32 len = readAmqp<quint32>(io, &isOk);
    if (!isOk) {
        if (ok != nullptr) {
            *ok = false;
        }
        return QVariantList();
    }
    QByteArray packedData = io->read(len);
    if (packedData.size() != len) {
        if (ok != nullptr)
            *ok = false;
        qWarning() << "Attempt to allocate failed" << len;
        return QVariantList();
    }
    QBuffer packedIo(&packedData);
    if (!packedIo.open(QIODevice::ReadOnly)) {
        qWarning() << "Attempt to open buffer failed";
        return QVariantList();
    }
    QVariantList items;
    while (!packedIo.atEnd()) {
        const QVariant nextItem = qmq::Frame::readFieldValue(&packedIo, &isOk);
        if (!isOk) {
            if (ok != nullptr) {
                *ok = false;
            }
            return items;
        }
        items.append(nextItem);
    }
    return items;
}

bool writeAmqpFieldArray(QIODevice *io, const QVariantList &value)
{
    QByteArray packedBuffer;
    {
        QBuffer packedIo(&packedBuffer);
        if (!packedIo.open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to open";
            return false;
        }

        for (const QVariant &item : value) {
            const bool isOk = qmq::Frame::writeFieldValue(&packedIo, item);
            if (!isOk) {
                return false;
            }
        }
    }
    const quint32 len = packedBuffer.size();
    const bool isOk = writeAmqp<quint32>(io, len);
    if (!isOk)
        return false;
    if (!io->write(packedBuffer)) {
        return false;
    }

    return true;
}

bool writeAmqpVariantFieldArray(QIODevice *io, const QVariant &value)
{
    if (!value.canConvert<QVariantList>()) {
        qWarning() << "Cannot convert to QVariantList";
        return false;
    }
    return writeAmqpFieldArray(io, value.toList());
}

bool writeAmqpTimestamp(QIODevice *io, const QDateTime &value)
{
    const qint64 secsSinceEpoch = value.toSecsSinceEpoch();
    return writeAmqp<qint64>(io, secsSinceEpoch);
}

bool writeAmqpVariantTimestamp(QIODevice *io, const QVariant &value)
{
    if (!value.canConvert<QDateTime>()) {
        qWarning() << "Cannot convert to QDateTime";
        return false;
    }
    return writeAmqpTimestamp(io, value.toDateTime());
}

QVariantHash readAmqpVariantFieldTable(QIODevice *io, bool *ok)
{
    bool isOk = true;
    const quint32 len = readAmqp<quint32>(io, &isOk);
    if (!isOk) {
        if (ok != nullptr) {
            *ok = false;
        }
        return QVariantHash();
    }
    QByteArray packedData = io->read(len);
    if (packedData.size() != len) {
        if (ok != nullptr) {
            *ok = false;
        }
        qWarning() << "Attempt to allocate failed" << len;
        return QVariantHash();
    }
    QBuffer packedIo(&packedData);
    if (!packedIo.open(QIODevice::ReadOnly)) {
        qWarning() << "Attempt to open buffer failed";
        return QVariantHash();
    }
    QVariantHash items;
    while (!packedIo.atEnd()) {
        const QByteArray name = readAmqpShortString(&packedIo, &isOk);
        if (!isOk) {
            if (ok != nullptr) {
                *ok = false;
            }
            return items;
        }
        const QVariant nextItem = qmq::Frame::readFieldValue(&packedIo, &isOk);
        if (!isOk) {
            if (ok != nullptr) {
                *ok = false;
            }
            return items;
        }
        items[QString::fromUtf8(name)] = nextItem;
    }
    return items;
}

bool writeAmqpFieldTable(QIODevice *io, const QVariantHash &value)
{
    QByteArray packedBuffer;
    {
        QBuffer packedIo(&packedBuffer);
        if (!packedIo.open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to open";
            return false;
        }

        for (auto it = value.constKeyValueBegin(); it != value.constKeyValueEnd(); ++it) {
            bool isOk = writeAmqpShortString(&packedIo, it->first);
            if (!isOk) {
                return false;
            }
            isOk = qmq::Frame::writeFieldValue(&packedIo, it->second);
            if (!isOk) {
                return false;
            }
        }
    }
    const quint32 len = packedBuffer.size();
    const bool isOk = writeAmqp<quint32>(io, len);
    if (!isOk)
        return false;

    if (io->write(packedBuffer) != packedBuffer.size()) {
        return false;
    }
    return true;
}

bool writeAmqpVariantFieldTable(QIODevice *io, const QVariant &value)
{
    if (!value.canConvert<QVariantHash>()) {
        qWarning() << "Cannot convert to QVariantHash";
        return false;
    }
    return writeAmqpFieldTable(io, value.toHash());
}

} // namespace

qmq::FieldValue qmq::Frame::metatypeToFieldValue(int typeId)
{
    switch (typeId) {
    case QMetaType::Type::Bool:
        return qmq::FieldValue::Boolean;
    case QMetaType::Type::Char:
        return qmq::FieldValue::ShortShortInt;
    case QMetaType::Type::UChar:
        return qmq::FieldValue::ShortShortUint;
    case QMetaType::Type::Short:
        return qmq::FieldValue::ShortInt;
    case QMetaType::Type::UShort:
        return qmq::FieldValue::ShortUint;
    case QMetaType::Type::Int:
        return qmq::FieldValue::LongInt;
    case QMetaType::Type::UInt:
        return qmq::FieldValue::LongUint;
    case QMetaType::Type::LongLong:
        return qmq::FieldValue::LongLongInt;
    case QMetaType::Type::ULongLong:
        return qmq::FieldValue::LongLongUint;
    case QMetaType::Type::Float:
        return qmq::FieldValue::Float;
    case QMetaType::Type::Double:
        return qmq::FieldValue::Double;
    // Decimal is handled later because the value isn't a compile-time constant
    case QMetaType::Type::QString:
        return qmq::FieldValue::ShortString;
    case QMetaType::Type::QByteArray:
        return qmq::FieldValue::LongString;
    case QMetaType::Type::QVariantList:
        return qmq::FieldValue::FieldArray;
    case QMetaType::Type::QDateTime:
        return qmq::FieldValue::Timestamp;
    case QMetaType::Type::QVariantHash:
        return qmq::FieldValue::FieldTable;
    case QMetaType::Type::Void:
        return qmq::FieldValue::Void;
    default:
        break;
    }
    if (typeId == qMetaTypeId<qmq::Decimal>()) {
        return qmq::FieldValue::DecimalValue;
    }
    return qmq::FieldValue::Invalid;
}

QMetaType::Type qmq::Frame::fieldValueToMetatype(qmq::FieldValue fieldtype)
{
    switch (fieldtype) {
    case qmq::FieldValue::Bit:
        Q_FALLTHROUGH();
    case qmq::FieldValue::Boolean:
        return QMetaType::Type::Bool;
    case qmq::FieldValue::ShortShortInt:
        return QMetaType::Type::Char;
    case qmq::FieldValue::ShortShortUint:
        return QMetaType::Type::UChar;
    case qmq::FieldValue::ShortInt:
        return QMetaType::Type::Short;
    case qmq::FieldValue::ShortUint:
        return QMetaType::Type::UShort;
    case qmq::FieldValue::LongInt:
        return QMetaType::Type::Int;
    case qmq::FieldValue::LongUint:
        return QMetaType::Type::UInt;
    case qmq::FieldValue::LongLongInt:
        return QMetaType::Type::LongLong;
    case qmq::FieldValue::LongLongUint:
        return QMetaType::Type::ULongLong;
    case qmq::FieldValue::Float:
        return QMetaType::Type::Float;
    case qmq::FieldValue::Double:
        return QMetaType::Type::Double;
    case qmq::FieldValue::DecimalValue:
        return static_cast<QMetaType::Type>(qMetaTypeId<qmq::Decimal>());
    case qmq::FieldValue::ShortString:
        return QMetaType::Type::QString;
    case qmq::FieldValue::LongString:
        return QMetaType::Type::QByteArray;
    case qmq::FieldValue::FieldArray:
        return QMetaType::Type::QVariantList;
    case qmq::FieldValue::Timestamp:
        return QMetaType::Type::QDateTime;
    case qmq::FieldValue::FieldTable:
        return QMetaType::Type::QVariantHash;
    case qmq::FieldValue::Void:
        return QMetaType::Type::Void;
    case qmq::FieldValue::Invalid:
        return QMetaType::Type::UnknownType;
    default:
        qCritical() << "Bad Fieldtype" << (int) fieldtype;
        return QMetaType::Type::UnknownType;
    }
}

QVariant qmq::Frame::readFieldValue(QIODevice *io, bool *ok)
{
    bool isOk;
    const FieldValue type = static_cast<FieldValue>(readAmqp<quint8>(io, &isOk));
    if (!isOk) {
        if (ok != nullptr) {
            *ok = false;
        }
        return QVariant();
    }
    return readNativeFieldValue(io, type, ok);
}

QVariant qmq::Frame::readNativeFieldValue(QIODevice *io, FieldValue type, bool *ok)
{
    switch (type) {
    case FieldValue::Boolean:
        return readAmqpVariantBool(io, ok);
    case FieldValue::ShortShortInt:
        return readAmqpVariant<qint8>(io, ok);
    case FieldValue::ShortShortUint:
        return readAmqpVariant<quint8>(io, ok);
    case FieldValue::ShortInt:
        return readAmqpVariant<qint16>(io, ok);
    case FieldValue::ShortUint:
        return readAmqpVariant<quint16>(io, ok);
    case FieldValue::LongInt:
        return readAmqpVariant<qint32>(io, ok);
    case FieldValue::LongUint:
        return readAmqpVariant<quint32>(io, ok);
    case FieldValue::LongLongInt:
        return readAmqpVariant<qint64>(io, ok);
    case FieldValue::LongLongUint:
        return readAmqpVariant<quint64>(io, ok);
    case FieldValue::Float:
        return readAmqpVariant<float>(io, ok);
    case FieldValue::Double:
        return readAmqpVariant<double>(io, ok);
    case FieldValue::DecimalValue:
        return readAmqpVariantDecimal(io, ok);
    case FieldValue::ShortString:
        return readAmqpVariantShortString(io, ok);
    case FieldValue::LongString:
        return readAmqpVariantLongString(io, ok);
    case FieldValue::FieldArray:
        return readAmqpVariantFieldArray(io, ok);
    case FieldValue::Timestamp: {
        bool isOk = false;
        const qint64 v = readAmqp<qint64>(io, &isOk);
        if (ok != nullptr) {
            *ok = isOk;
        }
        if (isOk) {
            return QVariant(QDateTime::fromSecsSinceEpoch(v));
        }
        return QVariant();
    }
    case FieldValue::FieldTable:
        return readAmqpVariantFieldTable(io, ok);
    case FieldValue::Void:
        if (ok != nullptr) {
            *ok = true;
        }
        return QVariant(QMetaType(QMetaType::Type::Void));
        break;
    default:
        qWarning() << "Unknown field type" << (int) type;

        return QVariant();
    }
}

QVariantList qmq::Frame::readNativeFieldValues(QIODevice *io,
                                               const QList<FieldValue> &types,
                                               bool *ok)
{
    bool isOk = true;
    int bitPos = 0;
    QVariantList ret;
    ret.reserve(types.size());
    for (qsizetype i = 0; i < types.size(); ++i) {
        const FieldValue &type = types.at(i);
        if (type == FieldValue::Bit) {
            ++bitPos;
        }
        if ((type != FieldValue::Bit) || (i == types.size() - 1)) {
            while (bitPos > 0) {
                const quint8 byte = readAmqp<quint8>(io, &isOk);
                const int numBitsRead = std::min(bitPos, 8);
                for (int ibit = 0; ibit < numBitsRead; ++ibit) {
                    const bool isSet = (((1 << ibit) & byte) != 0);
                    ret.push_back(QVariant::fromValue(isSet));
                }
                bitPos -= numBitsRead;
            }
        }
        if (type != FieldValue::Bit) {
            ret.push_back(readNativeFieldValue(io, type, &isOk));
        }
    }
    if (ok != nullptr) {
        *ok = isOk;
    }
    return ret;
}

bool qmq::Frame::writeFieldValue(QIODevice *io, const QVariant &value)
{
    const FieldValue valueType = metatypeToFieldValue(value.typeId());
    return writeFieldValue(io, value, valueType);
}

bool qmq::Frame::writeFieldValue(QIODevice *io, const QVariant &value, FieldValue valueType)
{
    const bool isOk = writeAmqp<quint8>(io, static_cast<quint8>(valueType));
    if (!isOk) {
        return false;
    }

    return writeNativeFieldValue(io, value, valueType);
}

bool qmq::Frame::writeNativeFieldValue(QIODevice *io, const QVariant &value, FieldValue valueType)
{
    switch (valueType) {
    case FieldValue::Boolean:
        return writeAmqpBool(io, value.toBool());
    case FieldValue::ShortShortInt:
        return writeAmqpVariant<qint8>(io, value);
    case FieldValue::ShortShortUint:
        return writeAmqpVariant<quint8>(io, value);
    case FieldValue::ShortInt:
        return writeAmqpVariant<qint16>(io, value);
    case FieldValue::ShortUint:
        return writeAmqpVariant<quint16>(io, value);
    case FieldValue::LongInt:
        return writeAmqpVariant<qint32>(io, value);
    case FieldValue::LongUint:
        return writeAmqpVariant<quint32>(io, value);
    case FieldValue::LongLongInt:
        return writeAmqpVariant<qint64>(io, value);
    case FieldValue::LongLongUint:
        return writeAmqpVariant<quint64>(io, value);
    case FieldValue::Float:
        return writeAmqpVariant<float>(io, value);
    case FieldValue::Double:
        return writeAmqpVariant<double>(io, value);
    case FieldValue::DecimalValue:
        return writeAmqpVariantDecimal(io, value);
    case FieldValue::ShortString:
        return writeAmqpVariantShortString(io, value);
    case FieldValue::LongString:
        return writeAmqpVariantLongString(io, value);
    case FieldValue::FieldArray:
        return writeAmqpVariantFieldArray(io, value);
    case FieldValue::Timestamp:
        return writeAmqpVariantTimestamp(io, value);
    case FieldValue::FieldTable:
        return writeAmqpVariantFieldTable(io, value);
    case FieldValue::Void:
        return true;
        break;
    default:
        qWarning() << "Unknown field type" << (int) valueType;
        return false;
    }
}

bool qmq::Frame::writeNativeFieldValues(QIODevice *io,
                                        const QVariantList &values,
                                        const QList<FieldValue> &types)
{
    bool isOk = true;
    int bitPos = 0;
    quint8 bitBuffer = 0;
    for (qsizetype i = 0; i < types.size(); ++i) {
        const FieldValue &type = types.at(i);
        const QVariant &value = values.at(i);
#ifndef NDEBUG
        if (!verifyTypeCompat(type, value.metaType())) {
            qWarning() << "Incompatible type" << (char) type << value.metaType().name() << i;
        }
#endif
        if (type == FieldValue::Bit) {
            if (!value.canConvert<bool>()) {
                qWarning() << "Failed conversion to bool";
                return false;
            }
            const bool isSet = value.value<bool>();
            if (isSet) {
                bitBuffer |= (1 << bitPos);
            }
            ++bitPos;
        }
        if ((bitPos == 8)
            || (bitPos > 0 && ((type != FieldValue::Bit) || (i == types.size() - 1)))) {
            isOk = writeAmqp<quint8>(io, bitBuffer);
            if (!isOk) {
                return false;
            }
            bitBuffer = 0;
            bitPos = 0;
        }
        if (type != FieldValue::Bit) {
            isOk = writeNativeFieldValue(io, value, type);
        }
        if (!isOk) {
            return false;
        }
    }
    return isOk;
}

std::unique_ptr<qmq::Frame> qmq::Frame::readFrame(QIODevice *io,
                                                  quint32 maxFrameSize,
                                                  ErrorCode *err)
{
    if (io->bytesAvailable() < (FrameHeaderSize + 1)) {
        *err = ErrorCode::InsufficientDataAvailable;
        qDebug() << "Cannot read frame; waiting for more data, available bytes:"
                 << io->bytesAvailable();
        return std::unique_ptr<qmq::Frame>();
    }

    const int headerLen = FrameHeaderSize;
    char header[headerLen];
    if (io->peek(header, headerLen) != headerLen) {
        qWarning() << "peek failed";
        return std::unique_ptr<qmq::Frame>();
    }
    const FrameType t = static_cast<FrameType>(header[0]);
    const quint16 channel = qFromBigEndian<quint16>(static_cast<const char *>(header) + 1);
    const quint32 size = qFromBigEndian<quint32>(static_cast<const char *>(header) + 3);

    if (maxFrameSize != 0 && size > maxFrameSize) {
        *err = ErrorCode::FrameTooLarge;
        qWarning() << "Frame too large" << size;
        return std::unique_ptr<qmq::Frame>();
    }
    if (io->bytesAvailable() < (size + FrameHeaderSize + 1)) {
        qDebug() << "InsufficientDataAvailable" << io->bytesAvailable() << size << FrameHeaderSize;
        *err = ErrorCode::InsufficientDataAvailable;
        return std::unique_ptr<qmq::Frame>();
    }

    io->skip(FrameHeaderSize);

    const QByteArray content = io->read((size));
    if (content.size() != (size)) {
        *err = ErrorCode::IoError;
        qWarning() << "Read finished before frame completed. read:" << content.size()
                   << "expected:" << size;
        return std::unique_ptr<qmq::Frame>();
    }

    bool isOk = false;
    const quint8 endByte = readAmqp<quint8>(io, &isOk);
    if (!isOk || endByte != FrameEndChar) {
        *err = ErrorCode::InvalidFrameData;
        qWarning() << "Frame end byte invalid or could not be read" << (int) endByte << (isOk);
        return std::unique_ptr<qmq::Frame>();
    }
    qDebug() << ":Frame::readFrame Construct frame from data. channel:" << channel
             << "content size:" << size;
    switch (t) {
    case qmq::FrameType::Method:
        return std::unique_ptr<qmq::Frame>(MethodFrame::fromContent(channel, content).release());
    case qmq::FrameType::Header:
        return std::unique_ptr<qmq::Frame>(HeaderFrame::fromContent(channel, content).release());
    case qmq::FrameType::Body:
        return std::unique_ptr<qmq::Frame>(BodyFrame::fromContent(channel, content).release());
    case qmq::FrameType::Heartbeat:
        return std::unique_ptr<qmq::Frame>(HeartbeatFrame::fromContent(channel, content).release());
    default:
        *err = ErrorCode::UnknownFrameType;
        qWarning() << "Unknown frame type";
        return std::unique_ptr<qmq::Frame>();
    }
}

bool qmq::Frame::writeFrame(QIODevice *io, quint32 maxFrameSize, const Frame &f)
{
    qDebug() << "Writing frame to channel" << f.channel() << "with type=" << (int) f.type();
    const QByteArray content = f.content();

    const quint8 t = static_cast<quint8>(f.type());
    const quint16 channel = f.channel();
    const quint32 size = content.size();

    if (maxFrameSize != 0 && (content.size() + FrameHeaderSize + 1) > maxFrameSize) {
        qWarning() << "Cannot write frame: too large.";
        return false;
    }

    // Create a temporary store so that the frame is written in a single TCP write operation.
    QByteArray iobuffer;
    iobuffer.reserve(size + 8);
    QBuffer buf(&iobuffer);
    buf.open(QIODevice::WriteOnly);

    bool isOk = writeAmqp<quint8>(&buf, t);
    isOk = isOk && writeAmqp<quint16>(&buf, channel);
    isOk = isOk && writeAmqp<quint32>(&buf, size);
    isOk = isOk && (buf.write(content) == content.size());
    isOk = isOk && writeAmqp<quint8>(&buf, FrameEndChar);

    // Actually send the packet.
    isOk = isOk && (io->write(iobuffer) == iobuffer.size());
    qDebug() << "Written frame" << (isOk ? "OK" : "FAILED") << "with size" << size;
    return isOk;
}

std::unique_ptr<qmq::BodyFrame> qmq::BodyFrame::fromContent(quint16 channel,
                                                            const QByteArray &content)
{
    return std::unique_ptr<qmq::BodyFrame>(new BodyFrame(channel, content));
}
std::unique_ptr<qmq::MethodFrame> qmq::MethodFrame::fromContent(quint16 channel,
                                                                const QByteArray &content)
{
    QBuffer io;
    io.setData(content);
    bool isOk = io.open(QIODevice::ReadOnly);
    const quint16 classId = readAmqp<quint16>(&io, &isOk);
    const quint16 methodId = readAmqp<quint16>(&io, &isOk);
    const QByteArray arguments = content.mid(4);
    return std::unique_ptr<qmq::MethodFrame>(new MethodFrame(channel, classId, methodId, arguments));
}

QByteArray qmq::MethodFrame::content() const
{
    qDebug() << "Get method content";
    QBuffer io;
    bool isOk = io.open(QIODevice::WriteOnly);
    isOk = isOk && writeAmqp<quint16>(&io, this->classId());
    isOk = isOk && writeAmqp<quint16>(&io, this->methodId());
    isOk = isOk && (io.write(this->m_arguments) == this->m_arguments.size());
    if (!isOk) {
        qWarning() << "Failed writing frame content";
    }
    io.close();
    return io.data();
}

QVariantList qmq::MethodFrame::getArguments(bool *ok) const
{
    const QList<FieldValue> types = spec::methodArgs(this->classId(), this->methodId());
    QBuffer io;
    io.setData(this->m_arguments);
    bool isOk = io.open(QIODevice::ReadOnly);
    if (!isOk) {
        if (ok != nullptr)
            *ok = false;
        qWarning() << "Unable to open stream for reading";
        return QVariantList();
    }
    return Frame::readNativeFieldValues(&io, types, ok);
}

bool qmq::MethodFrame::setArguments(const QVariantList &values)
{
    const QList<FieldValue> types = spec::methodArgs(this->classId(), this->methodId());
    if (values.size() != types.size()) {
        qCritical() << "Size of values does not match types";
        return false;
    }
    QBuffer io;
    bool isOk = io.open(QIODevice::WriteOnly);
    if (!isOk) {
        return false;
    }
    isOk = Frame::writeNativeFieldValues(&io, values, types);
    io.close();
    this->m_arguments = io.buffer();
    return isOk;
}

qmq::HeaderFrame::HeaderFrame(quint16 channel,
                              quint16 classId,
                              quint64 contentSize,
                              const QHash<qmq::BasicProperty, QVariant> &properties)
    : Frame(qmq::FrameType::Header, channel)
    , m_classId(classId)
    , m_contentSize(contentSize)
    , m_properties(properties)
{}

std::unique_ptr<qmq::HeaderFrame> qmq::HeaderFrame::fromContent(quint16 channel,
                                                                const QByteArray &content)
{
    QBuffer io;
    io.setData(content);
    bool isOk = io.open(QIODevice::ReadOnly);
    const quint16 classId = readAmqp<quint16>(&io, &isOk);
    /* const quint16 weight = */ readAmqp<quint16>(&io, &isOk);
    const quint64 contentSize = readAmqp<quint64>(&io, &isOk);
    const quint16 propertyFlags = readAmqp<quint16>(&io, &isOk);
    if (!isOk) {
        qWarning() << "Buffer not opened";
        return std::unique_ptr<qmq::HeaderFrame>();
    }

    QHash<BasicProperty, QVariant> properties;
    for (unsigned int i = 0; i < std::size(spec::basicPropertyTypes); ++i) {
        if ((propertyFlags & ((1 << 15) >> i)) != 0) {
            const FieldValue propType = spec::basicPropertyTypes[i];
            const QVariant value = Frame::readNativeFieldValue(&io, propType, &isOk);
            const BasicProperty propId = static_cast<BasicProperty>(i);
            properties.insert(propId, value);
        }
    }

    return std::unique_ptr<qmq::HeaderFrame>(
        new HeaderFrame(channel, classId, contentSize, properties));
}

QByteArray qmq::HeaderFrame::content() const
{
    QList<BasicProperty> proplist(m_properties.keys());
    std::sort(proplist.begin(), proplist.end());

    quint16 propertyFlags = 0;
    QVariantList orderedValues;
    QList<FieldValue> orderedTypes;
    orderedValues.reserve(m_properties.size());
    orderedTypes.reserve(m_properties.size());

    for (BasicProperty it : proplist) {
        const int itIndex = (int) it;
        propertyFlags |= ((1 << 15) >> itIndex); // High bit comes first.
        orderedValues.push_back(m_properties.value(it));
        orderedTypes.push_back(spec::basicPropertyTypes[itIndex]);
    }
    QBuffer io;
    bool isOk = io.open(QIODevice::WriteOnly);
    if (!isOk) {
        qWarning() << "Buffer not opened";
        return QByteArray();
    }
    do {
        isOk = writeAmqp<quint16>(&io, this->classId());
        if (!isOk)
            break;
        isOk = writeAmqp<quint16>(&io, 0); // weight.
        if (!isOk)
            break;
        isOk = writeAmqp<quint64>(&io, this->contentSize());
        if (!isOk)
            break;
        isOk = writeAmqp<quint16>(&io, propertyFlags);
        if (!isOk)
            break;
        isOk = Frame::writeNativeFieldValues(&io, orderedValues, orderedTypes);
    } while (false);
    io.close();
    if (!isOk) {
        qWarning() << "Error writing field values";
        return QByteArray();
    }
    return io.buffer();
}

std::unique_ptr<qmq::HeartbeatFrame> qmq::HeartbeatFrame::fromContent(quint16 channel,
                                                                      const QByteArray &content)
{
    if (channel != 0) {
        qWarning() << "Hearbeat frame non-zero channel";
    }
    if (!content.isEmpty()) {
        qWarning() << "Hearbeat frame has unexpected content, which has been discarded";
    }
    return std::unique_ptr<qmq::HeartbeatFrame>(new HeartbeatFrame());
}
