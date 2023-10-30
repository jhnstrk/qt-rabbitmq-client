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

} // namespace qmq
