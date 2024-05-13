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

void ConnectionHandler::setTuneParameters(quint16 channelMax,
                                          quint32 maxFrameSizeBytes,
                                          quint16 heartbeatSeconds)
{
    this->m_channelMax = channelMax;
    this->m_maxFrameSizeBytes = maxFrameSizeBytes;
    this->m_heartbeatSeconds = heartbeatSeconds;
}

bool ConnectionHandler::handleMethodFrame(const MethodFrame &frame)
{
    Q_ASSERT(frame.classId() == static_cast<quint16>(qmq::spec::connection::ID_));
    switch (frame.methodId()) {
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
        qWarning() << "Unknown connection frame" << frame.methodId();
        break;
    }
    return false;
}

bool ConnectionHandler::handleHeartbeatFrame(const HeartbeatFrame &)
{
    qDebug() << "Received heartbeat";
    return true;
}

bool ConnectionHandler::onStart(const MethodFrame &frame)
{
    bool isOk = false;
    const QVariantList args = frame.getArguments(&isOk);
    if (!isOk) {
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
    auth.setUsername(m_client->username().toUtf8());
    auth.setPassword(m_client->password().toUtf8());
    QVariantHash clientProperties;
    const QString mechanism = auth.mechanism();
    const QByteArray response = auth.responseBytes("");
    const QString locale = "en_US";
    const QVariantList args({clientProperties, mechanism, response, locale});
    MethodFrame frame(channel0, spec::connection::ID_, spec::connection::StartOk);
    qDebug() << "Set startOk method frame args" << args;
    frame.setArguments(args);
    return m_client->sendFrame(frame);
}

bool ConnectionHandler::onTune(const MethodFrame &frame)
{
    bool ok = false;
    const QVariantList args = frame.getArguments(&ok);
    if (!ok) {
        qWarning() << "Failed to parse args";
        return false;
    }
    qDebug() << "Tune" << args;

    const quint16 channelMax = args.at(0).toUInt(&ok);
    if (channelMax != 0) {
        // zero means no limit is specfied. Choose something.
        this->m_channelMax = std::min(channelMax, this->m_channelMax);
    }

    const quint32 frameMaxSizeBytes = args.at(1).toUInt(&ok);
    if (frameMaxSizeBytes != 0) {
        // zero means no limit is specfied. Chose something.
        this->m_maxFrameSizeBytes = std::min(frameMaxSizeBytes, this->m_maxFrameSizeBytes);
    }

    const quint16 heartbeatSeconds = args.at(2).toUInt(&ok);
    if (heartbeatSeconds != 0) {
        // zero means no heartbeat is desired.
        this->m_heartbeatSeconds = std::min(heartbeatSeconds, this->m_heartbeatSeconds);
    }

    if (!this->sendTuneOk()) {
        return false;
    }
    if (!this->sendOpen()) {
        return false;
    }
    if (!this->startHeartbeat()) {
        return false;
    }
    return true;
}

bool ConnectionHandler::sendTuneOk()
{
    const QVariantList args({QVariant::fromValue(this->m_channelMax),
                             this->m_maxFrameSizeBytes,
                             QVariant::fromValue(this->m_heartbeatSeconds)});
    MethodFrame frame(channel0, spec::connection::ID_, spec::connection::TuneOk);
    qDebug() << "Set TuneOk method frame args" << args;
    frame.setArguments(args);
    return m_client->sendFrame(frame);
}

bool ConnectionHandler::sendOpen()
{
    const QString virtualHost = m_client->virtualHost();
    const QString reserved1;      // capabilities
    const bool reserved2 = false; // insist
    const QVariantList args({virtualHost, reserved1, reserved2});
    MethodFrame frame(channel0, spec::connection::ID_, spec::connection::Open);
    qDebug() << "Set open method frame args" << args;
    frame.setArguments(args);
    return m_client->sendFrame(frame);
}

bool ConnectionHandler::onOpenOk(const MethodFrame &frame)
{
    Q_UNUSED(frame);
    qDebug() << "OpenOk";
    emit this->connectionOpened();
    return true;
}

bool ConnectionHandler::onClose(const MethodFrame &frame)
{
    bool isOk = false;
    const QVariantList args = frame.getArguments(&isOk);
    if (!isOk) {
        qWarning() << "Failed to parse args";
        return false;
    }
    qDebug() << "Close" << args;
    m_closeReason.code = args.at(0).value<quint16>();
    m_closeReason.replyText = args.at(1).toString();
    m_closeReason.classId = args.at(2).value<quint16>();
    m_closeReason.methodId = args.at(3).value<quint16>();
    m_closeReason.isServerInitiated = true;
    qDebug() << "Received close" << m_closeReason.code << m_closeReason.replyText
             << m_closeReason.classId << m_closeReason.methodId;
    this->sendCloseOk();
    return true;
}

bool ConnectionHandler::sendCloseOk()
{
    MethodFrame frame(channel0, spec::connection::ID_, spec::connection::CloseOk);
    qDebug() << "Sending CloseOk";
    const bool isOk = m_client->sendFrame(frame);
    emit connectionClosed(m_closeReason.code,
                          m_closeReason.replyText,
                          m_closeReason.classId,
                          m_closeReason.methodId);
    return isOk;
}

bool ConnectionHandler::sendClose(quint16 code,
                                  const QString &replyText,
                                  quint16 classId,
                                  quint16 methodId)
{
    m_closeReason.code = code;
    m_closeReason.replyText = replyText;
    m_closeReason.classId = classId;
    m_closeReason.methodId = methodId;
    m_closeReason.isServerInitiated = false;

    const QVariantList args({QVariant::fromValue(code),
                             replyText,
                             QVariant::fromValue(classId),
                             QVariant::fromValue(methodId)});
    MethodFrame frame(channel0, spec::connection::ID_, spec::connection::Close);
    qDebug() << "Set connection.close frame args" << args;
    frame.setArguments(args);
    return m_client->sendFrame(frame);
}

bool ConnectionHandler::onCloseOk(const MethodFrame &frame)
{
    Q_UNUSED(frame);
    qDebug() << "CloseOk received";
    this->stopHeartbeat();
    emit connectionClosed(m_closeReason.code,
                          m_closeReason.replyText,
                          m_closeReason.classId,
                          m_closeReason.methodId);
    return true;
}

bool ConnectionHandler::startHeartbeat()
{
    this->m_lastheartbeatReceived = QDateTime::currentDateTimeUtc();
    if (this->m_heartbeatSeconds <= 0) {
        return true;
    }
    if (this->m_heartbeatTimer == nullptr) {
        this->m_heartbeatTimer = new QTimer(this);
        // To help avoid timeouts, we send heartbeats at twice the min.
        this->m_heartbeatTimer->setInterval(int(this->m_heartbeatSeconds) * (1000 / 2));
        connect(this->m_heartbeatTimer,
                &QTimer::timeout,
                this,
                &ConnectionHandler::onHeartbeatTimer);
    }
    this->m_heartbeatTimer->start();
    return true;
}

void ConnectionHandler::stopHeartbeat()
{
    if (this->m_heartbeatTimer != nullptr) {
        this->m_heartbeatTimer->deleteLater();
        this->m_heartbeatTimer = nullptr;
    }
}

void ConnectionHandler::onHeartbeatTimer()
{
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    // This follows RabbitMq - "After two missed heartbeats, the peer is considered to be unreachable."
    if (this->m_lastheartbeatReceived.secsTo(nowUtc) > (this->m_heartbeatSeconds * 2)) {
        qWarning() << "Missed heartbeats from server: last receieved at"
                   << this->m_lastheartbeatReceived << "now:" << nowUtc
                   << "heartbeat:" << this->m_heartbeatSeconds << "s";
        this->m_client->disconnectFromHost(500, "Missed heartbeats");
    }
    this->m_client->sendHeartbeat();
}

void ConnectionHandler::resetTrafficFromServerHeartbeat()
{
    this->m_lastheartbeatReceived = QDateTime::currentDateTimeUtc();
}
} // namespace detail
} // namespace qmq
