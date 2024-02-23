#pragma once

#include "qtrabbitmq/abstract_method_handler.h"

#include <QObject>
#include <qglobal.h>

namespace qmq {
class Client;

namespace detail {
class ConnectionHandler : public QObject, public AbstractMethodHandler
{
    Q_OBJECT
public:
    ConnectionHandler(Client *client);
    bool handleFrame(const MethodFrame *frame) override;

    bool sendClose(qint16 code, const QString &replyText, quint16 classId, quint16 methodId);

Q_SIGNALS:
    void connectionOpened();

protected:
    bool sendStartOk();
    bool sendTuneOk();

    bool sendOpen();
    bool onOpenOk(const MethodFrame *frame);
    bool onClose(const MethodFrame *frame);
    bool sendCloseOk();

private:
    bool onStart(const MethodFrame *frame);
    bool onTune(const MethodFrame *frame);
    bool onCloseOk(const MethodFrame *frame);

    Client *m_client = nullptr;
    quint16 m_channelMax = 255;
    quint32 m_frameMaxSizeBytes = 1024 * 1024;
    quint16 m_heartbeatSeconds = 8;
};

} // namespace detail
} // namespace qmq
