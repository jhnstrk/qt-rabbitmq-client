#include <qtrabbitmq/message.h>

#include <QDataStream>
#include <QDebug>
#include <QMetaType>

QDebug operator<<(QDebug debug, const qmq::Message &message)
{
    QDebugStateSaver saver(debug);
    const int maxDebugLen = 64;
    debug.noquote().nospace() << "qmq::Message(payload=\"" << message.payload().left(maxDebugLen)
                              << ((message.payload().size() > maxDebugLen) ? "..." : "")
                              << "\", deliveryTag=" << message.deliveryTag() << ")";
    return debug;
}

bool operator==(const qmq::Message &lhs, const qmq::Message &rhs)
{
    return (lhs.deliveryTag() == rhs.deliveryTag()) && (lhs.exchangeName() == rhs.exchangeName())
           && (lhs.isRedelivered() == rhs.isRedelivered()) && (lhs.payload() == rhs.payload())
           && (lhs.properties() == rhs.properties()) && (lhs.routingKey() == rhs.routingKey());
}
