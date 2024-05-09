#include "spec_constants.h"
#include <qtrabbitmq/channel.h>
#include <qtrabbitmq/client.h>
#include <qtrabbitmq/consumer.h>
#include <qtrabbitmq/exception.h>

#include <QUuid>

namespace {
constexpr const quint64 MAX_MESSAGE_SIZE = 10 * 1024 * 1024;

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
        return {};
    }
}

class MessageItem
{
public:
    MessageItem(quint16 cid, quint16 mid)
        : classId(cid)
        , methodId(mid)
    {}
    virtual ~MessageItem() = default;

    virtual void start() = 0;
    virtual void finish() = 0;
    virtual void setException(const QException &) = 0;
    quint16 classId = 0;
    quint16 methodId = 0;
};

template<class T>
class MessagePromise : public MessageItem
{
public:
    MessagePromise(quint16 cid, quint16 mid)
        : MessageItem(cid, mid)
    {}
    void start() override { promise.start(); };
    void finish() override { promise.finish(); };
    void setException(const QException &exc) override { promise.setException(exc); }

    QPromise<T> promise;
};

struct IncomingMessage
{
    QHash<qmq::BasicProperty, QVariant> m_properties;
    quint64 m_contentSize = 0;
    QByteArray m_payload;
    QString m_consumerTag;
    quint64 m_deliveryTag = 0;
    bool m_redelivered = false;
    bool m_isGet = false;       // Is the message coming from a (synchronous) Get operation?
    quint32 m_messageCount = 0; // Message count is passed with Get.
    QString m_exchangeName;
    QString m_routingKey;
};

using MessageItemPtr = QSharedPointer<MessageItem>;
using MessageItemVoidPtr = QSharedPointer<MessagePromise<void>>;
using MessageVlistPtr = QSharedPointer<MessagePromise<QVariantList>>;
using MessageIntPtr = QSharedPointer<MessagePromise<int>>;
using MessageStrPtr = QSharedPointer<MessagePromise<QString>>;

template<class T>
QSharedPointer<MessagePromise<T>> getPromise(MessageItemPtr &ptr)
{
    if (!ptr) {
        return QSharedPointer<MessagePromise<T>>();
    }
    return ptr.dynamicCast<MessagePromise<T>>();
}
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
        return {};
    }
    quint16 channelId = 0;
    Client *client = nullptr;
    QList<MessageItemPtr> inFlightMessages;
    QScopedPointer<IncomingMessage> deliveringMessage;
    QHash<QString, QPointer<Consumer>> consumers;
};

Channel::Channel(Client *client, quint16 channelId)
    : d(new Private)
{
    d->channelId = channelId;
    d->client = client;
}

Channel::~Channel() = default;

int Channel::channelId() const
{
    return d->channelId;
}

