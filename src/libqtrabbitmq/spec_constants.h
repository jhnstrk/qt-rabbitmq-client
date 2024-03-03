#ifndef SPEC_CONSTANTS_H
#define SPEC_CONSTANTS_H

#include <qglobal.h>
#include <qtrabbitmq/qtrabbitmq.h>

#include <QList>

namespace qmq {

namespace spec {

namespace domain {
constexpr const FieldValue Bit = FieldValue::Bit;
constexpr const FieldValue ClassId = FieldValue::ShortInt;
constexpr const FieldValue ConsumerTag = FieldValue::ShortString;
constexpr const FieldValue DeliveryTag = FieldValue::LongLongInt;
constexpr const FieldValue ExchangeName = FieldValue::ShortString;
constexpr const FieldValue Long = FieldValue::LongInt;
constexpr const FieldValue LongLong = FieldValue::LongLongInt;
constexpr const FieldValue LongStr = FieldValue::LongString;
constexpr const FieldValue MessageCount = FieldValue::LongInt;
constexpr const FieldValue MethodId = FieldValue::ShortInt;
constexpr const FieldValue NoAck = FieldValue::Bit;
constexpr const FieldValue NoLocal = FieldValue::Bit;
constexpr const FieldValue NoWait = FieldValue::Bit;
constexpr const FieldValue Octet = FieldValue::ShortShortUint;
constexpr const FieldValue Path = FieldValue::ShortString;
constexpr const FieldValue PeerProperties = FieldValue::FieldTable;
constexpr const FieldValue QueueName = FieldValue::ShortString;
constexpr const FieldValue Redelivered = FieldValue::Bit;
constexpr const FieldValue ReplyCode = FieldValue::ShortInt;
constexpr const FieldValue ReplyText = FieldValue::ShortString;
constexpr const FieldValue Short = FieldValue::ShortInt;
constexpr const FieldValue ShortStr = FieldValue::ShortString;
constexpr const FieldValue Table = FieldValue::FieldTable;
constexpr const FieldValue Timestamp = FieldValue::Timestamp;
}; // namespace domain

namespace connection {
constexpr const quint16 ID_ = 10, Start = 10, StartOk = 11, Secure = 20, SecureOk = 21, Tune = 30,
                        TuneOk = 31, Open = 40, OpenOk = 41, Close = 50, CloseOk = 51, Blocked = 60,
                        Unblocked = 61;

} // namespace connection

namespace channel {
constexpr const quint16 ID_ = 20, Open = 10, OpenOk = 11, Flow = 20, FlowOk = 21, Close = 40,
                        CloseOk = 41;
}

namespace exchange {
constexpr const quint16 ID_ = 40, Declare = 10, DeclareOk = 11, Delete = 20, DeleteOk = 21,
                        Bind = 30, BindOk = 31, Unbind = 40, UnbindOk = 51;
};

namespace queue {
constexpr const quint16 ID_ = 50, Declare = 10, DeclareOk = 11, Bind = 20, BindOk = 21, Purge = 30,
                        PurgeOk = 31, Delete = 40, DeleteOk = 41, Unbind = 50, UnbindOk = 51;
};

namespace basic {
constexpr const quint16 ID_ = 60, Qos = 10, QosOk = 11, Consume = 20, ConsumeOk = 21, Cancel = 30,
                        CancelOk = 31, Publish = 40, Return = 50, Deliver = 60, Get = 70,
                        GetOk = 71, GetEmpty = 72, Ack = 80, Nack = 120, Reject = 90,
                        RecoverAsync = 100, Recover = 110, RecoverOk = 111;
};

namespace confirm {
constexpr const quint16 ID_ = 85, Select = 10, SelectOk = 11;
};

namespace tx {
constexpr const quint16 ID_ = 90, Select = 10, SelectOk = 11, Commit = 20, CommitOk = 21,
                        Rollback = 30, RollbackOk = 31;
};

namespace constants {
constexpr const int FrameMethod = 1;
constexpr const int FrameHeader = 2;
constexpr const int FrameBody = 3;
constexpr const int FrameHeartbeat = 8;
constexpr const int FrameMinSize = 4096;
constexpr const int FrameEnd = 206;
constexpr const int ReplySuccess = 200;
constexpr const int ContentTooLarge = 311;
constexpr const int NoConsumers = 313;
constexpr const int ConnectionForced = 320;
constexpr const int InvalidPath = 402;
constexpr const int AccessRefused = 403;
constexpr const int NotFound = 404;
constexpr const int ResourceLocked = 405;
constexpr const int PreconditionFailed = 406;
constexpr const int FrameRrror = 501;
constexpr const int SyntaxError = 502;
constexpr const int CommandInvalid = 503;
constexpr const int ChannelError = 504;
constexpr const int UnexpectedFrame = 505;
constexpr const int ResourceRrror = 506;
constexpr const int NotAllowed = 530;
constexpr const int NotImplemented = 540;
constexpr const int InternalError = 541;
}; // namespace constants

QList<qmq::FieldValue> methodArgs(quint16 methodClass, quint16 methodId);

constexpr qmq::FieldValue basicPropertyTypes[] = {
    domain::ShortStr,  // ContentType = 0,     // shortstr MIME content type
    domain::ShortStr,  // ContentEncoding = 1, // shortstr MIME content encoding
    domain::Table,     // Headers = 2,         // table message header field table
    domain::Octet,     // DeliveryMode = 3,    // octet nonpersistent (1) or persistent (2)
    domain::Octet,     // Priority = 4,        // octet message priority, 0 to 9
    domain::ShortStr,  // CorrelationId = 5,   // shortstr application correlation identifier
    domain::ShortStr,  // ReplyTo = 6,         // shortstr address to reply to
    domain::ShortStr,  // Expiration = 7,      // shortstr message expiration specification
    domain::ShortStr,  // MessageId = 8,       // shortstr application message identifier
    domain::Timestamp, // Timestamp = 9,       // timestamp message timestamp
    domain::ShortStr,  // Type = 10,           // shortstr message type name
    domain::ShortStr,  // UserId = 11,         // shortstr creating user id
    domain::ShortStr,  // AppId = 12,          // shortstr creating application id
    domain::ShortStr,  //_ClusterId = 13,     // shortstr reserved, must be empty
};
} // namespace spec

} // namespace qmq

#endif // SPEC_CONSTANTS_H
