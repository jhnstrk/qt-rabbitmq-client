#pragma once

#include <QException>

namespace qmq {

class Exception : public QException
{
public:
    Exception(int code, const QString &message) noexcept
        : m_what(message.toUtf8())
        , m_code(code)
    {}
    Exception(Exception &&other) noexcept
        : m_what(other.m_what)
        , m_code(other.m_code)
    {}

    Exception(const Exception &other) noexcept
        : m_what(other.m_what)
        , m_code(other.m_code)
    {}
    virtual ~Exception() noexcept override {}

    void raise() const override { throw *this; }
    Exception *clone() const override { return new Exception(*this); }

    const char *what() const noexcept override { return m_what.constData(); }

private:
    QByteArray m_what; // Required to provide "what".
    int m_code = 0;
};

} // namespace qmq