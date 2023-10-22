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

private:
    bool onStart(const MethodFrame *frame);
};

} // namespace detail

} // namespace qmq
