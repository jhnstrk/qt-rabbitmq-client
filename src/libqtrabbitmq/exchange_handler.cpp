#include "exchange_handler.h"
#include "spec_constants.h"
#include <qtrabbitmq/client.h>

namespace qmq {
namespace detail {

ExchangeHandler::ExchangeHandler(Client *client, quint16 channelId, const QString &exchangeName)
    : m_client(client)
    , m_channelId(channelId)
    , m_exchangeName(exchangeName)
{}

bool ExchangeHandler::handleFrame(const MethodFrame *frame)
{
    Q_ASSERT(frame->classId() == static_cast<quint16>(qmq::Exchange::ID_));
    switch (frame->methodId()) {
    case Exchange::DeclareOk:
        return this->onDeclareOk(frame);
    case Exchange::DeleteOk:
        return this->onDeleteOk(frame);
    default:
        qWarning() << "Unknown exchange frame" << frame->methodId();
        break;
    }
    return false;
}

bool ExchangeHandler::sendDeclare()
{
    const qint16 reserved1 = 0; // ticket, must be zero
    const QString exchange = this->m_exchangeName;
    const QString type;
    const bool passive = false;   // Used to check if an exchange exists.
    const bool durable = false;   // Durable exchanges remain active when a server restarts.
    const bool reserved2 = false; // auto-delete, must be zero
    const bool reserved3 = false; // internal, must be zero
    const bool noWait = false;    // If set, the server will not respond to the method.
    const QVariantHash arguments;

    QVariantList args(
        {reserved1, exchange, type, passive, durable, reserved2, reserved3, noWait, arguments});
    MethodFrame frame(this->m_channelId, Exchange::ID_, Exchange::Declare);
    qDebug() << "Send exchange::declare method" << this->m_channelId << "frame args" << args;
    frame.setArguments(args);
    return m_client->sendFrame(&frame);
}

bool ExchangeHandler::onDeclareOk(const MethodFrame *frame)
{
    qDebug() << "Excange::DeclareOk received";
    return true;
}

bool ExchangeHandler::sendDelete()
{
    const qint16 reserved1 = 0; // "ticket", must be zero
    const QString exchange = this->m_exchangeName;
    const bool ifUnused = false;
    const bool noWait = false;
    QVariantList args({reserved1, exchange, ifUnused, noWait});
    MethodFrame frame(this->m_channelId, Exchange::ID_, Exchange::Delete);
    qDebug() << "Set excahnge::delete frame args" << args;
    frame.setArguments(args);
    return m_client->sendFrame(&frame);
}

bool ExchangeHandler::onDeleteOk(const MethodFrame *)
{
    qDebug() << "Exchange::DeleteOk received";
    return true;
}

} // namespace detail
} // namespace qmq
