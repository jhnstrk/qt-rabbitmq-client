#include "frame.h"

#include <QBuffer>
#include <QDateTime>
#include <QIODevice>
#include <QVariant>
#include <QtEndian>

namespace {
template<typename T>
T readAmqp(QIODevice *io, bool *ok)
{
    const int N = sizeof(T);
    char buffer[N];
    if (io->read(buffer, N) != N) {
        qCritical() << "Error reading value";
        if (ok)
            *ok = false;
        return T();
    };
    if (ok)
        *ok = true;
    return qFromBigEndian<T>(buffer);
}

template<typename T>
QVariant readAmqpVariant(QIODevice *io, bool *ok)
{
    bool isOk = false;
    const T value = readAmqp<T>(io, &isOk);
    if (isOk) {
        if (ok)
            *ok = true;
        return QVariant::fromValue<T>(value);
    }
    if (ok)
        *ok = false;
    return QVariant();
}

template<typename T>
bool writeAmqp(QIODevice *io, T value)
{
    const int N = sizeof(T);
    char buffer[N];
    qToBigEndian<T>(value, buffer);
    if (io->write(buffer, N) != N) {
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
        if (ok)
            *ok = false;
        return false;
    };
    if (ok)
        *ok = true;
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
    if (ok)
        *ok = isOk;
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
        if (ok)
            *ok = false;
        return qmq::Decimal();
    };
    const quint8 scale = static_cast<quint8>(buffer[0]);
    const quint32 value = qFromBigEndian<quint32>(buffer + 1);
    if (ok)
        *ok = true;
    return qmq::Decimal(scale, value);
}

bool writeAmqpDecimal(QIODevice *io, const qmq::Decimal &value)
{
    return writeAmqp<quint8>(io, quint8(value.scale))
           && writeAmqp<quint32>(io, quint32(value.value));
}

QVariant readAmqpVariantDecimal(QIODevice *io, bool *ok)
{
    bool isOk;
    const qmq::Decimal v = readAmqpDecimal(io, &isOk);
    if (ok)
        *ok = isOk;
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
        if (ok)
            *ok = false;
        return QByteArray();
    };
    QByteArray buffer;
    buffer.resize(len);
    if (io->read(buffer.data(), buffer.size()) != len) {
        qCritical() << "Error reading value";
        if (ok)
            *ok = false;
        return QByteArray();
    }

    if (ok)
        *ok = true;
    return buffer;
}

bool writeAmqpShortString(QIODevice *io, const QByteArray &value)
{
    if (value.size() > 0xFF) {
        qWarning() << "string too long";
        return false;
    }
    const quint8 len = value.size();
    bool ok = writeAmqp<quint8>(io, len);
    if (!ok) {
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
    if (ok)
        *ok = isOk;
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
    if (io->read(lenBuf, 4) != 4) {
        qCritical() << "Error reading length";
        if (ok)
            *ok = false;
        return QByteArray();
    };
    const quint32 len = qFromBigEndian<quint32>(lenBuf);
    QByteArray buffer;
    buffer.resize(len);
    if (io->read(buffer.data(), buffer.size()) != len) {
        qCritical() << "Error reading value";
        if (ok)
            *ok = false;
        return QByteArray();
    }

    if (ok)
        *ok = true;
    return buffer;
}

bool writeAmqpLongString(QIODevice *io, const QByteArray &value)
{
    const quint32 len = value.size();
    if (value.size() != len) {
        qWarning() << "string too long";
        return false;
    }
    bool ok = writeAmqp<quint32>(io, len);
    if (!ok) {
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
    if (ok)
        *ok = isOk;
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
        if (ok)
            *ok = false;
        return QVariantList();
    }
    QByteArray packedData = io->read(len);
    if (packedData.size() != len) {
        if (ok)
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
        const QVariant nextItem = qmq::detail::Frame::readFieldValue(&packedIo, &isOk);
        if (!isOk) {
            if (ok)
                *ok = false;
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
            bool ok = qmq::detail::Frame::writeFieldValue(&packedIo, item);
            if (!ok)
                return false;
        }
    }
    const quint32 len = packedBuffer.size();
    bool ok = writeAmqp<quint32>(io, len);
    if (!ok)
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
        if (ok)
            *ok = false;
        return QVariantHash();
    }
    QByteArray packedData = io->read(len);
    if (packedData.size() != len) {
        if (ok)
            *ok = false;
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
            if (ok)
                *ok = false;
            return items;
        }
        const QVariant nextItem = qmq::detail::Frame::readFieldValue(&packedIo, &isOk);
        if (!isOk) {
            if (ok)
                *ok = false;
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
            bool ok = writeAmqpShortString(&packedIo, it->first);
            if (!ok)
                return false;
            ok = qmq::detail::Frame::writeFieldValue(&packedIo, it->second);
            if (!ok)
                return false;
        }
    }
    const quint32 len = packedBuffer.size();
    bool ok = writeAmqp<quint32>(io, len);
    if (!ok)
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

