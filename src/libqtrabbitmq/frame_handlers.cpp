#include "frame_handlers.h"
#include "spec_constants.h"

#include <qtrabbitmq/authentication.h>

namespace qmq {
namespace detail {

ConnectionHandler::ConnectionHandler(Client *client)
    : m_client(client)
{}

bool ConnectionHandler::handleFrame(const MethodFrame *frame)
{
    Q_ASSERT(frame->classId() == static_cast<quint16>(qmq::FrameType::Method));
    switch (frame->methodId()) {
    case Connection::Start:
        return this->onStart(frame);
    case Connection::Tune:
        return this->onTune(frame);
    default:
        qWarning() << "Unknown connection frame" << frame->methodId();
        break;
    }
    return false;
}

bool ConnectionHandler::onStart(const MethodFrame *frame)
{
    qDebug() << "Frane content" << frame->content();
    bool ok;
    const QVariantList args = frame->getArguments(methodArgs(frame->classId(), frame->methodId()),
                                                  &ok);
    if (!ok) {
        qWarning() << "Failed to parse args";
        return false;
    }
    qDebug() << "Start" << args;
    const QString protocolVersion = QString("%1.%2").arg(args.at(0).toInt()).arg(args.at(1).toInt());
    qDebug() << "Protocol:" << protocolVersion;
    this->sendStartOk();
    return true;
}

bool ConnectionHandler::sendStartOk()
{
    AmqpPlainAuthenticator auth;
    auth.setUsername("rabbituser");
    auth.setPassword("rabbitpass");
    QVariantHash clientProperties;
    const QString mechanism = auth.mechanism();
    const QByteArray response = auth.responseBytes("");
    const QString locale = "en_US";
    QVariantList args({clientProperties, mechanism, response, locale});
    qDebug() << "Create Method frame" << response;
    MethodFrame frame(0, Connection::ID_, Connection::StartOk);
    qDebug() << "Set method frame args" << args;
    frame.setArguments(args, methodArgs(Connection::ID_, Connection::StartOk));
    return m_client->sendFrame(&frame);
}

bool ConnectionHandler::onTune(const MethodFrame *frame)
{
    qDebug() << "Frane content" << frame->content();
    bool ok;
    const QVariantList args = frame->getArguments(methodArgs(frame->classId(), frame->methodId()),
                                                  &ok);
    if (!ok) {
        qWarning() << "Failed to parse args";
        return false;
    }
    qDebug() << "Tune" << args;

    this->m_channelMax = args.at(0).toInt(&ok);
    this->m_frameMaxSizeBytes = args.at(1).toLongLong(&ok);
    this->m_heartbeatSeconds = args.at(2).toInt(&ok);
    this->sendTuneOk();
    return true;
}

bool ConnectionHandler::sendTuneOk()
{
    QVariantList args({this->m_channelMax, this->m_frameMaxSizeBytes, this->m_heartbeatSeconds});
    MethodFrame frame(0, Connection::ID_, Connection::TuneOk);
    qDebug() << "Set method frame args" << args;
    frame.setArguments(args, methodArgs(Connection::ID_, Connection::TuneOk));
    return m_client->sendFrame(&frame);
}

} // namespace detail
} // namespace qmq
