#include <qtrabbitmq/qtrabbitmq.h>

namespace qmq {
QByteArray basicPropertyName(qmq::BasicProperty prop)
{
#define BASIC_PROPERTY_CASE(x) \
    case (qmq::BasicProperty::x): \
        return QByteArrayLiteral(#x)
    switch (prop) {
        BASIC_PROPERTY_CASE(ContentType);
        BASIC_PROPERTY_CASE(ContentEncoding);
        BASIC_PROPERTY_CASE(Headers);
        BASIC_PROPERTY_CASE(DeliveryMode);
        BASIC_PROPERTY_CASE(Priority);
        BASIC_PROPERTY_CASE(CorrelationId);
        BASIC_PROPERTY_CASE(ReplyTo);
        BASIC_PROPERTY_CASE(Expiration);
        BASIC_PROPERTY_CASE(MessageId);
        BASIC_PROPERTY_CASE(Timestamp);
        BASIC_PROPERTY_CASE(Type);
        BASIC_PROPERTY_CASE(UserId);
        BASIC_PROPERTY_CASE(AppId);
        BASIC_PROPERTY_CASE(_ClusterId);
    default:
        return QByteArray("BasicProperty_") + QByteArray::number((int) prop);
    };
}

} // namespace qmq
