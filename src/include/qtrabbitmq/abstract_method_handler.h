#pragma once

#include <qtrabbitmq/qtrabbitmq.h>

#include <qtrabbitmq/frame.h>

namespace qmq {

class AbstractMethodHandler
{
public:
    virtual ~AbstractMethodHandler() {}

    virtual bool handleFrame(const MethodFrame *frame) = 0;
};

} // namespace qmq
