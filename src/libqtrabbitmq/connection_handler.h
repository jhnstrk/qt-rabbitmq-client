#pragma once

#include "qtrabbitmq/abstract_frame_handler.h"

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
    bool handleMethodFrame(const MethodFrame *frame) override;
    bool handleHeaderFrame(const HeaderFrame *) override { return false; }
    bool handleBodyFrame(const BodyFrame *) override { return false; }
    bool handleHeartbeatFrame(const HeartbeatFrame *) override { return false; }

    bool sendClose(qint16 code, const QString &replyText, quint16 classId, quint16 methodId);

    quint32 maxFrameSizeBytes() const { return m_maxFrameSizeBytes; }

Q_SIGNALS:
    void connectionOpened();
    void connectionClosed(qint16 code, const QString &replyText, quint16 classId, quint16 methodId);

protected:
    bool sendStartOk();
    bool sendTuneOk();

    bool sendOpen();
    bool onOpenOk(const MethodFrame *frame);
    bool onClose(const MethodFrame *frame);
    bool sendCloseOk();
    bool startHeartbeat();
    void stopHeartbeat();
    void onHeartbeatTimer();

private:
    bool onStart(const MethodFrame *frame);
    bool onTune(const MethodFrame *frame);
    bool onCloseOk(const MethodFrame *frame);

    Client *m_client = nullptr;
    QTimer *m_heartbeatTimer = nullptr;
    quint16 m_channelMax = 255;
    quint32 m_maxFrameSizeBytes = 1024 * 1024;
    quint16 m_heartbeatSeconds = 8;
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
