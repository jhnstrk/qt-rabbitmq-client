#pragma once

#include <QByteArray>
#include <QStringList>

namespace qmq {
class Authenticator
{
public:
    virtual ~Authenticator() {}

    virtual QString mechanism() const = 0;
    virtual QByteArray responseBytes(const QByteArray &challenge) const = 0;
};

class AmqpPlainAuthenticator : public Authenticator
{
public:
    AmqpPlainAuthenticator();

    QString mechanism() const override { return "AMQPLAIN"; }

    QByteArray responseBytes(const QByteArray &challenge) const override;

    //! Usernane and password are stored as bytearrays to avoid utf-encoding issues.
    void setUsername(const QByteArray &u) { m_username = u; }
    void setPassword(const QByteArray &p) { m_password = p; }

private:
    QByteArray m_username;
    QByteArray m_password;
};

class SaslPlainAuthenticator : public Authenticator
{
public:
    SaslPlainAuthenticator();

    QString mechanism() const override { return "PLAIN"; }

    QByteArray responseBytes(const QByteArray &challenge) const override;

    void setUsername(const QByteArray &u) { m_username = u; }
    void setPassword(const QByteArray &p) { m_password = p; }

private:
    QByteArray m_username;
    QByteArray m_password;
};
} // namespace qmq
