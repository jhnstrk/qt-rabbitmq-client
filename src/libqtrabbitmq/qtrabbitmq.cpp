#include <qtrabbitmq/qtrabbitmq.h>

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
QByteArray frameEnd()
{
    return QByteArrayLiteral("\xCE");
}
} // namespace

namespace qmq {

class Client::Private
{
public:
    QSslSocket *socket = nullptr;
    QUrl url;
};

Client::Client(QObject *parent)
    : QObject(parent)
    , d(new Private)
{}

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

void Client::onSocketConnected()
{
    qDebug() << "Connected, writing header";
    d->socket->write(protocolHeader());
}

void Client::onSocketReadyRead()
{
    qDebug() << "Ready read";
    const int minimumFrameSize = 8;
    if (d->socket->bytesAvailable() < minimumFrameSize) {
        return;
    }
#warning(TODO)
}

void Client::onSocketSslErrors(const QList<QSslError> &errors)
{
#warning(TODO)
}

void Client::onSocketStateChanged(QAbstractSocket::SocketState state)
{
#warning(TODO)
}

void Client::onSocketErrorOccurred(QAbstractSocket::SocketError error)
{
#warning(TODO)
}

} // namespace qmq

#include <qtrabbitmq.moc>
