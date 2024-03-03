#include "spec_constants.h"
#include <qtrabbitmq/channel.h>
#include <qtrabbitmq/client.h>
#include <qtrabbitmq/exception.h>

#include <QUuid>

namespace {
QDebug operator<<(QDebug debug, const QHash<qmq::BasicProperty, QVariant> &h)
{
    for (auto it(h.constBegin()); it != h.constEnd(); ++it) {
        debug = (debug << qmq::basicPropertyName(it.key()) << it.value());
    }
    return debug;
}
QString exchangeTypeToString(qmq::Channel::ExchangeType t)
{
    switch (t) {
    case qmq::Channel::ExchangeType::Fanout:
        return "fanout";
    case qmq::Channel::ExchangeType::Direct:
        return "direct";
    case qmq::Channel::ExchangeType::Match:
        return "match";
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

struct IncomingMessage
{
    QHash<qmq::BasicProperty, QVariant> m_properties;
    quint64 m_contentSize = 0;
    QByteArray m_payload;
    QString m_consumerTag;
    qlonglong m_deliveryTag = 0;
    bool m_redelivered = false;
    QString m_exchangeName;
    QString m_routingKey;
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
        qWarning() << "No message found for classID" << classId << methodId;
        return MessageItemPtr();
    }
    qint16 channelId = 0;
    Client *client = nullptr;
    QList<MessageItemPtr> inFlightMessages;
    QScopedPointer<IncomingMessage> deliveringMessage;
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

bool Channel::handleMethodFrame(const MethodFrame *frame)
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
            return this->onExchangeDeclareOk(frame);
        default:
            qWarning() << "Unknown exchange frame" << frame->methodId();
            break;
        }
        break;
    case qmq::spec::queue::ID_:
        switch (frame->methodId()) {
        case qmq::spec::queue::DeclareOk:
            return this->onQueueDeclareOk(frame);
        case qmq::spec::queue::BindOk:
            return this->onQueueBindOk(frame);
        default:
            qWarning() << "Unknown queue frame" << frame->methodId();
            break;
        }
        break;
    case qmq::spec::basic::ID_:
        switch (frame->methodId()) {
        case qmq::spec::basic::ConsumeOk:
            return this->onBasicConsumeOk(frame);
        case qmq::spec::basic::Deliver:
            return this->onBasicDeliver(frame);
        default:
            qWarning() << "Unknown basic frame" << frame->methodId();
            break;
        }
        break;
    default:
        qWarning() << "Unknown frame class" << frame->classId() << "methodId:" << frame->methodId();
        break;
    }
    return false;
}

bool Channel::handleHeaderFrame(const HeaderFrame *frame)
{
    if (!d->deliveringMessage) {
        qWarning() << "Header frame unexpected with classID" << frame->classId();
        return false;
    }

    qDebug() << "Header with properties" << frame->properties();
    const quint64 messageSize = frame->contentSize();
    d->deliveringMessage->m_properties = frame->properties();
    d->deliveringMessage->m_contentSize = frame->contentSize();
    d->deliveringMessage->m_payload.reserve(messageSize);
    if (messageSize == 0) {
        this->incomingMessageComplete();
    }
}

