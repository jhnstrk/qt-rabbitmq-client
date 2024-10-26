#pragma once

#include <QMetaType>
#include <qglobal.h>

#include "qtrabbitmq_export.h"

namespace qmq {

struct QTRABBITMQ_EXPORT Decimal
{
    explicit Decimal(quint8 s = 0, qint32 v = 0)
        : scale(s)
        , value(v)
    {}

    QString toString() const;
    double toDouble() const;
    quint8 scale = 0;
    qint32 value = 0;
};

QTRABBITMQ_EXPORT size_t qHash(const Decimal &key, size_t seed);

QTRABBITMQ_EXPORT inline bool operator==(const Decimal &lhs, const Decimal &rhs)
{
    // Exact quality, not numeric equality since zero isn't handled.
    return lhs.scale == rhs.scale && lhs.value == rhs.value;
}

//! Register type conversion functions with QMetaType
QTRABBITMQ_EXPORT void registerDecimalConverters();

} // namespace qmq

QDataStream &operator<<(QDataStream &out, const qmq::Decimal &decimal);
QDataStream &operator>>(QDataStream &in, qmq::Decimal &decimal);
QDebug operator<<(QDebug debug, const qmq::Decimal &decimal);

Q_DECLARE_METATYPE(qmq::Decimal);
