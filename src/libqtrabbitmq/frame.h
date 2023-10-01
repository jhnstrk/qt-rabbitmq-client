#pragma once

#include <qtrabbitmq/qtrabbitmq.h>

namespace qmq {
namespace detail {

enum class FrameType : quint8 {
    Method = 1,
    Header = 2,
    Body = 3,
    Heartbeat = 4,
};

class Frame
{
public:
    Frame() {}

    virtual ~Frame() {}

    FrameType type() const { return static_cast<FrameType>(m_type); }
    quint16 channel() const { return m_channel; }

    static QVariant readFieldValue(QIODevice *io, bool *ok = nullptr);
    static bool writeFieldValue(QIODevice *io, const QVariant &value);
    static bool writeFieldValue(QIODevice *io, const QVariant &value, FieldValue valueType);

private:
    quint8 m_type;
    quint16 m_channel;
};

} // namespace detail
} // namespace qmq