bool Channel::handleMethodFrame(const MethodFrame *frame)
{
    Q_ASSERT(frame->channel() == this->channelId());
    switch (frame->classId()) {
    case qmq::spec::channel::ID_:
        switch (frame->methodId()) {
        case spec::channel::OpenOk:
            return this->onChannelOpenOk(frame);
        case spec::channel::Flow:
            return this->onChannelFlow(frame);
        case spec::channel::FlowOk:
            return this->onChannelFlowOk(frame);
        case spec::channel::Close:
            return this->onChannelClose(frame);
        case spec::channel::CloseOk:
            return this->onChannelCloseOk(frame);
        default:
            qWarning() << "Unknown channel frame" << frame->methodId();
            break;
        }
        break;
    case qmq::spec::exchange::ID_:
        switch (frame->methodId()) {
        case qmq::spec::exchange::DeclareOk:
            return this->onExchangeDeclareOk(frame);
        case qmq::spec::exchange::DeleteOk:
            return this->onExchangeDeleteOk(frame);
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
        case qmq::spec::queue::UnbindOk:
            return this->onQueueUnbindOk(frame);
        case qmq::spec::queue::PurgeOk:
            return this->onQueuePurgeOk(frame);
        case qmq::spec::queue::DeleteOk:
            return this->onQueueDeleteOk(frame);
        default:
            qWarning() << "Unknown queue frame" << frame->methodId();
            break;
        }
        break;
    case qmq::spec::basic::ID_:
        switch (frame->methodId()) {
        case qmq::spec::basic::QosOk:
            return this->onBasicQosOk(frame);
        case qmq::spec::basic::ConsumeOk:
            return this->onBasicConsumeOk(frame);
        case qmq::spec::basic::CancelOk:
            return this->onBasicCancelOk(frame);
        case qmq::spec::basic::Return:
            return this->onBasicReturn(frame);
        case qmq::spec::basic::Deliver:
            return this->onBasicDeliver(frame);
        case qmq::spec::basic::GetOk:
            return this->onBasicGetOk(frame);
        case qmq::spec::basic::GetEmpty:
            return this->onBasicGetEmpty(frame);
        case qmq::spec::basic::RecoverOk:
            return this->onBasicRecoverOk(frame);
        default:
            qWarning() << "Unknown basic frame" << frame->methodId();
            break;
        }
        break;
    case qmq::spec::tx::ID_:
        switch (frame->methodId()) {
        case qmq::spec::tx::SelectOk:
            return this->onTxSelectOk(frame);
        case qmq::spec::tx::CommitOk:
            return this->onTxCommitOk(frame);
        case qmq::spec::tx::RollbackOk:
            return this->onTxRollbackOk(frame);
        default:
            qWarning() << "Unknown tx frame" << frame->methodId();
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
    if (messageSize > MAX_MESSAGE_SIZE) {
        qWarning() << "Frame too large" << messageSize;
        this->closeChannel(500, "Message too large");
        return false;
    }
    d->deliveringMessage->m_properties = frame->properties();
    d->deliveringMessage->m_contentSize = frame->contentSize();
    d->deliveringMessage->m_payload.reserve(static_cast<qsizetype>(messageSize));
    if (messageSize == 0) {
        this->incomingMessageComplete();
    }
    return true;
}

bool Channel::handleBodyFrame(const BodyFrame *frame)
{
    qDebug() << "Body frame with" << frame->content().size() << "bytes";
    if (!d->deliveringMessage) {
        qWarning() << "Body frame unexpected";
        return false;
    }
    d->deliveringMessage->m_payload.append(frame->content());
    if (static_cast<quint64>(d->deliveringMessage->m_payload.size())
        >= d->deliveringMessage->m_contentSize) {
        this->incomingMessageComplete();
    }
    return true;
}

bool Channel::addConsumer(Consumer *c)
{
    const QString consumerTag = c->consumerTag();

    if (d->consumers.contains(consumerTag)) {
        qWarning() << "Denying attempt to add duplicate consumer";
        return false;
    }

    d->consumers.insert(consumerTag, QPointer<qmq::Consumer>(c));
    return true;
}

// ----------------------------------------------------------------------------
// Channel methods
QFuture<void> Channel::channelOpen()
{
    MethodFrame frame(d->channelId, spec::channel::ID_, spec::channel::Open);
    MessageItemVoidPtr messageTracker(new MessagePromise<void>(frame.classId(), frame.methodId()));
    messageTracker->promise.start();
    const QString reserved1; // out-of-band
    QVariantList args({reserved1});
    qDebug() << "Set open channel method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);
    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
    } else {
        d->inFlightMessages.push_back(messageTracker);
    }
    return messageTracker->promise.future();
}

bool Channel::onChannelOpenOk(const MethodFrame *frame)
{
    Q_UNUSED(frame);
    // OpenOK has only reserved arguments.
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::channel::ID_, spec::channel::Open));

    qDebug() << "channel::OpenOk";
    if (messageTracker) {
        messageTracker->finish();
    }
    return true;
}

