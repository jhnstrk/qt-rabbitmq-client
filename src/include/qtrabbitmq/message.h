#pragma once

#include <QByteArray>
#include <QHash>
#include <QVariant>

#include "qtrabbitmq.h"

namespace qmq {
class Message
{
public:
    Message();
    ~Message();

    QVariant property(BasicProperty p, const QVariant &defaultValue) const
    {
        return m_properties.value(p, defaultValue);
    }
    QByteArray payload() const { return m_payload; }
    void setPayload(const QByteArray &payload) { m_payload = payload; }

private:
    QHash<BasicProperty, QVariant> m_properties;
    QByteArray m_payload;
};

} // namespace qmq
