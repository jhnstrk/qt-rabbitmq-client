#pragma once

#include <QObject>
#include <QScopedPointer>

#include <qtrabbitmq/client.h>

namespace qmq {
class Queue : public QObject
{
    Q_OBJECT
public:
    virtual ~Queue();

    QString name() const;

Q_SIGNALS:
    void declareOk();

public Q_SLOTS:
    void declare();

protected Q_SLOTS:

private:
    Q_DISABLE_COPY(Queue)
    explicit Queue(Client *client);

    class Private;
    QScopedPointer<Private> d;
};

} // namespace qmq
