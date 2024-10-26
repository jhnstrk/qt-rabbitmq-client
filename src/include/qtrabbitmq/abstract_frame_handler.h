#pragma once

#include <qtrabbitmq/qtrabbitmq.h>

#include <qtrabbitmq/frame.h>

#include "qtrabbitmq_export.h"

namespace qmq {

class QTRABBITMQ_EXPORT AbstractFrameHandler
{
public:
    virtual ~AbstractFrameHandler() {}

    virtual bool handleMethodFrame(const MethodFrame &frame) = 0;
    virtual bool handleHeaderFrame(const HeaderFrame &frame) = 0;
    virtual bool handleBodyFrame(const BodyFrame &frame) = 0;
    virtual bool handleHeartbeatFrame(const HeartbeatFrame &frame) = 0;
};

} // namespace qmq