qmq::FieldValue qmq::detail::Frame::metatypeToFieldValue(int typeId)
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

QMetaType::Type qmq::detail::Frame::fieldValueToMetatype(qmq::FieldValue fieldtype)
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
    }
}

QVariant qmq::detail::Frame::readFieldValue(QIODevice *io, bool *ok)
{
    bool isOk;
    const FieldValue type = static_cast<FieldValue>(readAmqp<quint8>(io, &isOk));
    if (!isOk) {
        if (ok)
            *ok = false;
        return QVariant();
    }
    return readNativeFieldValue(io, type, ok);
}

QVariant qmq::detail::Frame::readNativeFieldValue(QIODevice *io, FieldValue type, bool *ok)
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
        if (ok)
            *ok = isOk;
        if (isOk) {
            return QVariant(QDateTime::fromSecsSinceEpoch(v));
        }
        return QVariant();
    }
    case FieldValue::FieldTable:
        return readAmqpVariantFieldTable(io, ok);
    case FieldValue::Void:
        if (ok)
            *ok = true;
        return QVariant(QMetaType(QMetaType::Type::Void));
        break;
    default:
        qWarning() << "Unknown field type" << (int) type;
        ;
        return QVariant();
    }
}

QVariantList qmq::detail::Frame::readNativeFieldValues(QIODevice *io,
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
    if (ok)
        *ok = isOk;
    return ret;
}

bool qmq::detail::Frame::writeFieldValue(QIODevice *io, const QVariant &value)
{
    const FieldValue valueType = metatypeToFieldValue(value.typeId());
    return writeFieldValue(io, value, valueType);
}

bool qmq::detail::Frame::writeFieldValue(QIODevice *io, const QVariant &value, FieldValue valueType)
{
    const bool ok = writeAmqp<quint8>(io, static_cast<quint8>(valueType));
    if (!ok)
        return false;

    return writeNativeFieldValue(io, value, valueType);
}

bool qmq::detail::Frame::writeNativeFieldValue(QIODevice *io,
                                               const QVariant &value,
                                               FieldValue valueType)
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

bool qmq::detail::Frame::writeNativeFieldValues(QIODevice *io,
                                                const QVariantList &values,
                                                const QList<FieldValue> &types)
{
    bool ok = true;
    int bitPos = 0;
    quint8 bitBuffer = 0;
    for (qsizetype i = 0; i < types.size(); ++i) {
        const FieldValue &type = types.at(i);
        const QVariant &value = values.at(i);
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
            ok = writeAmqp<quint8>(io, bitBuffer);
            if (!ok)
                return false;
            bitBuffer = 0;
            bitPos = 0;
        }
        if (type != FieldValue::Bit) {
            ok = writeNativeFieldValue(io, value, type);
        }
        if (!ok)
            return false;
    }
    return ok;
}

