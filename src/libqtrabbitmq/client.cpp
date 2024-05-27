#include <qtrabbitmq/channel.h>
#include <qtrabbitmq/client.h>

#include "connection_handler.h"
#include "spec_constants.h"

#include <QByteArray>
#include <QSslSocket>
#include <QString>
#include <QTimer>
#include <QUrl>

#include <memory>

namespace {
constexpr const quint16 defaultPort = 5672;
constexpr const quint16 defaultSslPort = 5673;
constexpr const char *const amqpScheme = "amqp";
constexpr const char *const amqpSslScheme = "amqps";

QByteArray protocolHeader()
{
    return QByteArrayLiteral("AMQP\x00\x00\x09\x01");
}
} // namespace

namespace qmq {

class Client::Private
{
public:
    QSslSocket *socket = nullptr;
    QUrl url;
    QString vhost;
    QString userName;
    QString password;
    QSharedPointer<detail::ConnectionHandler> connection;
    quint32 maxFrameSizeBytes = 1024 * 1024;
    QHash<quint16, QSharedPointer<qmq::Channel>> channels;
    quint16 nextChannelId = 1;
    quint16 maxChannelId = 2047;
    quint16 heartbeatSeconds = 60;
    ConnectionState state = ConnectionState::Closed;
};

Client::Client(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
    d->connection.reset(new detail::ConnectionHandler(this));
    connect(d->connection.data(),
            &detail::ConnectionHandler::connectionOpened,
            this,
            &Client::connected);
    connect(d->connection.data(),
            &detail::ConnectionHandler::connectionClosed,
            this,
            &Client::disconnected);
}

Client::~Client()
{
    d.reset();
}

QString Client::username() const
{
    return d->userName;
}

QString Client::password() const
{
    return d->password;
}

QUrl Client::connectionUrl() const
{
    return d->url;
}

quint32 Client::maxFrameSizeBytes() const
{
    if (d->connection) {
        return d->connection->maxFrameSizeBytes();
    }
    return d->maxFrameSizeBytes;
}

void Client::setMaxFrameSizeBytes(quint32 value)
{
    d->maxFrameSizeBytes = value;
}

quint16 Client::maxChannelId() const
{
    return d->connection->maxChannelId();
}

void Client::setMaxChannelId(quint16 mx)
{
    d->maxChannelId = mx;
}

quint16 Client::heartbeatSeconds() const
{
    return d->connection->heartbeatSeconds();
}

void Client::setHeartbeatSeconds(quint16 n)
{
    d->heartbeatSeconds = n;
}

bool Client::connectToHost(const QUrl &url)
{
    if (d->connection->state() != ConnectionState::Closed) {
        qWarning() << "Connecting while not in Closed state";
        this->disconnectFromHost();
    }
    d->connection->setTuneParameters(d->maxChannelId, d->maxFrameSizeBytes, d->heartbeatSeconds);

    bool useSsl = false;
    QString vhost = "/";
    quint16 port = defaultPort;

    if (url.scheme() == amqpScheme) {
    } else if (url.scheme() == amqpSslScheme) {
        useSsl = true;
        port = defaultSslPort;
    } else if (!url.scheme().isEmpty()) {
        qWarning() << "unknown scheme in URL";
        return false;
    }

    if (url.host().isEmpty()) {
        qWarning() << "invalid host in URL";
        return false;
    }

    if (!url.path().isEmpty()) {
        vhost = url.path();
    }

    port = static_cast<quint16>(url.port(port));

    d->url = url;
    d->vhost = vhost;
    d->userName = url.userName();
    d->password = url.password();

    d->socket = new QSslSocket(this);
    d->socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    d->socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    connect(d->socket, &QAbstractSocket::connected, this, &Client::onSocketConnected);
    connect(d->socket, &QAbstractSocket::readyRead, this, &Client::onSocketReadyRead);
    connect(d->socket, &QAbstractSocket::errorOccurred, this, &Client::onSocketErrorOccurred);
    connect(d->socket, &QAbstractSocket::stateChanged, this, &Client::onSocketStateChanged);
    connect(d->socket, &QSslSocket::sslErrors, this, &Client::onSocketSslErrors);

    d->state = ConnectionState::Opening;
    if (useSsl) {
        d->socket->connectToHostEncrypted(url.host(), port);
    } else {
        d->socket->connectToHost(url.host(), port);
    }
    return true;
}

QString Client::virtualHost() const
{
    return d->vhost;
}

bool Client::sendFrame(const Frame &frame)
{
    const quint32 maxFrameSize = this->maxFrameSizeBytes();
    return Frame::writeFrame(d->socket, maxFrameSize, frame);
}

void Client::onSocketConnected()
{
    qDebug() << "Connected, writing header";
    d->socket->write(protocolHeader());
}

void Client::onSocketReadyRead()
{
    qDebug() << "Ready read";
    const quint32 maxFrameSize = d->connection->maxFrameSizeBytes();
    ErrorCode errCode = qmq::ErrorCode::NoError;
    std::unique_ptr<Frame> frame(Frame::readFrame(d->socket, maxFrameSize, &errCode));
    if (frame) {
        qDebug() << "Read frame with type=" << (int) frame->type();
    } else {
        qDebug() << "No frame" << (int) errCode << d->socket->bytesAvailable();
        return;
    }
    QSharedPointer<AbstractFrameHandler> handler;
    if (frame->channel() == 0) {
        handler = d->connection;
    } else {
        handler = d->channels.value(frame->channel());
    }
    if (!handler) {
        qWarning() << "No handler for channel" << frame->channel();
        return;
    }

    bool isHandled = false;
    switch (frame->type()) {
    case FrameType::Method: {
        qDebug() << "Method frame on channel" << frame->channel();
        const MethodFrame &methodFr = static_cast<const MethodFrame &>(*frame);
        isHandled = handler->handleMethodFrame(methodFr);
    } break;
    case FrameType::Header:
        qDebug() << "Header frame on channel" << frame->channel();
        isHandled = handler->handleHeaderFrame(static_cast<const HeaderFrame &>(*frame.get()));
        break;
    case FrameType::Body:
        qDebug() << "Body frame on channel" << frame->channel();
        isHandled = handler->handleBodyFrame(static_cast<const BodyFrame &>(*frame.get()));
        break;
    case FrameType::Heartbeat:
        qDebug() << "Heartbeat frame on channel" << frame->channel();
        isHandled = handler->handleHeartbeatFrame(static_cast<const HeartbeatFrame &>(*frame.get()));
        break;
    default:
        qWarning() << "Unknown frame type" << (int) frame->type();
        break;
    }

    // Any activity should reset the heartbeat timeout.
    d->connection->resetTrafficFromServerHeartbeat();

    if (d->socket->bytesAvailable() > 0) {
        qDebug() << "More data to come";
        // Allow event loop to do some work if needed.
        QTimer::singleShot(std::chrono::milliseconds(0), this, &Client::onSocketReadyRead);
    }
    if (!isHandled) {
        qWarning() << "Unhandled frame type" << (int) frame->type() << "on channel"
                   << frame->channel();
    }
}

void Client::disconnectFromHost(quint16 code,
                                const QString &replyText,
                                quint16 classId,
                                quint16 methodId)
{
    if (d->socket == nullptr) {
        qWarning() << "Already disconnected";
        return;
    }
    d->connection->sendClose(code, replyText, classId, methodId);
}

QSharedPointer<Channel> Client::createChannel()
{
    const int maxChannelId = d->connection->maxChannelId();
    for (int i = 0; i < maxChannelId; i++) {
        const quint16 channelId = d->nextChannelId++;
        if (d->nextChannelId > maxChannelId) {
            d->nextChannelId = 1;
        }
        if (!d->channels.contains(channelId)) {
            QSharedPointer<Channel> handler(new Channel(this, channelId));
            d->channels[channelId] = handler;
            return handler.staticCast<Channel>();
        }
    }
    qWarning() << "No free channel id available to create channel";
    return nullptr;
}

void Client::onSocketSslErrors(const QList<QSslError> &errors)
{
#warning(TODO)
    qWarning() << "SSL errors" << errors;
}

bool Client::sendHeartbeat()
{
    qmq::HeartbeatFrame frame;
    return this->sendFrame(frame);
}

void Client::onSocketStateChanged(QAbstractSocket::SocketState state)
{
#warning(TODO)
    qDebug() << "onSocketStateChanged" << state;
}

void Client::onSocketErrorOccurred(QAbstractSocket::SocketError error)
{
#warning(TODO)
    qDebug() << "onSocketErrorOccurred" << error << d->socket->errorString();
}

} // namespace qmq

#include <client.moc>
