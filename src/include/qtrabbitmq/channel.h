#pragma once

#include <QFuture>
#include <QObject>
#include <QScopedPointer>

#include "abstract_method_handler.h"

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

class Channel : public QObject, public AbstractMethodHandler
{
    Q_OBJECT
public:
    enum class ExchangeType { Fanout, Direct, Topic, Headers };
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

    virtual bool handleFrame(const MethodFrame *frame) override;

protected:
    bool onOpenOk(const MethodFrame *frame);
    bool onCloseOk(const MethodFrame *frame);

    bool onExchangeDeclareOk(const MethodFrame *frame);
    bool onQueueDeclareOk(const MethodFrame *frame);
    bool onQueueBindOk(const MethodFrame *frame);

public Q_SLOTS:

protected Q_SLOTS:

private:
    Q_DISABLE_COPY(Channel)

    class Private;
    QScopedPointer<Private> d;
};

} // namespace qmq
