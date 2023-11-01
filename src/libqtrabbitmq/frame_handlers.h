#pragma once

#include <qtrabbitmq/qtrabbitmq.h>

#include <qtrabbitmq/frame.h>

namespace qmq {

namespace detail {

class AbstractMethodHandler
{
public:
    virtual bool handleFrame(const MethodFrame *frame) = 0;
};

} // namespace detail

} // namespace qmq
