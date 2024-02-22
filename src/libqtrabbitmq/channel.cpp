#include "spec_constants.h"
#include <qtrabbitmq/channel.h>
#include <qtrabbitmq/client.h>
#include <qtrabbitmq/exception.h>

namespace {
QString exchangeTypeToString(qmq::Channel::ExchangeType t)
{
    switch (t) {
    case qmq::Channel::ExchangeType::Fanout:
        return "fanout";
    case qmq::Channel::ExchangeType::Direct:
        return "direct";
    case qmq::Channel::ExchangeType::Headers:
        return "headers";
    case qmq::Channel::ExchangeType::Topic:
        return "topic";
    default:
        qWarning() << "Unknown exchange type" << (int) t;
        return QString();
    }
}

struct MessageItem
{
    MessageItem(qint16 cid, qint16 mid)
        : classId(cid)
        , methodId(mid)
    {}

    qint16 classId = 0;
    qint16 methodId = 0;
    QPromise<void> promise;
};

typedef QSharedPointer<MessageItem> MessageItemPtr;
} // namespace

namespace qmq {

class Channel::Private
{
public:
    MessageItemPtr popFirstMessageItem(qint16 classId, qint16 methodId)
    {
        for (auto it = inFlightMessages.begin(); it != inFlightMessages.end(); ++it) {
            const MessageItemPtr ptr = *it;
            if (ptr->methodId == methodId && ptr->classId == classId) {
                inFlightMessages.erase(it);
                return *it;
            }
        }
        return MessageItemPtr();
    }
    qint16 channelId = 0;
    Client *client = nullptr;
    QList<MessageItemPtr> inFlightMessages;
};

Channel::Channel(Client *client, qint16 channelId)
    : d(new Private)
{
    d->channelId = channelId;
    d->client = client;
}

Channel::~Channel() {}

int Channel::channelId() const
{
    return d->channelId;
}

bool Channel::handleFrame(const MethodFrame *frame)
{
    // Q_ASSERT(frame->classId() == static_cast<quint16>(qmq::spec::channel::ID_));
    Q_ASSERT(frame->channel() == this->channelId());
    switch (frame->classId()) {
    case qmq::spec::channel::ID_:
        switch (frame->methodId()) {
        case spec::channel::OpenOk:
            return this->onOpenOk(frame);
        case spec::channel::CloseOk:
            return this->onCloseOk(frame);
        default:
            qWarning() << "Unknown channel frame" << frame->methodId();
            break;
        }
        break;
    case qmq::spec::exchange::ID_:
        switch (frame->methodId()) {
        case qmq::spec::exchange::DeclareOk:
            this->onExchangeDeclareOk(frame);
        default:
            qWarning() << "Unknown exchange frame" << frame->methodId();
            break;
        }
        break;
    case qmq::spec::queue::ID_:
        switch (frame->methodId()) {
        case qmq::spec::queue::DeclareOk:
            this->onQueueDeclareOk(frame);
        case qmq::spec::queue::BindOk:
            this->onQueueBindOk(frame);
        default:
            qWarning() << "Unknown queue frame" << frame->methodId();
            break;
        }
        break;
    default:
        qWarning() << "Unknown frame class" << frame->classId();
        break;
    }
    return false;
}

QFuture<void> Channel::openChannel()
{
    MessageItemPtr messageTracker(new MessageItem(spec::channel::ID_, spec::channel::Open));
    messageTracker->promise.start();
    const QString reserved1; // out-of-band
    QVariantList args({reserved1});
    MethodFrame frame(d->channelId, spec::channel::ID_, spec::channel::Open);
    qDebug() << "Set open channel method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);
    if (!isOk) {
        messageTracker->promise.setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->promise.finish();
    } else {
        d->inFlightMessages.push_back(messageTracker);
    }
    return messageTracker->promise.future();
}

bool Channel::onOpenOk(const MethodFrame *frame)
{
    bool ok;
    const QVariantList args = frame->getArguments(&ok);
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::channel::ID_, spec::channel::Open));
    if (!ok) {
        qWarning() << "Failed to parse args";
        if (messageTracker) {
            messageTracker->promise.setException(qmq::Exception(1, "Failed to parse OpenOk frame"));
            messageTracker->promise.finish();
        }
        return false;
    }

    qDebug() << "channel::OpenOk" << args;
    if (messageTracker) {
        messageTracker->promise.finish();
    }
    return true;
}

