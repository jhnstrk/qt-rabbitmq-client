#include <qtrabbitmq/qtrabbitmq.h>

#include <QDataStream>
#include <QDebug>
#include <QHash>
#include <QMetaType>

namespace qmq {

QString Decimal::toString() const
{
    qint32 v = this->value;
    if (v < 0) {
        v = -v;
    }
    QByteArray n = QByteArray::number(v);
    if (this->scale) {
        if (this->scale > 10) {
            const int eValue = int(this->scale) - n.size() + 1;
            n.insert(1, '.');
            n += "e-";
            n += QByteArray::number(eValue);
        } else {
            if (n.length() < this->scale + 1) {
                n.prepend(QByteArray(this->scale + 1 - n.length(), '0'));
            }
            n.insert(n.length() - this->scale, '.');
        }
    }
    if (this->value < 0) {
        n.push_front('-');
    }
    return QString::fromLatin1(n);
}

double Decimal::toDouble() const
{
    return double(this->value) * std::pow(10., -double(this->scale));
}

size_t qHash(const Decimal &key, size_t seed)
{
    return ::qHash(key.scale, seed) ^ ::qHash(key.value, seed);
}

void registerDecimalConverters()
{
    QMetaType::registerConverter<qmq::Decimal, double>(&qmq::Decimal::toDouble);
    QMetaType::registerConverter<qmq::Decimal, QString>(&qmq::Decimal::toString);
}

} // namespace qmq

QDataStream &operator<<(QDataStream &out, const qmq::Decimal &decimal)
{
    return out << decimal.scale << decimal.value;
}

QDataStream &operator>>(QDataStream &in, qmq::Decimal &decimal)
{
    return in >> decimal.scale >> decimal.value;
}
QDebug operator<<(QDebug debug, const qmq::Decimal &decimal)
{
    QDebugStateSaver saver(debug);
    debug.noquote() << decimal.toString();
    return debug;
}
