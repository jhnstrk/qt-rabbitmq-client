#include "channel_handler.h"
#include "spec_constants.h"
#include <qtrabbitmq/client.h>

namespace qmq {
namespace detail {

ChannelHandler::ChannelHandler(Client *client, quint16 channelId)
    : m_client(client)
    , m_channelId(channelId)
{}

bool ChannelHandler::handleFrame(const MethodFrame *frame)
{
    Q_ASSERT(frame->classId() == static_cast<quint16>(qmq::Channel::ID_));
    switch (frame->methodId()) {
    case Channel::OpenOk:
        return this->onOpenOk(frame);
    case Channel::CloseOk:
        return this->onCloseOk(frame);
    default:
        qWarning() << "Unknown channel frame" << frame->methodId();
        break;
    }
    return false;
}

bool ChannelHandler::sendOpen()
{
    const QString reserved1; // out-of-band
    QVariantList args({reserved1});
    MethodFrame frame(this->m_channelId, Channel::ID_, Channel::Open);
    qDebug() << "Set open channel method" << this->m_channelId << "frame args" << args;
    frame.setArguments(args);
    return m_client->sendFrame(&frame);
}

bool ChannelHandler::onOpenOk(const MethodFrame *frame)
{
    bool ok;
    const QVariantList args = frame->getArguments(&ok);
    if (!ok) {
        qWarning() << "Failed to parse args";
        return false;
    }
    qDebug() << "Channel::OpenOk" << args;
    return true;
}

bool ChannelHandler::sendClose(qint16 code,
                               const QString &replyText,
                               quint16 classId,
                               quint16 methodId)
{
    QVariantList args({code, replyText, classId, methodId});
    MethodFrame frame(this->m_channelId, Channel::ID_, Channel::Close);
    qDebug() << "Set close frame args" << args;
    frame.setArguments(args);
    return m_client->sendFrame(&frame);
}

bool ChannelHandler::onCloseOk(const MethodFrame *)
{
    qDebug() << "CloseOk received";
    return true;
}

} // namespace detail
} // namespace qmq