QFuture<void> Channel::channelFlow(bool active)
{
    MethodFrame frame(d->channelId, spec::channel::ID_, spec::channel::Flow);
    MessageItemVoidPtr messageTracker(new MessagePromise<void>(frame.classId(), frame.methodId()));
    messageTracker->promise.start();
    const QVariantList args = {active};
    qDebug() << "Set channel flow method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);
    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
    } else {
        d->inFlightMessages.push_back(messageTracker);
    }
    return messageTracker->promise.future();
}

bool Channel::onChannelFlow(const MethodFrame *frame)
{
    bool isOk(false);
    const QVariantList args(frame->getArguments(&isOk));
    const bool active = args.at(0).toBool();
    isOk = this->channelFlowOk(active);

    return isOk;
}

bool Channel::channelFlowOk(bool active)
{
    MethodFrame frame(d->channelId, spec::channel::ID_, spec::channel::FlowOk);
    const QVariantList args = {active};
    qDebug() << "Set channel flow method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    const bool isOk = d->client->sendFrame(&frame);
    return isOk;
}

bool Channel::onChannelFlowOk(const MethodFrame *frame)
{
    Q_UNUSED(frame);
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::channel::ID_, spec::channel::Flow));

    if (messageTracker) {
        messageTracker->finish();
    }
    return true;
}

QFuture<void> Channel::closeChannel(quint16 code,
                                    const QString &replyText,
                                    quint16 classId,
                                    quint16 methodId)
{
    MessageItemVoidPtr messageTracker(
        new MessagePromise<void>(spec::channel::ID_, spec::channel::Close));
    QVariantList args({code, replyText, classId, methodId});
    MethodFrame frame(d->channelId, spec::channel::ID_, spec::channel::Close);
    qDebug() << "Set channel.close frame args" << args;
    frame.setArguments(args);
    messageTracker->promise.start();
    if (!d->client->sendFrame(&frame)) {
        messageTracker->setException(qmq::Exception(1, "Failed to send close frame"));
        messageTracker->finish();
    } else {
        d->inFlightMessages.push_back(messageTracker);
    }
    return messageTracker->promise.future();
}

bool Channel::onChannelCloseOk(const MethodFrame *frame)
{
    Q_UNUSED(frame);
    qDebug() << "CloseOk received";
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::channel::ID_, spec::channel::Close));
    if (messageTracker) {
        messageTracker->finish();
    }
    return true;
}

// Server has requested channel to close (normally due to an error)
bool Channel::onChannelClose(const MethodFrame *frame)
{
    qDebug() << "Close received";
    const QVariantList args = frame->getArguments();
    const quint16 code = args.at(0).value<quint16>();
    const QString replyText = args.at(1).toString();
    const quint16 classId = args.at(2).toUInt();
    const quint16 methodId = args.at(3).value<quint16>();
    qDebug() << "Code:" << code << "replyText:" << replyText << "class, method: (" << classId << ","
             << methodId << ")";
    return this->channelCloseOk();
}

