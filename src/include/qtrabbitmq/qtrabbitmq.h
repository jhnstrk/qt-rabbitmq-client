#pragma once

#include <QAbstractSocket>
#include <QObject>
#include <QScopedPointer>
#include <QSslError>

class QUrl;

namespace qmq {

struct Decimal
{
    explicit Decimal(quint8 s = 0, qint32 v = 0)
        : scale(s)
        , value(v)
    {}

    QString toString() const;
    quint8 scale = 0;
    qint32 value = 0;
};

size_t qHash(const Decimal &key, size_t seed);
inline bool operator==(const Decimal &lhs, const Decimal &rhs)
{
    return lhs.scale == rhs.scale && lhs.value == rhs.value;
}

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
    Bit = 'x',
    Invalid = 0
};

enum class FrameType { Method = 1, Header = 2, Body = 3, Heartbeat = 4, Invalid = 0 };

class Client : public QObject
{
    Q_OBJECT

    Client(QObject *parent = nullptr);
    ~Client();

    QUrl connectionUrl() const;
    void connectToHost(const QUrl &url);

protected Q_SLOTS:
    void onSocketConnected();
    void onSocketReadyRead();
    void onSocketErrorOccurred(QAbstractSocket::SocketError error);
    void onSocketStateChanged(QAbstractSocket::SocketState state);
    void onSocketSslErrors(const QList<QSslError> &errors);

private:
    Q_DISABLE_COPY(Client)

    class Private;
    QScopedPointer<Private> d;
};

} // namespace qmq

QDataStream &operator<<(QDataStream &out, const qmq::Decimal &decimal);
QDataStream &operator>>(QDataStream &in, qmq::Decimal &decimal);
QDebug operator<<(QDebug debug, const qmq::Decimal &decimal);

Q_DECLARE_METATYPE(qmq::Decimal);
