#pragma once

#include <qglobal.h>
#include <qtrabbitmq/qtrabbitmq.h>

#include <QIODevice>
#include <QList>
#include <QMetaType>
#include <QScopedPointer>
#include <QVariant>

namespace qmq {

enum class ErrorCode {
    NoError = 0,
    InsufficientDataAvailable = 1,
    FrameTooLarge = 2,
    IoError,
    UnknownFrameType,
    InvalidFrameData,
};

class Frame
{
public:
    virtual ~Frame() {}

    FrameType type() const { return m_type; }
    void setChannel(quint16 channel) { m_channel = channel; }
    quint16 channel() const { return m_channel; }

    virtual QByteArray content() const = 0;

    static QVariant readFieldValue(QIODevice *io, bool *ok = nullptr);
    static QVariant readNativeFieldValue(QIODevice *io, FieldValue valueType, bool *ok = nullptr);
    static QVariantList readNativeFieldValues(QIODevice *io,
                                              const QList<FieldValue> &valueTypes,
                                              bool *ok = nullptr);

    static bool writeFieldValue(QIODevice *io, const QVariant &value);
    static bool writeFieldValue(QIODevice *io, const QVariant &value, FieldValue valueType);
    static bool writeNativeFieldValue(QIODevice *io, const QVariant &value, FieldValue valueType);
    static bool writeNativeFieldValues(QIODevice *io,
                                       const QVariantList &values,
                                       const QList<FieldValue> &valueTypes);

    //! maxFrameSize of 0 is treated as unlimited.
    static QScopedPointer<Frame> readFrame(QIODevice *io, quint32 maxFrameSize, ErrorCode *err);
    static bool writeFrame(QIODevice *io, quint32 maxFrameSize, const Frame *f);

    //! Note that bit type isn't handled here.
    static qmq::FieldValue metatypeToFieldValue(int typeId);
    static QMetaType::Type fieldValueToMetatype(qmq::FieldValue fieldtype);

protected:
    Frame(qmq::FrameType type, quint16 channel = 0)
        : m_type(type)
        , m_channel(channel)
    {}

private:
    qmq::FrameType m_type;
    quint16 m_channel;
    static const int FrameHeaderSize = 7;
    static const quint8 FrameEndChar = 0xCE;
};

class MethodFrame : public Frame
{
public:
    static QScopedPointer<MethodFrame> fromContent(quint16 channel, const QByteArray &content);

    quint16 classId() const { return m_classId; }
    quint16 methodId() const { return m_methodId; }

    QByteArray content() const override;

    QVariantList getArguments(bool *ok = nullptr) const;
    bool setArguments(const QVariantList &values);

    MethodFrame(quint16 channel, quint16 classId, quint16 methodId)
        : Frame(qmq::FrameType::Method, channel)
        , m_classId(classId)
        , m_methodId(methodId)
        , m_arguments()
    {}

protected:
    MethodFrame(quint16 channel, quint16 classId, quint16 methodId, const QByteArray &arguments)
        : Frame(qmq::FrameType::Method, channel)
        , m_classId(classId)
        , m_methodId(methodId)
        , m_arguments(arguments)
    {}

private:
    quint16 m_classId = 0;
    quint16 m_methodId = 0;
    QByteArray m_arguments;
};

class HeaderFrame : public Frame
{
public:
    static QScopedPointer<HeaderFrame> fromContent(quint16 channel, const QByteArray &content);

    QByteArray body() const { return m_body; }
    void setBody(const QByteArray &body) { m_body = body; }

    QByteArray content() const override { return m_body; }

private:
    HeaderFrame(quint16 channel)
        : Frame(qmq::FrameType::Header, channel)
    {}
    QByteArray m_body;
};

class BodyFrame : public Frame
{
public:
    static QScopedPointer<BodyFrame> fromContent(quint16 channel, const QByteArray &content);

    QByteArray body() const { return m_body; }
    void setBody(const QByteArray &body) { m_body = body; }

    QByteArray content() const override { return m_body; }

private:
    BodyFrame(quint16 channel, const QByteArray &body)
        : Frame(qmq::FrameType::Body, channel)
        , m_body(body)
    {}
    QByteArray m_body;
};

class HeartbeatFrame : public Frame
{
public:
    HeartbeatFrame()
        : Frame(qmq::FrameType::Heartbeat, 0)
    {}

    static QScopedPointer<HeartbeatFrame> fromContent(quint16 channel, const QByteArray &content);

    QByteArray content() const override { return QByteArray(); }

private:
};

} // namespace qmq