bool Channel::channelCloseOk()
{
    MethodFrame frame(d->channelId, spec::channel::ID_, spec::channel::CloseOk);
    qDebug() << "Set channel.closeOk frame args";
    if (!d->client->sendFrame(&frame)) {
        qWarning() << "Unable to send CloseOK";
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------
// Exchange methods.
QFuture<void> Channel::declareExchange(const QString &exchangeName,
                                       ExchangeType type,
                                       DeclareExchangeOptions opts,
                                       const QVariantHash &arguments)
{
    MessageItemVoidPtr messageTracker(
        new MessagePromise<void>(spec::exchange::ID_, spec::exchange::Declare));
    MethodFrame frame(d->channelId, spec::exchange::ID_, spec::exchange::Declare);
    const short reserved1 = 0;
    const QString exchangeType = exchangeTypeToString(type);
    const bool reserved2 = false;
    const bool reserved3 = false;

    QVariantList args({reserved1,
                       exchangeName,
                       exchangeType,
                       opts.testFlag(DeclareExchangeOption::Passive),
                       opts.testFlag(DeclareExchangeOption::Durable),
                       reserved2,
                       reserved3,
                       opts.testFlag(DeclareExchangeOption::NoWait),
                       arguments});
    qDebug() << "Set declare exchange method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);
    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onExchangeDeclareOk(const MethodFrame *frame)
{
    Q_UNUSED(frame);
    qDebug() << "Declare Exchange OK received";
    MessageItemPtr messageTracker(
        d->popFirstMessageItem(spec::exchange::ID_, spec::exchange::Declare));

    if (messageTracker) {
        messageTracker->finish();
    }
    return true;
}

QFuture<void> Channel::deleteExchange(const QString &exchangeName)
{
    MethodFrame frame(d->channelId, spec::exchange::ID_, spec::exchange::Delete);
    MessageItemVoidPtr messageTracker(new MessagePromise<void>(frame.classId(), frame.methodId()));

    const short reserved1 = 0;
    const bool ifUnused = false;
    const bool noWait = false;
    const QVariantList args({reserved1, exchangeName, ifUnused, noWait});
    qDebug() << "Set delete exchange method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onExchangeDeleteOk(const MethodFrame *frame)
{
    Q_UNUSED(frame);
    qDebug() << "Delete Exchange OK received";
    MessageItemPtr messageTracker(
        d->popFirstMessageItem(spec::exchange::ID_, spec::exchange::Delete));

    if (messageTracker) {
        messageTracker->finish();
    }
    return true;
}

// ----------------------------------------------------------------------------
// Queue methods
QFuture<QVariantList> Channel::declareQueue(const QString &queueName,
                                            DeclareQueueOptions opts,
                                            const QVariantHash &arguments)
{
    MethodFrame frame(d->channelId, spec::queue::ID_, spec::queue::Declare);
    MessageVlistPtr messageTracker(
        new MessagePromise<QVariantList>(frame.classId(), frame.methodId()));

    const short reserved1 = 0;
    QVariantList args({reserved1,
                       queueName,
                       opts.testFlag(DeclareQueueOption::Passive),
                       opts.testFlag(DeclareQueueOption::Durable),
                       opts.testFlag(DeclareQueueOption::Exclusive),
                       opts.testFlag(DeclareQueueOption::AutoDelete),
                       opts.testFlag(DeclareQueueOption::NoWait),
                       arguments});
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
    MessageVlistPtr trackedPromise(getPromise<QVariantList>(messageTracker));
    const QVariantList args = frame->getArguments();
    qDebug() << "Name, messageCount, consumerCount" << args;
    if (messageTracker) {
        if (trackedPromise) {
            trackedPromise->promise.addResult(args);
        }
        messageTracker->finish();
    }
    return true;
}

QFuture<void> Channel::bindQueue(const QString &queueName, const QString &exchangeName)
{
    MethodFrame frame(d->channelId, spec::queue::ID_, spec::queue::Bind);
    MessageItemVoidPtr messageTracker(new MessagePromise<void>(frame.classId(), frame.methodId()));

    const short reserved1 = 0;
    const QString routingKey;
    const bool noWait = false;
    const QVariantHash bindArguments;
    const QVariantList args({reserved1, queueName, exchangeName, routingKey, noWait, bindArguments});
    qDebug() << "Set bind queue method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onQueueBindOk(const MethodFrame *)
{
    qDebug() << "Bind Queue OK received";
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::queue::ID_, spec::queue::Bind));

    if (messageTracker) {
        messageTracker->finish();
    }
    return true;
}

QFuture<void> Channel::unbindQueue(const QString &queueName, const QString &exchangeName)
{
    MethodFrame frame(d->channelId, spec::queue::ID_, spec::queue::Unbind);
    MessageItemVoidPtr messageTracker(new MessagePromise<void>(frame.classId(), frame.methodId()));

    const short reserved1 = 0;
    const QString routingKey;
    const QVariantHash unbindArguments;
    const QVariantList args({reserved1, queueName, exchangeName, routingKey, unbindArguments});
    qDebug() << "Set bind queue method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onQueueUnbindOk(const MethodFrame *frame)
{
    Q_UNUSED(frame);
    qDebug() << "Bind Queue OK received";
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::queue::ID_, spec::queue::Unbind));

    if (messageTracker) {
        messageTracker->finish();
    }
    return true;
}

QFuture<int> Channel::purgeQueue(const QString &queueName)
{
    MethodFrame frame(d->channelId, spec::queue::ID_, spec::queue::Purge);
    MessageIntPtr messageTracker(new MessagePromise<int>(frame.classId(), frame.methodId()));

    const short reserved1 = 0;
    const bool noWait = false;
    const QVariantList args({reserved1, queueName, noWait});
    qDebug() << "Set purge queue method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onQueuePurgeOk(const MethodFrame *frame)
{
    qDebug() << "Purge Queue OK received";
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::queue::ID_, spec::queue::Purge));
    MessageIntPtr trackedPromise(getPromise<int>(messageTracker));
    const QVariantList args = frame->getArguments();
    qDebug() << "messageCount" << args;
    if (messageTracker) {
        if (trackedPromise) {
            trackedPromise->promise.addResult(args.at(0).toInt());
        }
        messageTracker->finish();
    }
    return true;
}

QFuture<int> Channel::deleteQueue(const QString &queueName)
{
    MethodFrame frame(d->channelId, spec::queue::ID_, spec::queue::Delete);
    MessageIntPtr messageTracker(new MessagePromise<int>(frame.classId(), frame.methodId()));

    const short reserved1 = 0;
    const bool ifUnused = false;
    const bool ifEmpty = false;
    const bool noWait = false;
    const QVariantList args({reserved1, queueName, ifUnused, ifEmpty, noWait});
    qDebug() << "Set delete queue method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onQueueDeleteOk(const MethodFrame *frame)
{
    qDebug() << "Delete Queue OK received";
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::queue::ID_, spec::queue::Delete));
    MessageIntPtr trackedPromise(getPromise<int>(messageTracker));
    const QVariantList args = frame->getArguments();
    qDebug() << "messageCount" << args;
    if (messageTracker) {
        if (trackedPromise) {
            trackedPromise->promise.addResult(args.at(0).toInt());
        }
        messageTracker->finish();
    }
    return true;
}

