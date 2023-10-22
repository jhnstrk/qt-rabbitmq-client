#include "frame_handlers.h"
#include "spec_constants.h"

namespace qmq {
namespace detail {

ConnectionHandler::ConnectionHandler(Client * client){

}

bool ConnectionHandler::handleFrame(const MethodFrame *frame)
{
    Q_ASSERT(frame->classId() == static_cast<quint16>(qmq::FrameType::Method));
    switch (frame->methodId()) {
    case static_cast<quint16>(Connection::Start):
        return this->onStart(frame);
    default:
        qWarning() << "Unknown connection frame" << frame->methodId();
        break;
    }
    return false;
}

bool ConnectionHandler::onStart(const MethodFrame *frame)
{
    const QList<FieldValue> types
        = {Domain::Octet, Domain::Octet, Domain::PeerProperties, Domain::LongStr, Domain::LongStr};
    const QVariantList args = frame->getArguments(types);
    if (args.isEmpty()) {
        qWarning() << "Failed to parse args";
        return false;
    }
    qDebug() << "Start" << args;
    const QString protocolVersion = QString("%1.%2").arg(args.at(0).toInt()).arg(args.at(1).toInt());
    qDebug() << "Protocol:" << protocolVersion;
    return true;
}

} // namespace detail
} // namespace qmq
