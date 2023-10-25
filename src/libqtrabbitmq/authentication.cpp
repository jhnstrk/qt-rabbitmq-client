#include <qtrabbitmq/authentication.h>

#include "frame.h"

#include <QBuffer>

namespace qmq {
AmqpPlainAuthenticator::AmqpPlainAuthenticator() {}

QByteArray AmqpPlainAuthenticator::responseBytes(const QByteArray &challenge) const
{
    QByteArray result;
    {
        QBuffer io(&result);
        bool ok = io.open(QIODevice::WriteOnly);
        QVariantHash data;
        data["LOGIN"] = this->m_username;
        data["PASSWORD"] = this->m_password;
        qDebug() << "Build AMQPLAIN response";
        ok = detail::Frame::writeNativeFieldValue(&io, data, FieldValue::FieldTable);
    }
    return result.mid(4);
}

SaslPlainAuthenticator::SaslPlainAuthenticator() {}

QByteArray SaslPlainAuthenticator::responseBytes(const QByteArray &challenge) const
{
    // https://datatracker.ietf.org/doc/html/rfc4616
    const QByteArray authzid; // Intentionally empty
    const QByteArray authcid = this->m_username;
    const QByteArray passwd = this->m_password;
    const QByteArray nul(1, char(0));

    qDebug() << "Build PLAIN response";
    const QByteArray result = authzid + nul + authcid + nul + passwd;
    return result;
}

} // namespace qmq