// ----------------------------------------------------------------------------
// Basic Methods
QFuture<void> Channel::qos(uint prefetchSize, ushort prefetchCount, bool global)
{
    MethodFrame frame(d->channelId, spec::basic::ID_, spec::basic::Qos);
    MessageItemVoidPtr messageTracker(new MessagePromise<void>(frame.classId(), frame.methodId()));

    // Long, Short, Bit
    const QVariantList args({prefetchSize, prefetchCount, global});
    qDebug() << "Set qos method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    const bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onBasicQosOk(const MethodFrame *frame)
{
    Q_UNUSED(frame);
    qDebug() << "Basic Qos OK received";
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::basic::ID_, spec::basic::Qos));

    if (messageTracker) {
        messageTracker->finish();
    }
    return true;
}

QFuture<QString> Channel::consume(const QString &queueName,
                                  const QString &consumerTag,
                                  ConsumeOptions flags)
{
    MethodFrame frame(d->channelId, spec::basic::ID_, spec::basic::Consume);
    MessageStrPtr messageTracker(new MessagePromise<QString>(frame.classId(), frame.methodId()));

    // Short, QueueName, ConsumerTag, NoLocal, NoAck, Bit, NoWait, Table
    const short reserved1 = 0;
    const bool noLocal = flags.testFlag(ConsumeOption::NoLocal);
    const bool noAck = flags.testFlag(ConsumeOption::NoAck);
    const bool exclusive = flags.testFlag(ConsumeOption::Exclusive);
    const bool noWait = flags.testFlag(ConsumeOption::NoWait);
    const QVariantHash consumeArguments;
    const QVariantList args(
        {reserved1, queueName, consumerTag, noLocal, noAck, exclusive, noWait, consumeArguments});
    qDebug() << "Set consume method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onBasicConsumeOk(const MethodFrame *frame)
{
    qDebug() << "Basic Consume OK received";
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::basic::ID_, spec::basic::Consume));
    MessageStrPtr trackedPromise(getPromise<QString>(messageTracker));
    const QVariantList args = frame->getArguments();
    qDebug() << "consumerTag" << args;
    if (messageTracker) {
        if (trackedPromise) {
            trackedPromise->promise.addResult(args.at(0).toString());
        }
        messageTracker->finish();
    }

    return true;
}