bool Channel::handleBodyFrame(const BodyFrame *frame)
{
    qDebug() << "Body frame with" << frame->content().size() << "bytes";
    if (!d->deliveringMessage) {
        qWarning() << "Body frame unexpected";
        return false;
    }
    d->deliveringMessage->m_payload.append(frame->content());
    if (d->deliveringMessage->m_payload.size() >= d->deliveringMessage->m_contentSize) {
        this->incomingMessageComplete();
    }
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
    bool ok = false;
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
    if (!isOk) {
        messageTracker->promise.setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->promise.finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
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

    if (!isOk) {
        messageTracker->promise.setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->promise.finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
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

    if (!isOk) {
        messageTracker->promise.setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->promise.finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
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

QFuture<void> Channel::consume(const QString &queueName)
{
    MethodFrame frame(d->channelId, spec::basic::ID_, spec::basic::Consume);
    MessageItemPtr messageTracker(new MessageItem(frame.classId(), frame.methodId()));

    // Short, QueueName, ConsumerTag, NoLocal, NoAck, Bit, NoWait, Table
    const short reserved1 = 0;
    const QString consumerTag = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);
    const bool noLocal = false;
    const bool noAck = false;
    const bool exclusive = false;
    const bool noWait = false;
    const QVariantHash consumeArguments;
    const QVariantList args(
        {reserved1, queueName, consumerTag, noLocal, noAck, exclusive, noWait, consumeArguments});
    qDebug() << "Set consume method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->promise.setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->promise.finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onBasicConsumeOk(const MethodFrame *frame)
{
    qDebug() << "Basic Consume OK received";
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::basic::ID_, spec::basic::Consume));

    if (messageTracker) {
        messageTracker->promise.finish();
    }
    return true;
}

QFuture<void> Channel::publish(const QString &exchangeName, const qmq::Message &message)
{
    MethodFrame frame(d->channelId, spec::basic::ID_, spec::basic::Publish);
    MessageItemPtr messageTracker(new MessageItem(frame.classId(), frame.methodId()));

    // Short, ExchangeName, ShortStr, Bit, Bit
    const short reserved1 = 0;
    const QString routingKey;
    const bool mandatory = false;
    const bool immediate = false;
    const QVariantHash consumeArguments;
    const QVariantList args({reserved1, exchangeName, routingKey, mandatory, immediate});
    qDebug() << "Set publish method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->promise.setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->promise.finish();
        return messageTracker->promise.future();
    }

    // d->inFlightMessages.push_back(messageTracker);
    const QByteArray payload = message.payload();
    HeaderFrame header(d->channelId, frame.classId(), payload.size(), message.properties());
    isOk = d->client->sendFrame(&header);

    qint64 writtenBytes = 0;
    const qint64 maxFrameSize = 1024 * 1024; // TODO.
    const qint64 maxPayloadsize = maxFrameSize - 8;
    while (writtenBytes < payload.size()) {
        const qint64 len = std::min(maxPayloadsize, payload.size() - writtenBytes);
        const QByteArray part = payload.mid(writtenBytes, len);
        BodyFrame body(d->channelId, part);
        isOk = d->client->sendFrame(&body);
        writtenBytes += len;
    }
    messageTracker->promise.finish();
    return messageTracker->promise.future();
}

bool Channel::sendAck(qlonglong deliveryTag, bool muliple)
{
    MethodFrame frame(d->channelId, spec::basic::ID_, spec::basic::Ack);

    QVariantList args({deliveryTag, muliple});
    qDebug() << "Set ack method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        qWarning() << "Failed to send frame";
    }

    return isOk;
}

bool Channel::onBasicDeliver(const MethodFrame *frame)
{
    qDebug() << "Deliver received";
    bool isOk = false;
    // {ConsumerTag, DeliveryTag, Redelivered, ExchangeName, ShortStr};
    const QVariantList args = frame->getArguments(&isOk);
    if (!isOk) {
        qWarning() << "Failed to get arguments";
    }
    const QString consumerTag = args.at(0).toString();
    const qlonglong deliveryTag = args.at(1).toLongLong();
    const bool redelivered = args.at(2).toBool();
    const QString exchangeName = args.at(3).toString();
    const QString routingKey = args.at(4).toString();

    d->deliveringMessage.reset(new IncomingMessage);
    d->deliveringMessage->m_consumerTag = consumerTag;
    d->deliveringMessage->m_deliveryTag = deliveryTag;
    d->deliveringMessage->m_exchangeName = exchangeName;
    d->deliveringMessage->m_redelivered = redelivered;
    d->deliveringMessage->m_routingKey = routingKey;

    return true;
}

void Channel::incomingMessageComplete()
{
    qDebug() << "Message complete with delivery tag" << d->deliveringMessage->m_deliveryTag
             << "and size" << d->deliveringMessage->m_payload.size();
    qDebug() << "payload" << QString::fromUtf8(d->deliveringMessage->m_payload);
    qmq::Message msg(d->deliveringMessage->m_properties, d->deliveringMessage->m_payload);
    msg.setPayload(d->deliveringMessage->m_payload);
}
} // namespace qmq

#include <channel.moc>
