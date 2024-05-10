#include "spec_constants.h"

#include <QDebug>
#include <QList>

namespace qmq {

namespace spec {

QList<qmq::FieldValue> methodArgs(quint16 methodClass, quint16 methodId)
{
    using namespace spec::domain;

    switch (methodClass) {
    case spec::connection::ID_:
        switch (methodId) {
            using namespace spec::connection;
        case Start:
            return {Octet, Octet, PeerProperties, LongStr, LongStr};
        case StartOk:
            return {PeerProperties, ShortStr, LongStr, ShortStr};
        case Secure:
            return {LongStr};
        case SecureOk:
            return {LongStr};
        case Tune:
            return {Short, Long, Short};
        case TuneOk:
            return {Short, Long, Short};
        case Open:
            return {Path, ShortStr, Bit};
        case OpenOk:
            return {ShortStr};
        case Close:
            return {ReplyCode, ReplyText, ClassId, MethodId};
        case CloseOk:
            return {};
        default:
            break;
        }
        break;
    case spec::channel::ID_:
        switch (methodId) {
            using namespace spec::channel;
        case Open:
            return {ShortStr};
        case OpenOk:
            return {LongStr};
        case Flow:
            return {Bit};
        case FlowOk:
            return {Bit};
        case Close:
            return {ReplyCode, ReplyText, ClassId, MethodId};
        case CloseOk:
            return {};
        default:
            break;
        }
        break;
    case spec::exchange::ID_:
        switch (methodId) {
            using namespace spec::exchange;
        case Declare:
            return {Short, ExchangeName, ShortStr, Bit, Bit, Bit, Bit, NoWait, Table};
        case DeclareOk:
            return {};
        case Delete:
            return {Short, ExchangeName, Bit, NoWait};
        case DeleteOk:
            return {};
        default:
            break;
        }
        break;
    case spec::queue::ID_:
        switch (methodId) {
            using namespace spec::queue;
        case Declare:
            return {Short, QueueName, Bit, Bit, Bit, Bit, NoWait, Table};
        case DeclareOk:
            return {QueueName, MessageCount, Long};
        case Bind:
            return {Short, QueueName, ExchangeName, ShortStr, NoWait, Table};
        case BindOk:
            return {};
        case Unbind:
            return {Short, QueueName, ExchangeName, ShortStr, Table};
        case UnbindOk:
            return {};
        case Purge:
            return {Short, QueueName, NoWait};
        case PurgeOk:
            return {MessageCount};
        case Delete:
            return {Short, QueueName, Bit, Bit, NoWait};
        case DeleteOk:
            return {MessageCount};
        default:
            break;
        }
        break;
    case spec::basic::ID_:
        switch (methodId) {
            using namespace spec::basic;
        case Qos:
            return {Long, Short, Bit};
        case QosOk:
            return {};
        case Consume:
            return {Short, QueueName, ConsumerTag, NoLocal, NoAck, Bit, NoWait, Table};
        case ConsumeOk:
            return {ConsumerTag};
        case Cancel:
            return {ConsumerTag, NoWait};
        case CancelOk:
            return {ConsumerTag};
        case Publish:
            return {Short, ExchangeName, ShortStr, Bit, Bit};
        case Return:
            return {ReplyCode, ReplyText, ExchangeName, ShortStr};
        case Deliver:
            return {ConsumerTag, DeliveryTag, Redelivered, ExchangeName, ShortStr};
        case Get:
            return {Short, QueueName, NoAck};
        case GetOk:
            return {DeliveryTag, Redelivered, ExchangeName, ShortStr, MessageCount};
        case GetEmpty:
            return {ShortStr};
        case Ack:
            return {DeliveryTag, Bit};
        case Reject:
            return {DeliveryTag, Bit};
        case RecoverAsync:
            return {Bit};
        case Recover:
            return {Bit};
        case RecoverOk:
            return {Bit};
        default:
            break;
        }
        break;
    case spec::confirm::ID_:
        // RabbitMQ extension. https://www.rabbitmq.com/amqp-0-9-1-quickref
        switch (methodId) {
            using namespace spec::confirm;
        case Select:
            return {NoWait};
        case SelectOk:
            return {};
        default:
            break;
        }
        break;
    case spec::tx::ID_:
        switch (methodId) {
            using namespace spec::tx;
        case Select:
            return {};
        case SelectOk:
            return {};
        case Commit:
            return {};
        case CommitOk:
            return {};
        case Rollback:
            return {};
        case RollbackOk:
            return {};
        default:
            break;
        }
        break;
    default:
        break;
    }

    qWarning() << "Unknown method" << methodClass << methodId;
    return QList<qmq::FieldValue>();
}

} // namespace spec

} // namespace qmq