QFuture<QString> Channel::cancel(const QString &consumerTag, bool noWait)
{
    MethodFrame frame(d->channelId, spec::basic::ID_, spec::basic::Cancel);
    MessageStrPtr messageTracker(new MessagePromise<QString>(frame.classId(), frame.methodId()));

    // ShortStr, Bit
    const QVariantList args({consumerTag, noWait});
    qDebug() << "Set qos method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    const bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onBasicCancelOk(const MethodFrame *frame)
{
    qDebug() << "Basic Cancel OK received";
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::basic::ID_, spec::basic::Cancel));
    MessageStrPtr trackedPromise(getPromise<QString>(messageTracker));
    const QVariantList args = frame->getArguments();
    qDebug() << "consumerTag" << args;
    if (messageTracker) {
        if (trackedPromise) {
            trackedPromise->promise.addResult(args.at(0).toString());
        }
        messageTracker->finish();
    }
    return true;
}

bool Channel::publish(const QString &message,
                      const QString &exchangeName,
                      const QString &routingKey,
                      const BasicPropertyHash &properties,
                      PublishOptions opts)
{
    qmq::Message msg(message.toUtf8(), exchangeName, routingKey, properties);
    if (!properties.contains(qmq::BasicProperty::ContentType)) {
        msg.setProperty(qmq::BasicProperty::ContentType, QStringLiteral("text/plain"));
    }
    if (!properties.contains(qmq::BasicProperty::ContentEncoding)) {
        msg.setProperty(qmq::BasicProperty::ContentEncoding, QStringLiteral("utf-8"));
    }
    return publish(msg, opts);
}

bool Channel::publish(const qmq::Message &message, PublishOptions opts)
{
    MethodFrame frame(d->channelId, spec::basic::ID_, spec::basic::Publish);

    // Short, ExchangeName, ShortStr, Bit, Bit
    const short reserved1 = 0;
    const QString exchangeName = message.exchangeName();
    const QString routingKey = message.routingKey();
    const bool mandatory = opts.testFlag(PublishOption::Mandatory);
    const bool immediate = opts.testFlag(PublishOption::Immediate);
    const QVariantList args({reserved1, exchangeName, routingKey, mandatory, immediate});
    qDebug() << "Set publish method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        return false;
    }

    // d->inFlightMessages.push_back(messageTracker);
    const QByteArray payload = message.payload();
    HeaderFrame header(d->channelId, frame.classId(), payload.size(), message.properties());
    isOk = d->client->sendFrame(&header);
    if (!isOk) {
        return false;
    }

    qint64 writtenBytes = 0;
    const qint64 maxFrameSize = d->client->maxFrameSizeBytes();
    const qint64 maxPayloadsize = maxFrameSize - 8;
    while (writtenBytes < payload.size()) {
        const qint64 len = std::min(maxPayloadsize, payload.size() - writtenBytes);
        const QByteArray part = payload.mid(writtenBytes, len);
        BodyFrame body(d->channelId, part);
        isOk = d->client->sendFrame(&body);
        if (!isOk) {
            return false;
        }
        writtenBytes += len;
    }
    return true;
}

