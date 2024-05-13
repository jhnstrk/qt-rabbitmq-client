#pragma once

#include <QFuture>
#include <QObject>
#include <QScopedPointer>

#include "abstract_frame_handler.h"
#include "consumer.h"
#include "message.h"

namespace qmq {
class Client;

enum class ExchangeDeclareOption {
    NoOptions = 0x0,
    Passive = 0x1,
    Durable = 0x2,
    NoWait = 0x10,
};
Q_DECLARE_FLAGS(ExchangeDeclareOptions, ExchangeDeclareOption)

enum class ExchangeDeleteOption {
    NoOptions = 0x0,
    IfUnused = 0x01,
    NoWait = 0x10,
};
Q_DECLARE_FLAGS(ExchangeDeleteOptions, ExchangeDeleteOption)

enum class QueueDeclareOption {
    NoOptions = 0x0,
    Passive = 0x1,
    Durable = 0x2,
    Exclusive = 0x4,
    AutoDelete = 0x8,
    NoWait = 0x10,
};
Q_DECLARE_FLAGS(QueueDeclareOptions, QueueDeclareOption)

enum class QueueDeleteOption {
    NoOptions = 0x0,
    IfUnused = 0x01,
    IfEmpty = 0x02,
    NoWait = 0x10,
};
Q_DECLARE_FLAGS(QueueDeleteOptions, QueueDeleteOption)

enum class PublishOption {
    NoOptions = 0x0,
    Mandatory = 0x1,
    Immediate = 0x2,
};
Q_DECLARE_FLAGS(PublishOptions, PublishOption)

enum class ConsumeOption {
    NoOptions = 0x0,
    NoLocal = 0x1,
    NoAck = 0x2,
    Exclusive = 0x4,
    NoWait = 0x10,
};
Q_DECLARE_FLAGS(ConsumeOptions, ConsumeOption)

class Channel : public QObject, public AbstractFrameHandler
{
    Q_OBJECT
public:
    // Types defined by Rabbit: https://www.rabbitmq.com/tutorials/amqp-concepts.html
    enum class ExchangeType { Invalid, Direct, Fanout, Topic, Match, Headers = Match };

    explicit Channel(Client *client, quint16 channelId);
    virtual ~Channel();

    int channelId() const;

    bool addConsumer(Consumer *c);

    QFuture<void> channelOpen();
    QFuture<void> channelFlow(bool active);
    QFuture<void> channelClose(quint16 code = 200,
                               const QString &replyText = QString(),
                               quint16 classId = 0,
                               quint16 methodId = 0);

    // RabbitMQ extension args: alternate-exchange
    QFuture<void> exchangeDeclare(const QString &exchangeName,
                                  ExchangeType type,
                                  ExchangeDeclareOptions opts = ExchangeDeclareOption::NoOptions,
                                  const QVariantHash &arguments = QVariantHash());
    QFuture<void> exchangeDelete(const QString &exchangeName,
                                 ExchangeDeleteOptions opts = ExchangeDeleteOption::NoOptions);
    QFuture<void> exchangeBind(const QString &exchangeNameDestination,
                               const QString &exchangeNameSource,
                               const QString &routingKey,
                               bool noWait = false,
                               const QVariantHash &arguments = QVariantHash());
    QFuture<void> exchangeUnbind(const QString &exchangeNameDestination,
                                 const QString &exchangeNameSource,
                                 const QString &routingKey,
                                 bool noWait = false,
                                 const QVariantHash &arguments = QVariantHash());

    // RabbitMQ extensions args: x-message-ttl, x-expires
    QFuture<QVariantList> queueDeclare(const QString &queueName,
                                       QueueDeclareOptions opts = QueueDeclareOption::NoOptions,
                                       const QVariantHash &arguments = QVariantHash());
    QFuture<void> queueBind(const QString &queueName,
                            const QString &exchangeName,
                            const QString &routingKey = QString(),
                            bool noWait = false,
                            const QVariantHash &arguments = QVariantHash());
    QFuture<void> queueUnbind(const QString &queueName,
                              const QString &exchangeName,
                              const QString &routingKey = QString(),
                              const QVariantHash &arguments = QVariantHash());