qmq::detail::Frame *qmq::detail::Frame::readFrame(QIODevice *io,
                                                  quint32 maxFrameSize,
                                                  ErrorCode *err)
{
    if (io->bytesAvailable() < (FrameHeaderSize + 1)) {
        *err = ErrorCode::InsufficientDataAvailable;
        return nullptr;
    }

    const int headerLen = FrameHeaderSize;
    char header[headerLen];
    if (io->peek(header, headerLen) != headerLen) {
        qWarning() << "peek failed";
        return nullptr;
    }
    const FrameType t = static_cast<FrameType>(header[0]);
    const quint16 channel = qFromBigEndian<quint16>(header + 1);
    const quint32 size = qFromBigEndian<quint32>(header + 3);

    if (maxFrameSize != 0 && size > maxFrameSize) {
        *err = ErrorCode::FrameTooLarge;
        return nullptr;
    }
    if (io->bytesAvailable() < (size + FrameHeaderSize + 1)) {
        qDebug() << "InsufficientDataAvailable" << io->bytesAvailable() << size << FrameHeaderSize;
        *err = ErrorCode::InsufficientDataAvailable;
        return nullptr;
    }

    io->skip(FrameHeaderSize);

    const QByteArray content = io->read((size));
    if (content.size() != (size)) {
        *err = ErrorCode::IoError;
        return nullptr;
    }

    bool ok;
    const quint8 endByte = readAmqp<quint8>(io, &ok);
    if (!ok || endByte != FrameEndChar) {
        *err = ErrorCode::InvalidFrameData;
        return nullptr;
    }
    switch (t) {
    case qmq::FrameType::Method:
        return MethodFrame::fromContent(channel, content);
    case qmq::FrameType::Header:
        return HeaderFrame::fromContent(channel, content);
    case qmq::FrameType::Body:
        return BodyFrame::fromContent(channel, content);
    case qmq::FrameType::Heartbeat:
        return HeartbeatFrame::fromContent(channel, content);
    default:
        *err = ErrorCode::UnknownFrameType;
        return nullptr;
    }
}

bool qmq::detail::Frame::writeFrame(QIODevice *io, quint32 maxFrameSize, Frame *f)
{
    qDebug() << "Write frame" << f->channel() << (int) f->type();
    const QByteArray content = f->content();

    const quint8 t = static_cast<quint8>(f->type());
    const quint16 channel = f->channel();
    const quint32 size = content.size();

    if (maxFrameSize != 0 && (content.size() + FrameHeaderSize + 1) > maxFrameSize) {
        qWarning() << "Cannot write frame: too large.";
        return false;
    }
    bool ok = writeAmqp<quint8>(io, t);
    ok = ok && writeAmqp<quint16>(io, channel);
    ok = ok && writeAmqp<quint32>(io, size);
    ok = ok && (io->write(content) == content.size());
    ok = ok && writeAmqp<quint8>(io, FrameEndChar);
    qDebug() << "Write frame" << ok << size;
    return ok;
}

qmq::detail::BodyFrame *qmq::detail::BodyFrame::fromContent(quint16 channel,
                                                            const QByteArray &content)
{
    return new BodyFrame(channel, content);
}
qmq::detail::MethodFrame *qmq::detail::MethodFrame::fromContent(quint16 channel,
                                                                const QByteArray &content)
{
    QBuffer io;
    io.setData(content);
    bool ok = io.open(QIODevice::ReadOnly);
    const quint16 classId = readAmqp<quint16>(&io, &ok);
    const quint16 methodId = readAmqp<quint16>(&io, &ok);
    const QByteArray arguments = content.mid(4);
    return new MethodFrame(channel, classId, methodId, arguments);
}

QByteArray qmq::detail::MethodFrame::content() const
{
    qDebug() << "Get method content";
    QBuffer io;
    bool ok = io.open(QIODevice::WriteOnly);
    ok = ok && writeAmqp<quint16>(&io, this->classId());
    ok = ok && writeAmqp<quint16>(&io, this->methodId());
    ok = ok && (io.write(this->m_arguments) == this->m_arguments.size());
    if (!ok) {
        qWarning() << "Failed writing frame content";
    }
    io.close();
    return io.data();
}

QVariantList qmq::detail::MethodFrame::getArguments(const QList<FieldValue> &types) const
{
    QBuffer io;
    io.setData(this->m_arguments);
    bool ok = io.open(QIODevice::ReadOnly);
    return Frame::readNativeFieldValues(&io, types, &ok);
}

bool qmq::detail::MethodFrame::setArguments(const QVariantList &values,
                                            const QList<FieldValue> &types)
{
    if (values.size() != types.size()) {
        qCritical() << "Size of values does not match types";
        return false;
    }
    QBuffer io;
    bool ok = io.open(QIODevice::WriteOnly);
    if (!ok)
        return false;
    ok = Frame::writeNativeFieldValues(&io, values, types);
    io.close();
    this->m_arguments = io.buffer();
    return ok;
}

qmq::detail::HeaderFrame *qmq::detail::HeaderFrame::fromContent(quint16 channel,
                                                                const QByteArray &content)
{
    return new HeaderFrame(channel);
}
qmq::detail::HeartbeatFrame *qmq::detail::HeartbeatFrame::fromContent(quint16 channel,
                                                                      const QByteArray &content)
{
    return new HeartbeatFrame();
}
