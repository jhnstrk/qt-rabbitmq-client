#pragma once

#include <QAbstractSocket>
#include <QObject>
#include <QScopedPointer>

class QUrl;

namespace qmq {

struct Decimal
{
    Decimal(quint8 s = 0, quint32 v = 0)
        : scale(s)
        , value(v)
    {}

    quint8 scale = 0;
    quint32 value = 0;
};

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
    Invalid = 0
};

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
    void onSocketErrorOccurred();
    void onSocketStateChanged();
    void onSocketSslErrors();

private:
    Q_DISABLE_COPY(Client)

    class Private;
    QScopedPointer<Private> d;
};

} // namespace qmq
