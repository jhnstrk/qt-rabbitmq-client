#pragma once

#include "frame_handlers.h"
#include <qglobal.h>

namespace qmq {
class Client;

namespace detail {
class ExchangeHandler : public AbstractMethodHandler
{
public:
    ExchangeHandler(Client *client, quint16 channelId, const QString &exchangeName);
    bool handleFrame(const MethodFrame *frame) override;
    bool sendDeclare();
    bool sendDelete();

protected:
    bool onDeclareOk(const MethodFrame *frame);
    bool onDeleteOk(const MethodFrame *frame);

private:
    Client *m_client = nullptr;
    quint16 m_channelId = 0;
    QString m_exchangeName;
};

} // namespace detail
} // namespace qmq
