#pragma once

#include <QFuture>
#include <QObject>
#include <QScopedPointer>

#include "abstract_frame_handler.h"
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

    QFuture<void> consume(const QString &queueName);

    bool sendAck(qlonglong deliveryTag, bool muliple);
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
