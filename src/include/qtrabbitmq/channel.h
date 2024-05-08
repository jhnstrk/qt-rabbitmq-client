#pragma once

#include <QFuture>
#include <QObject>
#include <QScopedPointer>

#include "abstract_frame_handler.h"
#include "consumer.h"
#include "message.h"

namespace qmq {
class Client;

struct DeclareExchangeOptions
{
    bool passive = false;
    bool durable = false;
    bool noWait = false;
    QVariantHash arguments;
};

struct DeclareQueueOptions
{
    bool passive = false;
    bool durable = false;
    bool exclusive = false;
    bool autoDelete = false;
    bool noWait = false;
    QVariantHash arguments;
};

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

    QFuture<void> openChannel();
    QFuture<void> flow(bool active);
    QFuture<void> closeChannel(quint16 code = 200,
                               const QString &replyText = QString(),
                               quint16 classId = 0,
                               quint16 methodId = 0);

    QFuture<void> declareExchange(const QString &exchangeName,
                                  ExchangeType type,
                                  const DeclareExchangeOptions &opts = DeclareExchangeOptions());
    QFuture<void> deleteExchange(const QString &exchangeName);

    QFuture<QVariantList> declareQueue(const QString &queueName,
                                       const DeclareQueueOptions &opts = DeclareQueueOptions());
    QFuture<void> bindQueue(const QString &queueName, const QString &exchangeName);
    QFuture<void> unbindQueue(const QString &queueName, const QString &exchangeName);

    // Returns the message-count
    QFuture<int> deleteQueue(const QString &queueName);

    // Returns the message-count
    QFuture<int> purgeQueue(const QString &queueName);

    // Client methods for the "Basic" API.
    QFuture<void> qos(uint prefetchSize, ushort prefetchCount, bool global);
    enum class ConsumeOption {
        NoOptions = 0x0,
        NoLocal = 0x1,
        NoAck = 0x2,
        Exclusive = 0x4,
        NoWait = 0x8,
    };
    Q_DECLARE_FLAGS(ConsumeOptions, ConsumeOption)
    QFuture<QString> consume(const QString &queueName,
                             const QString &consumerTag,
                             ConsumeOptions flags = ConsumeOption::NoOptions);
    // Returns the consumer tag.
    QFuture<QString> cancel(const QString &consumerTag, bool noWait = false);
    // If the queue isn't empty, return value is { message, messageCount }
    // If the queue is empty, the return is empty.
    QFuture<QVariantList> get(const QString &queueName, bool noAck);
    bool publish(const QString &exchangeName, const qmq::Message &message);
    bool recoverAsync(bool requeue);
    QFuture<void> recover(bool requeue);
    bool ack(qint64 deliveryTag, bool muliple = false);
    bool reject(qint64 deliveryTag, bool requeue);

    QFuture<void> txSelect();
    QFuture<void> txRollback();
    QFuture<void> txCommit();

    bool handleMethodFrame(const MethodFrame *frame) override;
    bool handleHeaderFrame(const HeaderFrame *frame) override;
    bool handleBodyFrame(const BodyFrame *frame) override;
    bool handleHeartbeatFrame(const HeartbeatFrame *) override { return false; }

protected:
    bool onChannelOpenOk(const MethodFrame *frame);
    bool onChannelFlow(const MethodFrame *frame);
    bool flowOk(bool active);
    bool onChannelFlowOk(const MethodFrame *frame);
    bool onChannelClose(const MethodFrame *frame);
    bool onChannelCloseOk(const MethodFrame *frame);
    bool closeOk();

    bool onExchangeDeclareOk(const MethodFrame *frame);
    bool onExchangeDeleteOk(const MethodFrame *frame);

    bool onQueueDeclareOk(const MethodFrame *frame);
    bool onQueueBindOk(const MethodFrame *frame);
    bool onQueueUnbindOk(const MethodFrame *frame);
    bool onQueueDeleteOk(const MethodFrame *frame);
    bool onQueuePurgeOk(const MethodFrame *frame);

    bool onBasicQosOk(const MethodFrame *frame);
    bool onBasicConsumeOk(const MethodFrame *frame);
    bool onBasicDeliver(const MethodFrame *frame);
    bool onBasicCancelOk(const MethodFrame *frame);
    bool onBasicReturn(const MethodFrame *frame);
    bool onBasicGetOk(const MethodFrame *frame);
    bool onBasicGetEmpty(const MethodFrame *frame);
    bool onBasicRecoverOk(const MethodFrame *frame);

    bool onTxSelectOk(const MethodFrame *frame);
    bool onTxCommitOk(const MethodFrame *frame);
    bool onTxRollbackOk(const MethodFrame *frame);

    void incomingMessageComplete();
public Q_SLOTS:

protected Q_SLOTS:

private:
    Q_DISABLE_COPY(Channel)

    class Private;
    QScopedPointer<Private> d;
};

} // namespace qmq
