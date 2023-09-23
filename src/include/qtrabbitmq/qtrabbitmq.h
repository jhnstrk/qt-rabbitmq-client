#pragma once

#include <QObject>
#include <QScopedPointer>

class QUrl;

namespace qmq {

class Client : public QObject
{
    Q_OBJECT

    Client(QObject *parent = nullptr);
    ~Client();

    QUrl connectionUrl() const;
    void setConnectionUrl(const QUrl &url);

private:
    Q_DISABLE_COPY(Client)

    class Private;
    QScopedPointer<Private> d;
};

} // namespace qmq
