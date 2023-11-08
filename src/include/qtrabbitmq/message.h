#pragma once

#include <QHash>

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

private:
    QHash<BasicProperty, QVariant> m_properties;
    QByteArray m_payload;
};

} // namespace qmq
