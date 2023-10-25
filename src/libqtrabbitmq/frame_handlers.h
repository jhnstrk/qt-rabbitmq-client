#include <qtrabbitmq/qtrabbitmq.h>

#include "frame.h"

namespace qmq {

namespace detail {

class AbstractMethodHandler
{
public:
    virtual bool handleFrame(const MethodFrame *frame) = 0;
};

class ConnectionHandler : public AbstractMethodHandler
{
public:
    ConnectionHandler(Client *client);
    bool handleFrame(const MethodFrame *frame) override;

protected:
    bool sendStartOk();
    bool sendTuneOk();

private:
    bool onStart(const MethodFrame *frame);
    bool onTune(const MethodFrame *frame);

    Client *m_client = nullptr;
    quint16 m_channelMax = 255;
    quint32 m_frameMaxSizeBytes = 1024 * 1024;
    quint16 m_heartbeatSeconds = 8;
};

} // namespace detail

} // namespace qmq
