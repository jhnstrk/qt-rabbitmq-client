#pragma once

namespace qmq {

enum class FieldValue : char {
    Boolean = 't',
    ShortShortInt = 'b',
    ShortShortUint = 'B',
    ShortInt = 'U',
    ShortUint = 'u',
    LongInt = 'I',
    LongUint = 'i',
    LongLongInt = 'L',
    LongLongUint = 'l',
    Float = 'f',
    Double = 'd',
    DecimalValue = 'D',
    ShortString = 's',
    LongString = 'S',
    FieldArray = 'A',
    Timestamp = 'T',
    FieldTable = 'F',
    Void = 'V',
    Bit = 1, // Native type
    Invalid = 0
};

enum class FrameType { Method = 1, Header = 2, Body = 3, Heartbeat = 4, Invalid = 0 };

enum class BasicProperty {
    ContentType = 0,     // shortstr MIME content type
    ContentEncoding = 1, // shortstr MIME content encoding
    Headers = 2,         // table message header field table
    DeliveryMode = 3,    // octet nonpersistent (1) or persistent (2)
    Priority = 4,        // octet message priority, 0 to 9
    CorrelationId = 5,   // shortstr application correlation identifier
    ReplyTo = 6,         // shortstr address to reply to
    Expiration = 7,      // shortstr message expiration specification
    MessageId = 8,       // shortstr application message identifier
    Timestamp = 9,       // timestamp message timestamp
    Type = 10,           // shortstr message type name
    UserId = 11,         // shortstr creating user id
    AppId = 12,          // shortstr creating application id
    _ClusterId = 13,     // shortstr reserved, must be empty
};
} // namespace qmq
