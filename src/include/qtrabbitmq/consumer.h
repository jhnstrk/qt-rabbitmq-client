#pragma once

#include <QFuture>
#include <QObject>
#include <QString>

#include "message.h"
#include "qtrabbitmq.h"

namespace qmq {
class Channel;

class Consumer : public QObject
{
    Q_OBJECT
public:
    Consumer(const QString &consumerTag = QString(), QObject *parent = nullptr);
    virtual ~Consumer();

    QString consumerTag() const;

    QFuture<void> consume(Channel *channel, const QString &queue);

    Message dequeueMessage();

    void pushMessage(const qmq::Message &msg);
    bool hasMessage() const;

Q_SIGNALS:
    void messageReady();

private:
    class Private;
    QScopedPointer<Private> d;
};
} // namespace qmq
