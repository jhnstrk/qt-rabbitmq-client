#pragma once

#include <QFuture>
#include <QObject>
#include <QScopedPointer>

namespace qmq {
class Client;

class Channel : public QObject
{
    Q_OBJECT
public:
    virtual ~Channel();

    int channelId() const;

    QFuture<void> declareExchange(const QString &exchangeName);
    QFuture<void> declareQueue(const QString &queueName);
    QFuture<void> bindQueue(const QString &queueName, const QString &exchangeName);

Q_SIGNALS:
    void messageReceived();

public Q_SLOTS:
    void open();

protected Q_SLOTS:

private:
    Q_DISABLE_COPY(Channel)
    explicit Channel(Client *client);

    class Private;
    QScopedPointer<Private> d;
};

} // namespace qmq