bool Channel::onBasicReturn(const MethodFrame *frame)
{
    bool isOk = false;
    // {code, replyText, exchangeName, routingKey}
    const QVariantList args = frame->getArguments(&isOk);
    qDebug() << "Return received" << args;
    if (!isOk) {
        qWarning() << "Failed to get arguments";
    }

    const quint16 code = args.at(0).value<quint16>();
    const QString replyText = args.at(1).toString();
    const QString exchangeName = args.at(2).toString();
    const QString routingKey = args.at(3).toString();

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
    const quint64 deliveryTag = args.at(1).toULongLong();
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

QFuture<QVariantList> Channel::get(const QString &queueName, bool noAck)
{
    MethodFrame frame(d->channelId, spec::basic::ID_, spec::basic::Get);
    MessageVlistPtr messageTracker(
        new MessagePromise<QVariantList>(frame.classId(), frame.methodId()));

    // Short, ShortStr, Bit
    const ushort reserved1 = 0;
    const QVariantList args({reserved1, queueName, noAck});
    qDebug() << "Set get method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    const bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onBasicGetOk(const MethodFrame *frame)
{
    qDebug() << "Basic Get OK received";
    // {DeliveryTag, Redelivered, ExchangeName, ShortStr, Long};
    bool isOk = false;
    const QVariantList args = frame->getArguments(&isOk);
    if (!isOk) {
        qWarning() << "Failed to get arguments";
    }
    const quint64 deliveryTag = args.at(0).toULongLong();
    const bool redelivered = args.at(1).toBool();
    const QString exchangeName = args.at(2).toString();
    const QString routingKey = args.at(3).toString();
    const quint32 messageCount = args.at(4).toUInt();

    d->deliveringMessage.reset(new IncomingMessage);
    d->deliveringMessage->m_consumerTag = QString();
    d->deliveringMessage->m_deliveryTag = deliveryTag;
    d->deliveringMessage->m_exchangeName = exchangeName;
    d->deliveringMessage->m_redelivered = redelivered;
    d->deliveringMessage->m_routingKey = routingKey;
    d->deliveringMessage->m_isGet = true;
    d->deliveringMessage->m_messageCount = messageCount;

    return true;
}

bool Channel::onBasicGetEmpty(const MethodFrame *frame)
{
    Q_UNUSED(frame);
    qDebug() << "Basic Get Empty received";
    // Only argument is reserved.
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::basic::ID_, spec::basic::Get));
    if (messageTracker) {
        messageTracker->finish();
    }
    return true;
}

bool Channel::ack(quint64 deliveryTag, bool muliple)
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

bool Channel::reject(quint64 deliveryTag, bool requeue)
{
    MethodFrame frame(d->channelId, spec::basic::ID_, spec::basic::Reject);

    QVariantList args({deliveryTag, requeue});
    qDebug() << "Set reject method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    const bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        qWarning() << "Failed to send frame";
    }

    return isOk;
}

bool Channel::recoverAsync(bool requeue)
{
    MethodFrame frame(d->channelId, spec::basic::ID_, spec::basic::RecoverAsync);

    const QVariantList args = {requeue};
    qDebug() << "Set recoverAsync method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    const bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        qWarning() << "Failed to send frame";
    }

    return isOk;
}

QFuture<void> Channel::recover(bool requeue)
{
    MethodFrame frame(d->channelId, spec::basic::ID_, spec::basic::Recover);
    MessageItemVoidPtr messageTracker(new MessagePromise<void>(frame.classId(), frame.methodId()));

    // ShortStr, Bit
    QVariantList args = {requeue};
    qDebug() << "Set recover method" << d->channelId << "frame args" << args;
    frame.setArguments(args);
    const bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onBasicRecoverOk(const MethodFrame *frame)
{
    Q_UNUSED(frame);
    qDebug() << "Basic Recover OK received";
    // Only argument is reserved.
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::basic::ID_, spec::basic::Recover));
    if (messageTracker) {
        messageTracker->finish();
    }
    return true;
}
// ----------------------------------------------------------------------------
// TX
QFuture<void> Channel::txSelect()
{
    MethodFrame frame(d->channelId, spec::tx::ID_, spec::tx::Select);
    MessageItemVoidPtr messageTracker(new MessagePromise<void>(frame.classId(), frame.methodId()));
    // No args.
    qDebug() << "Set Tx Select method" << d->channelId;
    const bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onTxSelectOk(const MethodFrame *frame)
{
    Q_UNUSED(frame);
    qDebug() << "Tx Select OK received";
    // Only argument is reserved.
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::tx::ID_, spec::tx::Select));
    if (messageTracker) {
        messageTracker->finish();
    }
    return true;
}

