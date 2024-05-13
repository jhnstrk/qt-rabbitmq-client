#pragma once

#include "qtrabbitmq/abstract_frame_handler.h"

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <qglobal.h>

namespace qmq {
class Client;

namespace detail {
class ConnectionHandler : public QObject, public AbstractFrameHandler
{
    Q_OBJECT
public:
    ConnectionHandler(Client *client);
    bool handleMethodFrame(const MethodFrame &frame) override;
    bool handleHeaderFrame(const HeaderFrame &) override { return false; }
    bool handleBodyFrame(const BodyFrame &) override { return false; }
    bool handleHeartbeatFrame(const HeartbeatFrame &) override;

    bool sendClose(quint16 code, const QString &replyText, quint16 classId, quint16 methodId);

    quint32 maxFrameSizeBytes() const { return m_maxFrameSizeBytes; }
    quint16 maxChannelId() const { return m_channelMax; }
    quint16 heartbeatSeconds() const { return m_heartbeatSeconds; }
    void setTuneParameters(quint16 channelMax, quint32 maxFrameSizeBytes, quint16 heartbeatSeconds);

    // Any valid traffic from the server counts as a heartbeat.
    void resetTrafficFromServerHeartbeat();
Q_SIGNALS:
    void connectionOpened();
    void connectionClosed(quint16 code, const QString &replyText, quint16 classId, quint16 methodId);

protected:
    bool sendStartOk();
    bool sendTuneOk();

    bool sendOpen();
    bool onOpenOk(const MethodFrame &frame);
    bool onClose(const MethodFrame &frame);
    bool sendCloseOk();
    bool startHeartbeat();
    void stopHeartbeat();
    void onHeartbeatTimer();

private:
    bool onStart(const MethodFrame &frame);
    bool onTune(const MethodFrame &frame);
    bool onCloseOk(const MethodFrame &frame);

    Client *m_client = nullptr;
    QTimer *m_heartbeatTimer = nullptr;
    quint16 m_channelMax = 2047;
    quint32 m_maxFrameSizeBytes = 131072;
    quint16 m_heartbeatSeconds = 60;
    QDateTime m_lastheartbeatReceived;
    struct CloseArgs
    {
        quint16 code = 0;
        QString replyText;
        quint16 classId;
        quint16 methodId;
        bool isServerInitiated = false;
    };
    CloseArgs m_closeReason;
};

} // namespace detail
} // namespace qmq
