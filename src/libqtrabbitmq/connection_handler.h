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
    bool handleHeaderFrame(const HeaderFrame *frame) override {}
    bool handleBodyFrame(const BodyFrame *frame) override {}
    bool handleHeartbeatFrame(const HeartbeatFrame *frame) override {}

    bool sendClose(qint16 code, const QString &replyText, quint16 classId, quint16 methodId);

    quint32 frameMaxSizeBytes() const { return m_frameMaxSizeBytes; }

Q_SIGNALS:
    void connectionOpened();

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
    quint32 m_frameMaxSizeBytes = 1024 * 1024;
    quint16 m_heartbeatSeconds = 8;
};

} // namespace detail
} // namespace qmq
