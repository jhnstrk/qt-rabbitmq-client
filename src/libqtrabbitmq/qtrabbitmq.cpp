#include <qtrabbitmq/qtrabbitmq.h>

#include "frame_handlers.h"
#include "spec_constants.h"

#include <QSslError>
#include <QSslSocket>
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
    QScopedPointer<detail::ConnectionHandler> connection;
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

bool Client::sendFrame(detail::Frame *f)
{
    const quint32 maxFrameSize = 64 * 1024; // todo
    return detail::Frame::writeFrame(d->socket, maxFrameSize, f);
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
    detail::ErrorCode errCode;
    QScopedPointer<detail::Frame> frame(detail::Frame::readFrame(d->socket, maxFrameSize, &errCode));
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
            const detail::MethodFrame *mf = static_cast<detail::MethodFrame *>(frame.get());
            switch (mf->classId()) {
            case (int) Connection::ID_:
                d->connection->handleFrame(mf);
                break;
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

void Client::onSocketSslErrors(const QList<QSslError> &errors)
{
#warning(TODO)
    qDebug() << "SSL errors" << errors;
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

#include <qtrabbitmq.moc>