QFuture<void> Channel::txCommit()
{
    MethodFrame frame(d->channelId, spec::tx::ID_, spec::tx::Commit);
    MessageItemVoidPtr messageTracker(new MessagePromise<void>(frame.classId(), frame.methodId()));
    // No args.
    qDebug() << "Set Tx Select method" << d->channelId;
    const bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onTxCommitOk(const MethodFrame *frame)
{
    Q_UNUSED(frame);
    qDebug() << "Tx Commit OK received";
    // Only argument is reserved.
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::tx::ID_, spec::tx::Commit));
    if (messageTracker) {
        messageTracker->finish();
    }
    return true;
}

QFuture<void> Channel::txRollback()
{
    MethodFrame frame(d->channelId, spec::tx::ID_, spec::tx::Rollback);
    MessageItemVoidPtr messageTracker(new MessagePromise<void>(frame.classId(), frame.methodId()));
    // No args.
    qDebug() << "Set Tx Rollback method" << d->channelId;
    const bool isOk = d->client->sendFrame(&frame);

    if (!isOk) {
        messageTracker->setException(qmq::Exception(1, "Failed to send frame"));
        messageTracker->finish();
        return messageTracker->promise.future();
    }

    d->inFlightMessages.push_back(messageTracker);

    return messageTracker->promise.future();
}

bool Channel::onTxRollbackOk(const MethodFrame *frame)
{
    Q_UNUSED(frame);
    qDebug() << "Tx Select OK received";
    // Only argument is reserved.
    MessageItemPtr messageTracker(d->popFirstMessageItem(spec::tx::ID_, spec::tx::Rollback));
    if (messageTracker) {
        messageTracker->finish();
    }
    return true;
}

// ----------------------------------------------------------------------------
void Channel::incomingMessageComplete()
{
    qDebug() << "Message complete with delivery tag" << d->deliveringMessage->m_deliveryTag
             << "and size" << d->deliveringMessage->m_payload.size();
    qDebug() << "payload"
             << (QString::fromUtf8(d->deliveringMessage->m_payload.left(64))
                 + (d->deliveringMessage->m_payload.size() > 64 ? "...[truncated]" : ""));

    qmq::Message msg(d->deliveringMessage->m_payload,
                     d->deliveringMessage->m_exchangeName,
                     d->deliveringMessage->m_routingKey,
                     d->deliveringMessage->m_properties);
    msg.setDeliveryTag(d->deliveringMessage->m_deliveryTag);
    msg.setRedelivered(d->deliveringMessage->m_redelivered);

    if (d->deliveringMessage->m_isGet) {
        MessageItemPtr messageTracker(d->popFirstMessageItem(spec::basic::ID_, spec::basic::Get));
        if (!messageTracker) {
            qWarning() << "Unexpected message";
            return;
        }
        MessageVlistPtr trackedPromise(getPromise<QVariantList>(messageTracker));

        const QVariantList promiseArgs = {QVariant::fromValue<qmq::Message>(msg),
                                          d->deliveringMessage->m_messageCount};
        trackedPromise->promise.addResult(promiseArgs);
        trackedPromise->finish();
    } else {
        const QString consumerTag = d->deliveringMessage->m_consumerTag;
        auto consumerIt = d->consumers.find(consumerTag);
        if (consumerIt == d->consumers.end()) {
            qWarning() << "No consumer found for message";
        }

        consumerIt.value()->pushMessage(msg);
    }
}
} // namespace qmq

#include <channel.moc>
