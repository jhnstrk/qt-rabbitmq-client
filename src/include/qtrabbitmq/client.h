#pragma once

#include "channel.h"
#include "frame.h"

#include <QAbstractSocket>
#include <QFuture>
#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QSslError>
#include <QString>

namespace qmq {
class Client : public QObject
{
    Q_OBJECT
public:
    Client(QObject *parent = nullptr);
    ~Client();

    QUrl connectionUrl() const;
    bool connectToHost(const QUrl &url);

    QString virtualHost() const;

    QSharedPointer<Channel> createChannel();
    QSharedPointer<Channel> channel(quint16 channelId) const;

    QString username() const;
    QString password() const;

    quint32 maxFrameSizeBytes() const;
    void setMaxFrameSizeBytes(quint32 value);

    quint16 maxChannelId() const;
    void setMaxChannelId(quint16 mx);

    quint16 heartbeatSeconds() const;
    void setHeartbeatSeconds(quint16 n);

Q_SIGNALS:
    void connected();
    void disconnected();

public Q_SLOTS:
    bool sendFrame(const Frame *f);
    bool sendHeartbeat();

    void disconnectFromHost(quint16 code = 200,
                            const QString &replyText = QString(),
                            quint16 classId = 0,
                            quint16 methodId = 0);

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
