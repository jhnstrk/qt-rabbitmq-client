#include "connection_handler.h"
#include "spec_constants.h"
#include <qtrabbitmq/authentication.h>
#include <qtrabbitmq/client.h>

namespace {
const quint16 channel0 = 0; // All connection methods use channel = 0.
}
namespace qmq {
namespace detail {

ConnectionHandler::ConnectionHandler(Client *client)
    : m_client(client)
{}

bool ConnectionHandler::handleFrame(const MethodFrame *frame)
{
    Q_ASSERT(frame->classId() == static_cast<quint16>(qmq::spec::connection::ID_));
    switch (frame->methodId()) {
    case spec::connection::Start:
        return this->onStart(frame);
    case spec::connection::Tune:
        return this->onTune(frame);
    case spec::connection::OpenOk:
        return this->onOpenOk(frame);
    case spec::connection::Close:
        return this->onClose(frame);
    case spec::connection::CloseOk:
        return this->onCloseOk(frame);
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
    const QVariantList args = frame->getArguments(&ok);
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
    MethodFrame frame(channel0, spec::connection::ID_, spec::connection::StartOk);
    qDebug() << "Set method frame args" << args;
    frame.setArguments(args);
    return m_client->sendFrame(&frame);
}

bool ConnectionHandler::onTune(const MethodFrame *frame)
{
    qDebug() << "Frane content" << frame->content();
    bool ok;
    const QVariantList args = frame->getArguments(&ok);
    if (!ok) {
        qWarning() << "Failed to parse args";
        return false;
    }
    qDebug() << "Tune" << args;

    this->m_channelMax = args.at(0).toInt(&ok);
    this->m_frameMaxSizeBytes = args.at(1).toLongLong(&ok);
    this->m_heartbeatSeconds = args.at(2).toInt(&ok);
    this->sendTuneOk() && this->sendOpen();
    return true;
}

bool ConnectionHandler::sendTuneOk()
{
    QVariantList args({this->m_channelMax, this->m_frameMaxSizeBytes, this->m_heartbeatSeconds});
    MethodFrame frame(channel0, spec::connection::ID_, spec::connection::TuneOk);
    qDebug() << "Set method frame args" << args;
    frame.setArguments(args);
    return m_client->sendFrame(&frame);
}

bool ConnectionHandler::sendOpen()
{
    const QString virtualHost = m_client->virtualHost();
    const QString reserved1 = 0;  // capabilities
    const bool reserved2 = false; // insist
    QVariantList args({virtualHost, reserved1, reserved2});
    MethodFrame frame(channel0, spec::connection::ID_, spec::connection::Open);
    qDebug() << "Set method frame args" << args;
    frame.setArguments(args);
    return m_client->sendFrame(&frame);
}

bool ConnectionHandler::onOpenOk(const MethodFrame *frame)
{
    qDebug() << "OpenOk";
    emit this->connectionOpened();

    // m_client->sendHeartbeat();
    return true;
}

bool ConnectionHandler::onClose(const MethodFrame *frame)
{
    bool ok;
    const QVariantList args = frame->getArguments(&ok);
    if (!ok) {
        qWarning() << "Failed to parse args";
        return false;
    }
    qDebug() << "Close" << args;
    const int code = args.at(0).toInt();
    const QString replyText = args.at(1).toString();
    const int classId = args.at(2).toInt();
    const int methodId = args.at(3).toInt();
    qDebug() << "Received close";
    this->sendCloseOk();
    m_client->disconnectFromHost();
    return true;
}

bool ConnectionHandler::sendCloseOk()
{
    MethodFrame frame(channel0, spec::connection::ID_, spec::connection::CloseOk);
    qDebug() << "Sending CloseOk";
    return m_client->sendFrame(&frame);
}

bool ConnectionHandler::sendClose(qint16 code,
                                  const QString &replyText,
                                  quint16 classId,
                                  quint16 methodId)
{
    QVariantList args({code, replyText, classId, methodId});
    MethodFrame frame(channel0, spec::connection::ID_, spec::connection::Close);
    qDebug() << "Set close frame args" << args;
    frame.setArguments(args);
    return m_client->sendFrame(&frame);
}

bool ConnectionHandler::onCloseOk(const MethodFrame *)
{
    qDebug() << "CloseOk received";
    m_client->disconnectFromHost();
    return true;
}

} // namespace detail
} // namespace qmq
