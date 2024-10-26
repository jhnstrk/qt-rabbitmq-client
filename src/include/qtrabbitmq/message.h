#pragma once

#include <QByteArray>
#include <QHash>
#include <QMetaType>
#include <QVariant>

#include "qtrabbitmq.h"

#include "qtrabbitmq_export.h"

namespace qmq {

using BasicPropertyHash = QHash<BasicProperty, QVariant>;
class QTRABBITMQ_EXPORT Message
{
public:
    Message() = default;
    Message(const QByteArray &payload,
            const QString &exchangeName,
            const QString &routingKey = {},
            const BasicPropertyHash &properties = {})
        : m_payload(payload)
        , m_exchangeName(exchangeName)
        , m_routingKey(routingKey)
        , m_properties(properties)
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

    void setDeliveryTag(quint64 t) { m_deliveryTag = t; }
    quint64 deliveryTag() const { return m_deliveryTag; }

    void setRedelivered(bool v = true) { m_redelivered = v; }
    bool isRedelivered() const { return m_redelivered; }

    void setExchangeName(const QString &n) { m_exchangeName = n; }
    QString exchangeName() const { return m_exchangeName; }

private:
    QByteArray m_payload;
    QString m_exchangeName;
    QString m_routingKey;
    BasicPropertyHash m_properties;
    quint64 m_deliveryTag = 0;
    bool m_redelivered = false;
};

} // namespace qmq

QTRABBITMQ_EXPORT QDebug operator<<(QDebug debug, const qmq::Message &message);
QTRABBITMQ_EXPORT bool operator==(const qmq::Message &lhs, const qmq::Message &rhs);

Q_DECLARE_METATYPE(qmq::Message);
