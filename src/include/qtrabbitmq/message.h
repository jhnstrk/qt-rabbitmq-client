#pragma once

#include <QByteArray>
#include <QHash>
#include <QVariant>

#include "qtrabbitmq.h"

namespace qmq {
class Message
{
public:
    Message() = default;
    Message(const QHash<BasicProperty, QVariant> &properties, const QByteArray &payload)
        : m_properties(properties)
        , m_payload(payload)
    {}

    ~Message() = default;

    QVariant property(BasicProperty p, const QVariant &defaultValue) const
    {
        return m_properties.value(p, defaultValue);
    }
    void setProperty(BasicProperty p, const QVariant &value) { m_properties.insert(p, value); }
    const QByteArray &payload() const { return m_payload; }
    void setPayload(const QByteArray &payload) { m_payload = payload; }

    const QHash<BasicProperty, QVariant> &properties() const { return m_properties; }

private:
    QHash<BasicProperty, QVariant> m_properties;
    QByteArray m_payload;
};

} // namespace qmq
