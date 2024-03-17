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

    explicit Channel(Client *client, qint16 channelId);
    virtual ~Channel();

    int channelId() const;

    bool addConsumer(Consumer *c, const QString &queueName);

    QFuture<void> openChannel();
    QFuture<void> closeChannel(qint16 code,
                               const QString &replyText,
                               quint16 classId,
                               quint16 methodId);

    QFuture<void> declareExchange(const QString &exchangeName,
                                  ExchangeType type,
                                  const DeclareExchangeOptions &opts = DeclareExchangeOptions());

    QFuture<void> declareQueue(const QString &queueName,
                               const DeclareQueueOptions &opts = DeclareQueueOptions());
    QFuture<void> bindQueue(const QString &queueName, const QString &exchangeName);

    enum class ConsumeOption {
        NoOptions = 0x0,
        NoLocal = 0x1,
        NoAck = 0x2,
        Exclusive = 0x4,
        NoWait = 0x8,
    };
    Q_DECLARE_FLAGS(ConsumeOptions, ConsumeOption)
    QFuture<void> consume(const QString &queueName,
                          const QString &consumerTag,
                          ConsumeOptions flags = ConsumeOption::NoOptions);

    bool sendAck(qint64 deliveryTag, bool muliple);
    bool handleMethodFrame(const MethodFrame *frame) override;
    bool handleHeaderFrame(const HeaderFrame *frame) override;
    bool handleBodyFrame(const BodyFrame *frame) override;
    bool handleHeartbeatFrame(const HeartbeatFrame *frame) override {}

    QFuture<void> publish(const QString &exchangeName, const qmq::Message &message);

protected:
    bool onOpenOk(const MethodFrame *frame);
    bool onCloseOk(const MethodFrame *frame);

    bool onExchangeDeclareOk(const MethodFrame *frame);
    bool onQueueDeclareOk(const MethodFrame *frame);
    bool onQueueBindOk(const MethodFrame *frame);

    bool onBasicConsumeOk(const MethodFrame *frame);
    bool onBasicDeliver(const MethodFrame *frame);

    void incomingMessageComplete();
public Q_SLOTS:

protected Q_SLOTS:

private:
    Q_DISABLE_COPY(Channel)

    class Private;
    QScopedPointer<Private> d;
};

} // namespace qmq
