#pragma once

#include "frame.h"

#include <QAbstractSocket>
#include <QObject>
#include <QScopedPointer>
#include <QSslError>

namespace qmq {
class Client : public QObject
{
    Q_OBJECT
public:
    Client(QObject *parent = nullptr);
    ~Client();

    QUrl connectionUrl() const;
    void connectToHost(const QUrl &url);

    QString virtualHost() const;

Q_SIGNALS:
    void connected();

public Q_SLOTS:
    bool sendFrame(Frame *f);

    void disconnectFromHost();

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
