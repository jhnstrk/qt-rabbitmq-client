#pragma once

#include "qtrabbitmq/abstract_method_handler.h"
#include <qglobal.h>

namespace qmq {
class Client;

namespace detail {
class ConnectionHandler : public AbstractMethodHandler
{
public:
    ConnectionHandler(Client *client);
    bool handleFrame(const MethodFrame *frame) override;

protected:
    bool sendStartOk();
    bool sendTuneOk();

    bool sendOpen();
    bool sendClose(qint16 code, const QString &replyText, quint16 classId, quint16 methodId);
    bool onOpenOk(const MethodFrame *frame);
    bool onClose(const MethodFrame *frame);
    bool sendCloseOk();

private:
    bool onStart(const MethodFrame *frame);
    bool onTune(const MethodFrame *frame);
    bool onCloseOk(const MethodFrame *frame);

    Client *m_client = nullptr;
    quint16 m_channelMax = 255;
    quint32 m_frameMaxSizeBytes = 1024 * 1024;
    quint16 m_heartbeatSeconds = 8;
};

} // namespace detail
} // namespace qmq
