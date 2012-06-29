#include "YetiSocket.h"

#include "YetiUtil.h"

NAMEBEG

String SocketAddress::to_string() const
{
    String s = m_ipaddress_.to_string();
    s += ':';
    s += String::from_integer(m_port_);

    return s;
}

bool SocketAddress::operator ==(const SocketAddress & other) const
{
    return (other.get_ipaddress().as_long() == m_ipaddress_.as_long() && other.get_port() == m_port_);
}

NAMEEND