    // Returns the message-count
    QFuture<int> queueDelete(const QString &queueName,
                             QueueDeleteOptions opts = QueueDeleteOption::NoOptions);

    // Returns the message-count
    QFuture<int> queuePurge(const QString &queueName, bool noWait = false);

    // Client methods for the "Basic" API.
    QFuture<void> basicQos(quint32 prefetchSize, quint16 prefetchCount, bool global);

    QFuture<QString> basicConsume(const QString &queueName,
                                  const QString &consumerTag,
                                  ConsumeOptions flags = ConsumeOption::NoOptions);
    // Returns the consumer tag.
    QFuture<QString> basicCancel(const QString &consumerTag, bool noWait = false);
    // If the queue isn't empty, return value is { message, messageCount }
    // If the queue is empty, the return is empty.
    QFuture<QVariantList> basicGet(const QString &queueName, bool noAck);
    bool basicPublish(const qmq::Message &message, PublishOptions opts = PublishOption::NoOptions);
    bool basicPublish(const QString &payload,
                      const QString &exchangeName,
                      const QString &routingKey = QString(),
                      const BasicPropertyHash &properties = BasicPropertyHash(),
                      PublishOptions opts = PublishOption::NoOptions);

    bool basicRecoverAsync(bool requeue);
    QFuture<void> basicRecover(bool requeue);
    bool basicAck(quint64 deliveryTag, bool muliple = false);
    // Rabbit extension
    bool basicNack(quint64 deliveryTag, bool muliple = false, bool requeue = false);
    bool basicReject(quint64 deliveryTag, bool requeue);

    QFuture<void> confirmSelect(bool noWait);

    QFuture<void> txSelect();
    QFuture<void> txRollback();
    QFuture<void> txCommit();

    bool handleMethodFrame(const MethodFrame &frame) override;
    bool handleHeaderFrame(const HeaderFrame &frame) override;
    bool handleBodyFrame(const BodyFrame &frame) override;
    bool handleHeartbeatFrame(const HeartbeatFrame &) override { return false; }

protected:
    bool onChannelOpenOk(const MethodFrame &frame);
    bool onChannelFlow(const MethodFrame &frame);
    bool channelFlowOk(bool active);
    bool onChannelFlowOk(const MethodFrame &frame);
    bool onChannelClose(const MethodFrame &frame);
    bool onChannelCloseOk(const MethodFrame &frame);
    bool channelCloseOk();

    bool onExchangeDeclareOk(const MethodFrame &frame);
    bool onExchangeDeleteOk(const MethodFrame &frame);
    bool onExchangeBindOk(const MethodFrame &frame);
    bool onExchangeUnbindOk(const MethodFrame &frame);

    bool onQueueDeclareOk(const MethodFrame &frame);
    bool onQueueBindOk(const MethodFrame &frame);
    bool onQueueUnbindOk(const MethodFrame &frame);
    bool onQueueDeleteOk(const MethodFrame &frame);
    bool onQueuePurgeOk(const MethodFrame &frame);

    bool onBasicQosOk(const MethodFrame &frame);
    bool onBasicConsumeOk(const MethodFrame &frame);
    bool onBasicDeliver(const MethodFrame &frame);
    bool onBasicCancelOk(const MethodFrame &frame);
    bool onBasicReturn(const MethodFrame &frame);
    bool onBasicGetOk(const MethodFrame &frame);
    bool onBasicGetEmpty(const MethodFrame &frame);
    bool onBasicRecoverOk(const MethodFrame &frame);

    bool onConfirmSelectOk(const MethodFrame &frame);

    bool onTxSelectOk(const MethodFrame &frame);
    bool onTxCommitOk(const MethodFrame &frame);
    bool onTxRollbackOk(const MethodFrame &frame);

    void incomingMessageComplete();
public Q_SLOTS:

protected Q_SLOTS:

private:
    Q_DISABLE_COPY(Channel)

    class Private;
    QScopedPointer<Private> d;
};

} // namespace qmq
