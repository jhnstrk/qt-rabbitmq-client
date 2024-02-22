#pragma once

#include <qglobal.h>
#include <qtrabbitmq/abstract_method_handler.h>

namespace qmq {
class Client;

namespace detail {
class ExchangeHandler : public AbstractMethodHandler
{
public:
    // Types defined by Rabbit: https://www.rabbitmq.com/tutorials/amqp-concepts.html
    enum class ExchangeType { Invalid, Direct, Fanout, Topic, Match };
    ExchangeHandler(Client *client,
                    quint16 channelId,
                    const QString &exchangeName,
                    ExchangeType type = ExchangeType::Direct);
    bool handleFrame(const MethodFrame *frame) override;
    bool sendDeclare();
    bool sendDelete();

    static QString typeToString(ExchangeType type);
    static ExchangeType stringToType(const QString &typeStr);

protected:
    bool onDeclareOk(const MethodFrame *frame);
    bool onDeleteOk(const MethodFrame *frame);

private:
    Client *m_client = nullptr;
    quint16 m_channelId = 0;
    QString m_exchangeName;
    ExchangeType m_type = ExchangeType::Invalid;
};

} // namespace detail
} // namespace qmq
