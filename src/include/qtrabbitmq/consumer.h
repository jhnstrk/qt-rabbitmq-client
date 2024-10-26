#pragma once

#include <QFuture>
#include <QObject>
#include <QString>

#include "message.h"
#include "qtrabbitmq.h"

#include "qtrabbitmq_export.h"

namespace qmq {
class Channel;

class QTRABBITMQ_EXPORT Consumer : public QObject
{
    Q_OBJECT
public:
    Consumer(const QString &consumerTag = QString(), QObject *parent = nullptr);
    ~Consumer() override;

    QString consumerTag() const;

    QFuture<QString> consume(Channel *channel, const QString &queue);

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
