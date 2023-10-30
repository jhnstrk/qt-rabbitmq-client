#pragma once

#include "frame_handlers.h"
#include <qglobal.h>

namespace qmq {
class Client;

namespace detail {
class ChannelHandler : public AbstractMethodHandler
{
public:
    ChannelHandler(Client *client);
    bool handleFrame(const MethodFrame *frame) override;
    bool sendOpen();
    bool sendClose(qint16 code, const QString &replyText, quint16 classId, quint16 methodId);

protected:
    bool onOpenOk(const MethodFrame *frame);
    bool onCloseOk(const MethodFrame *frame);

private:
    Client *m_client = nullptr;
    quint16 m_channelId = 0;
};

} // namespace detail
} // namespace qmq
