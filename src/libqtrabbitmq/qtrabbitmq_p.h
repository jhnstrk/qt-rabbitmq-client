#pragma once

namespace qmq {
namespace detail {

constexpr const int AmqpMajorVersion = 0;
constexpr const int AmqpMinorVersion = 9;
constexpr const int AmqpRevisionVersion = 1;

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
    NoField = 'V',
    InValid = 0
};

}
