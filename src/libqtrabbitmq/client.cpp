#include <qtrabbitmq/channel.h>
#include <qtrabbitmq/client.h>

#include "connection_handler.h"
#include "spec_constants.h"

#include <QByteArray>
#include <QScopedPointer>
#include <QSslSocket>
#include <QString>
#include <QUrl>

namespace {
constexpr const int defaultPort = 5672;
constexpr const int defaultSslPort = 5673;
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
    QScopedPointer<detail::ConnectionHandler> connection;
    quint32 maxFrameSizeBytes = 1024 * 1024;
    QHash<quint16, QSharedPointer<qmq::Channel>> channels;
    quint16 nextChannelId = 1;
    quint16 maxChannelId = 100;
};

Client::Client(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
    d->connection.reset(new detail::ConnectionHandler(this));
}

Client::~Client()
{
    d.reset();
}

QUrl Client::connectionUrl() const
{
    return d->url;
}

void Client::connectToHost(const QUrl &url)
{
    bool useSsl = false;
    QString vhost = "/";
    int port = defaultPort;

    if (url.scheme() == amqpScheme) {
    } else if (url.scheme() == amqpSslScheme) {
        useSsl = true;
        port = defaultSslPort;
    } else if (!url.scheme().isEmpty()) {
        qWarning() << "unknown scheme in URL";
        return;
    }

    if (url.host().isEmpty()) {
        qWarning() << "invalid host in URL";
        return;
    }

    if (!url.path().isEmpty()) {
        vhost = url.path();
    }

    port = url.port(port);

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

    if (useSsl) {
        d->socket->connectToHostEncrypted(url.host(), port);
    } else {
        d->socket->connectToHost(url.host(), port);
    }
}

QString Client::virtualHost() const
{
    return d->vhost;
}

bool Client::sendFrame(const Frame *f)
{
    const quint32 maxFrameSize = d->maxFrameSizeBytes;
    return Frame::writeFrame(d->socket, maxFrameSize, f);
}

void Client::onSocketConnected()
{
    qDebug() << "Connected, writing header";
    d->socket->write(protocolHeader());
}

void Client::onSocketReadyRead()
{
    qDebug() << "Ready read";
    const quint32 maxFrameSize = 64 * 1024; // todo
    ErrorCode errCode = qmq::ErrorCode::NoError;
    QScopedPointer<Frame> frame(Frame::readFrame(d->socket, maxFrameSize, &errCode));
    if (frame) {
        qDebug() << "Read frame" << (int) frame->type();
    } else {
        qDebug() << "No frame" << (int) errCode << d->socket->bytesAvailable();
        return;
    }
    switch (frame->type()) {
    case FrameType::Method:
        qDebug() << "Method frame";
        {
            const MethodFrame *methodFr = static_cast<MethodFrame *>(frame.get());
            switch (methodFr->classId()) {
            case spec::connection::ID_:
                d->connection->handleFrame(methodFr);
                break;
            case spec::channel::ID_: {
                auto handler = d->channels.value(methodFr->channel());
                if (handler) {
                    handler->handleFrame(methodFr);
                } else {
                    qWarning() << "No handler for channel" << methodFr->channel();
                }
            } break;
            default:
                qWarning() << "unhandled methd frame";
                break;
            }
        }
        break;
    case FrameType::Header:
        qDebug() << "Header frame";
        break;
    case FrameType::Body:
        qDebug() << "body frame";
        break;
    case FrameType::Heartbeat:
        qDebug() << "Heartbeat frame";
        break;
    default:
        qWarning() << "Unknown frame type" << (int) frame->type();
        break;
    }

#warning(TODO)
}

void Client::disconnectFromHost()
{
    if (!d->socket) {
        qWarning() << "Already disconnected";
        return;
    }
    d->socket->disconnectFromHost();
}

QSharedPointer<Channel> Client::createChannel()
{
    for (int i = 0; i < d->maxChannelId; i++) {
        const quint16 channelId = d->nextChannelId++;
        if (d->nextChannelId > d->maxChannelId) {
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
    return this->sendFrame(&frame);
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
