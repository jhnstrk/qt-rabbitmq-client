#include <qtrabbitmq/consumer.h>

#include <QQueue>
#include <QUuid>

#include <qtrabbitmq/channel.h>

namespace qmq {
class Consumer::Private
{
public:
    QString consumerTag;
    QQueue<qmq::Message> messageQueue;
};

Consumer::Consumer(const QString &consumerTag, QObject *parent)
    : QObject(parent)
    , d(new Private)
{
    d->consumerTag = consumerTag;
    if (d->consumerTag.isEmpty()) {
        d->consumerTag = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);
    }
}

Consumer::~Consumer() = default;

QString Consumer::consumerTag() const
{
    return d->consumerTag;
}

void Consumer::pushMessage(const qmq::Message &msg)
{
    d->messageQueue.enqueue(msg);
    emit this->messageReady();
}

bool Consumer::hasMessage() const
{
    return !d->messageQueue.isEmpty();
}

QFuture<QString> Consumer::consume(Channel *channel, const QString &queueName)
{
    channel->addConsumer(this);
    return channel->consume(queueName, this->consumerTag());
}

Message Consumer::dequeueMessage()
{
    return d->messageQueue.dequeue();
}
} // namespace qmq