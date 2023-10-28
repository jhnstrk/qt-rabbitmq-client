#ifndef SPEC_CONSTANTS_H
#define SPEC_CONSTANTS_H

#include <qtrabbitmq/qtrabbitmq.h>

namespace qmq {

namespace Domain {
static constexpr const FieldValue Bit = FieldValue::Bit;
static constexpr const FieldValue ClassId = FieldValue::ShortInt;
static constexpr const FieldValue ConsumerTag = FieldValue::ShortString;
static constexpr const FieldValue DeliveryTag = FieldValue::LongLongInt;
static constexpr const FieldValue ExchangeName = FieldValue::ShortString;
static constexpr const FieldValue Long = FieldValue::LongInt;
static constexpr const FieldValue LongLong = FieldValue::LongLongInt;
static constexpr const FieldValue LongStr = FieldValue::LongString;
static constexpr const FieldValue MessageCount = FieldValue::LongInt;
static constexpr const FieldValue MethodId = FieldValue::ShortInt;
static constexpr const FieldValue NoAck = FieldValue::Bit;
static constexpr const FieldValue NoLocal = FieldValue::Bit;
static constexpr const FieldValue NoWait = FieldValue::Bit;
static constexpr const FieldValue Octet = FieldValue::ShortShortUint;
static constexpr const FieldValue Path = FieldValue::ShortString;
static constexpr const FieldValue PeerProperties = FieldValue::FieldTable;
static constexpr const FieldValue QueueName = FieldValue::ShortString;
static constexpr const FieldValue Redelivered = FieldValue::Bit;
static constexpr const FieldValue ReplyCode = FieldValue::ShortInt;
static constexpr const FieldValue ReplyText = FieldValue::ShortInt;
static constexpr const FieldValue Short = FieldValue::ShortInt;
static constexpr const FieldValue ShortStr = FieldValue::ShortString;
static constexpr const FieldValue Table = FieldValue::FieldTable;
static constexpr const FieldValue Timestamp = FieldValue::Timestamp;
}; // namespace Domain

namespace Connection {
constexpr const quint16 ID_ = 10, Start = 10, StartOk = 11, Secure = 20, SecureOk = 21, Tune = 30,
                        TuneOk = 31, Open = 40, OpenOk = 41, Close = 50, CloseOk = 51, Blocked = 60,
                        Unblocked = 61;

} // namespace Connection

namespace Channel {
constexpr const quint16 ID_ = 20, Open = 10, OpenOk = 11, Flow = 20, FlowOk = 21, Close = 40,
                        CloseOk = 41;
}

namespace Exchange {
constexpr const quint16 ID_ = 40, Declare = 10, DeclareOk = 11, Delete = 20, DeleteOk = 21,
                        Bind = 30, BindOk = 31, Unbind = 40, UnbindOk = 51;
};

namespace Queue {
constexpr const quint16 ID_ = 50, Declare = 10, DeclareOk = 11, Bind = 20, BindOk = 21, Purge = 30,
                        PurgeOk = 31, Delete = 40, DeleteOk = 41, Unbind = 50, UnbindOk = 51;
};

namespace Basic {
constexpr const quint16 ID_ = 60, Qos = 10, QosOk = 11, Consume = 20, ConsumeOk = 21, Cancel = 30,
                        CancelOk = 31, Publish = 40, Return = 50, Deliver = 60, Get = 70,
                        GetOk = 71, GetEmpty = 72, Ack = 80, Nack = 120, Reject = 90,
                        RecoverAsync = 100, Recover = 110, RecoverOk = 111;
};

namespace Confirm {
constexpr const quint16 ID_ = 85, Select = 10, SelectOk = 11;
};

namespace Tx {
constexpr const quint16 ID_ = 90, Select = 10, SelectOk = 11, Commit = 20, CommitOk = 21,
                        Rollback = 30, RollbackOk = 31;
};

namespace Constants {
static constexpr const int FrameMethod = 1;
static constexpr const int FrameHeader = 2;
static constexpr const int FrameBody = 3;
static constexpr const int FrameHeartbeat = 8;
static constexpr const int FrameMinSize = 4096;
static constexpr const int FrameEnd = 206;
static constexpr const int ReplySuccess = 200;
static constexpr const int ContentTooLarge = 311;
static constexpr const int NoConsumers = 313;
static constexpr const int ConnectionForced = 320;
static constexpr const int InvalidPath = 402;
static constexpr const int AccessRefused = 403;
static constexpr const int NotFound = 404;
static constexpr const int ResourceLocked = 405;
static constexpr const int PreconditionFailed = 406;
static constexpr const int FrameRrror = 501;
static constexpr const int SyntaxError = 502;
static constexpr const int CommandInvalid = 503;
static constexpr const int ChannelError = 504;
static constexpr const int UnexpectedFrame = 505;
static constexpr const int ResourceRrror = 506;
static constexpr const int NotAllowed = 530;
static constexpr const int NotImplemented = 540;
static constexpr const int InternalError = 541;
}; // namespace Constants

QList<qmq::FieldValue> methodArgs(quint16 methodClass, quint16 methodId);

} // namespace qmq

#endif // SPEC_CONSTANTS_H
