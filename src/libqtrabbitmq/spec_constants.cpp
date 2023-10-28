#include "spec_constants.h"

namespace qmq {

QList<qmq::FieldValue> methodArgs(quint16 methodClass, quint16 methodId)
{
    using namespace Domain;

    switch (methodClass) {
    case Connection::ID_:
        switch (methodId) {
            using namespace Connection;
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
            return {Path};
        case OpenOk:
            return {};
        case Close:
            return {ReplyCode, ReplyText, ClassId, MethodId};
        case CloseOk:
            return {};
        default:
            break;
        }
        break;
    case Channel::ID_:
        switch (methodId) {
            using namespace Channel;
        case Open:
            return {};
        case OpenOk:
            return {};
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
    case Exchange::ID_:
        switch (methodId) {
            using namespace Exchange;
        case Declare:
            return {ExchangeName, ShortStr, Bit, Bit, NoWait, Table};
        case DeclareOk:
            return {};
        case Delete:
            return {ExchangeName, Bit, NoWait};
        case DeleteOk:
            return {};
        default:
            break;
        }
        break;
    case Queue::ID_:
        switch (methodId) {
            using namespace Queue;
        case Declare:
            return {QueueName, Bit, Bit, Bit, Bit, NoWait, Table};
        case DeclareOk:
            return {QueueName, MessageCount, Long};
        case Bind:
            return {QueueName, ExchangeName, ShortStr, NoWait, Table};
        case BindOk:
            return {};
        case Unbind:
            return {QueueName, ExchangeName, ShortStr, Table};
        case UnbindOk:
            return {};
        case Purge:
            return {QueueName, NoWait};
        case PurgeOk:
            return {MessageCount};
        case Delete:
            return {QueueName, Bit, Bit, NoWait};
        case DeleteOk:
            return {MessageCount};
        default:
            break;
        }
        break;
    case Basic::ID_:
        switch (methodId) {
            using namespace Basic;
        case Qos:
            return {Long, Short, Bit};
        case QosOk:
            return {};
        case Consume:
            return {QueueName, ConsumerTag, NoLocal, NoAck, Bit, NoWait, Table};
        case ConsumeOk:
            return {ConsumerTag};
        case Cancel:
            return {ConsumerTag, NoWait};
        case CancelOk:
            return {ConsumerTag};
        case Publish:
            return {ExchangeName, ShortStr, Bit, Bit};
        case Return:
            return {ReplyCode, ReplyText, ExchangeName, ShortStr};
        case Deliver:
            return {ConsumerTag, DeliveryTag, Redelivered, ExchangeName, ShortStr};
        case Get:
            return {QueueName, NoAck};
        case GetOk:
            return {DeliveryTag, Redelivered, ExchangeName, ShortStr, MessageCount};
        case GetEmpty:
            return {};
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
    case Confirm::ID_:
        switch (methodId) {
            using namespace Confirm;
        default:
            break;
        }
        break;
    case Tx::ID_:
        switch (methodId) {
            using namespace Tx;
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

} // namespace qmq