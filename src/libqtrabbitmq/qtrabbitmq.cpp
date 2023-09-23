#include <qtrabbitmq/qtrabbitmq.h>

#include <QUrl>

namespace qmq {

class Client::Private
{
public:
    QUrl url;
};

Client::Client(QObject *parent)
    : QObject(parent)
    , d(new Private)
{}

Client::~Client()
{
    delete d;
    d = nullptr;
}

QUrl Client::connectionUrl() const
{
    return d->url;
}

void Client::setConnectionUrl(const QUrl &url)
{
    d->url = url;
}
} // namespace qmq

#include <qtrabbitmq.moc>
