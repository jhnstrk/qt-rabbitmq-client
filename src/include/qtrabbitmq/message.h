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
    Message(const QHash<BasicProperty, QVariant> &properties,
            const QByteArray &payload,
            const QString &routingKey)
        : m_properties(properties)
        , m_payload(payload)
        , m_routingKey(routingKey)
    {}

    ~Message() = default;

    QVariant property(BasicProperty p, const QVariant &defaultValue = QVariant()) const
    {
        return m_properties.value(p, defaultValue);
    }
    void setProperty(BasicProperty p, const QVariant &value) { m_properties.insert(p, value); }
    const QByteArray &payload() const { return m_payload; }
    void setPayload(const QByteArray &payload) { m_payload = payload; }
    void setPayload(const QString &payload)
    {
        m_payload = payload.toUtf8();
        this->setProperty(qmq::BasicProperty::ContentEncoding, "utf-8");
    }

    void setPayload(const char *payload)
    {
        m_payload = QByteArray(payload);
        this->setProperty(qmq::BasicProperty::ContentEncoding, "utf-8");
    }

    const QHash<BasicProperty, QVariant> &properties() const { return m_properties; }

    void setRoutingKey(const QString &key) { m_routingKey = key; }
    QString routingKey() const { return m_routingKey; }

    void setDeliveryTag(qint64 t) { m_deliveryTag = t; }
    qint64 deliveryTag() const { return m_deliveryTag; }

    void setRedelivered(bool v = true) { m_redelivered = v; }
    bool isRedelivered() const { return m_redelivered; }

    void setExchangeName(const QString &n) { m_exchangeName = n; }
    QString exchangeName() const { return m_exchangeName; }

private:
    QHash<BasicProperty, QVariant> m_properties;
    QByteArray m_payload;
    QString m_routingKey;
    qint64 m_deliveryTag = 0;
    bool m_redelivered = false;
    QString m_exchangeName;
};

} // namespace qmq
