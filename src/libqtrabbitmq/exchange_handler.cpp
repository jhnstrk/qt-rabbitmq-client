#include "exchange_handler.h"
#include "spec_constants.h"
#include <qtrabbitmq/client.h>

namespace qmq {
namespace detail {

ExchangeHandler::ExchangeHandler(Client *client,
                                 quint16 channelId,
                                 const QString &exchangeName,
                                 ExchangeType type)
    : m_client(client)
    , m_channelId(channelId)
    , m_exchangeName(exchangeName)
    , m_type(type)
{}

bool ExchangeHandler::handleFrame(const MethodFrame *frame)
{
    Q_ASSERT(frame->classId() == static_cast<quint16>(qmq::spec::exchange::ID_));
    switch (frame->methodId()) {
    case spec::exchange::DeclareOk:
        return this->onDeclareOk(frame);
    case spec::exchange::DeleteOk:
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
    const QString type = ExchangeHandler::typeToString(this->m_type);
    const bool passive = false;   // Used to check if an exchange exists.
    const bool durable = false;   // Durable exchanges remain active when a server restarts.
    const bool reserved2 = false; // auto-delete, must be zero
    const bool reserved3 = false; // internal, must be zero
    const bool noWait = false;    // If set, the server will not respond to the method.
    const QVariantHash arguments;

    QVariantList args(
        {reserved1, exchange, type, passive, durable, reserved2, reserved3, noWait, arguments});
    MethodFrame frame(this->m_channelId, spec::exchange::ID_, spec::exchange::Declare);
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
    MethodFrame frame(this->m_channelId, spec::exchange::ID_, spec::exchange::Delete);
    qDebug() << "Set excahnge::delete frame args" << args;
    frame.setArguments(args);
    return m_client->sendFrame(&frame);
}

QString ExchangeHandler::typeToString(ExchangeType type)
{
    switch (type) {
    case ExchangeType::Direct:
        return "amq.direct";
    case ExchangeType::Fanout:
        return "amq.fanout";
    case ExchangeType::Topic:
        return "amq.topic";
    case ExchangeType::Match:
        return "amq.match";
    case ExchangeType::Invalid:
        Q_FALLTHROUGH();
    default:
        qWarning() << "Unknown exchange";
        return QString();
    }
}

ExchangeHandler::ExchangeType ExchangeHandler::stringToType(const QString &typeStr)
{
    if (typeStr.isEmpty() || typeStr == "direct" || typeStr == "amq.direct") {
        return ExchangeType::Direct;
    }
    if (typeStr == "fanout" || typeStr == "amq.fanout") {
        return ExchangeType::Fanout;
    }
    if (typeStr == "topic" || typeStr == "amq.topic") {
        return ExchangeType::Topic;
    }
    if (typeStr == "match" || typeStr == "amq.match") {
        return ExchangeType::Match;
    }
    qWarning() << "Unknown exchange type" << typeStr;
    return ExchangeType::Invalid;
}

bool ExchangeHandler::onDeleteOk(const MethodFrame *)
{
    qDebug() << "spec::exchange::DeleteOk received";
    return true;
}

} // namespace detail
} // namespace qmq