QFuture<void> Channel::closeChannel(qint16 code,
                                    const QString &replyText,
                                    quint16 classId,
                                    quint16 methodId)
{
    MessageItemPtr messageTracker(new MessageItem(spec::channel::ID_, spec::channel::Close));
    QVariantList args({code, replyText, classId, methodId});
    MethodFrame frame(d->channelId, spec::channel::ID_, spec::channel::Close);
    qDebug() << "Set close frame args" << args;
    frame.setArguments(args);
    messageTracker->promise.start();
    if (!d->client->sendFrame(&frame)) {
        messageTracker->promise.setException(qmq::Exception(1, "Failed to send close frame"));
        messageTracker->promise.finish();
    } else {
        d->inFlightMessages.push_back(messageTracker);
    }
    return messageTracker->promise.future();
}

bool Channel::onCloseOk(const MethodFrame *)
{
    qDebug() << "CloseOk received";
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::channel::ID_, spec::channel::Close));
    if (messageTracker) {
        messageTracker->promise.finish();
    }
    return true;
}

QFuture<void> Channel::declareExchange(const QString &exchangeName,
                                       ExchangeType type,
                                       const DeclareExchangeOptions &opts)
{
    MessageItemPtr messageTracker(new MessageItem(spec::exchange::ID_, spec::exchange::Declare));
    MethodFrame frame(d->channelId, spec::exchange::ID_, spec::exchange::Declare);
    const short reserved1 = 0;
    const QString exchangeType = exchangeTypeToString(type);
    const bool reserved2 = false;
    const bool reserved3 = false;

    QVariantList args({reserved1,
                       exchangeName,
                       exchangeType,
                       opts.passive,
                       opts.durable,
                       reserved2,
                       reserved3,
                       opts.noWait,
                       opts.arguments});
    qDebug() << "Set declare exchange method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);
    QSharedPointer<QPromise<void>> newPromise(new QPromise<void>());
    if (!isOk) {
        newPromise->setException(qmq::Exception(1, "Failed to send frame"));
        newPromise->finish();
        return newPromise->future();
    } else {
        d->inFlightMessages.push_back(messageTracker);
    }

    return newPromise->future();
}

bool Channel::onExchangeDeclareOk(const MethodFrame *frame)
{
    qDebug() << "Declare Exchange OK received";
    MessageItemPtr messageTracker(
        d->popFirstMessageItem(spec::exchange::ID_, spec::exchange::Declare));

    if (messageTracker) {
        messageTracker->promise.finish();
    }
    return true;
}

QFuture<void> Channel::declareQueue(const QString &queueName, const DeclareQueueOptions &opts)
{
    MethodFrame frame(d->channelId, spec::queue::ID_, spec::queue::Declare);
    MessageItemPtr messageTracker(new MessageItem(frame.classId(), frame.methodId()));

    const short reserved1 = 0;
    QVariantList args({reserved1,
                       queueName,
                       opts.passive,
                       opts.durable,
                       opts.exclusive,
                       opts.autoDelete,
                       opts.noWait,
                       opts.arguments});
    qDebug() << "Set declare queue method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);
    QSharedPointer<QPromise<void>> newPromise(new QPromise<void>());
    if (!isOk) {
        newPromise->setException(qmq::Exception(1, "Failed to send frame"));
        newPromise->finish();
        return newPromise->future();
    } else {
        d->inFlightMessages.push_back(messageTracker);
    }

    return newPromise->future();
}

bool Channel::onQueueDeclareOk(const MethodFrame *frame)
{
    qDebug() << "Declare Queue OK received";
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::queue::ID_, spec::queue::Declare));

    const QVariantList args = frame->getArguments();
    qDebug() << "Name, messageCount, consumerCount" << args;
    if (messageTracker) {
        messageTracker->promise.finish();
    }
    return true;
}

QFuture<void> Channel::bindQueue(const QString &queueName, const QString &exchangeName)
{
    MethodFrame frame(d->channelId, spec::queue::ID_, spec::queue::Bind);
    MessageItemPtr messageTracker(new MessageItem(frame.classId(), frame.methodId()));

    const short reserved1 = 0;
    const QString routingKey;
    const bool noWait = false;
    const QVariantHash bindArguments;
    const QVariantList args({reserved1, queueName, exchangeName, routingKey, noWait, bindArguments});
    qDebug() << "Set bind queue method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);
    QSharedPointer<QPromise<void>> newPromise(new QPromise<void>());
    if (!isOk) {
        newPromise->setException(qmq::Exception(1, "Failed to send frame"));
        newPromise->finish();
        return newPromise->future();
    } else {
        d->inFlightMessages.push_back(messageTracker);
    }

    return newPromise->future();
}

bool Channel::onQueueBindOk(const MethodFrame *frame)
{
    qDebug() << "Bind Queue OK received";
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::queue::ID_, spec::queue::Bind));

    if (messageTracker) {
        messageTracker->promise.finish();
    }
    return true;
}

} // namespace qmq

#include <channel.moc>
