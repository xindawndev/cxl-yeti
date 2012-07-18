#ifndef _CXL_YETI_SOCKET_H_
#define _CXL_YETI_SOCKET_H_

#include "YetiTypes.h"
#include "YetiConstants.h"
#include "YetiStreams.h"
#include "YetiString.h"
#include "YetiDataBuffer.h"
#include "YetiNetwork.h"

NAMEBEG

const int YETI_ERROR_CONNECTION_RESET       = YETI_ERROR_BASE_SOCKET - 0;
const int YETI_ERROR_CONNECTION_ABORTED     = YETI_ERROR_BASE_SOCKET - 1;
const int YETI_ERROR_CONNECTION_REFUSED     = YETI_ERROR_BASE_SOCKET - 2;
const int YETI_ERROR_CONNECTION_FAILED      = YETI_ERROR_BASE_SOCKET - 3;
const int YETI_ERROR_HOST_UNKNOWN           = YETI_ERROR_BASE_SOCKET - 4;
const int YETI_ERROR_SOCKET_FAILED          = YETI_ERROR_BASE_SOCKET - 5;
const int YETI_ERROR_GETSOCKOPT_FAILED      = YETI_ERROR_BASE_SOCKET - 6;
const int YETI_ERROR_SETSOCKOPT_FAILED      = YETI_ERROR_BASE_SOCKET - 7;
const int YETI_ERROR_SOCKET_CONTROL_FAILED  = YETI_ERROR_BASE_SOCKET - 8;
const int YETI_ERROR_BIND_FAILED            = YETI_ERROR_BASE_SOCKET - 9;
const int YETI_ERROR_LISTEN_FAILED          = YETI_ERROR_BASE_SOCKET - 10;
const int YETI_ERROR_ACCEPT_FAILED          = YETI_ERROR_BASE_SOCKET - 11;
const int YETI_ERROR_ADDRESS_IN_USE         = YETI_ERROR_BASE_SOCKET - 12;
const int YETI_ERROR_NETWORK_DOWN           = YETI_ERROR_BASE_SOCKET - 13;
const int YETI_ERROR_NETWORK_UNREACHABLE    = YETI_ERROR_BASE_SOCKET - 14;
const int YETI_ERROR_NOT_CONNECTED          = YETI_ERROR_BASE_SOCKET - 15;

const unsigned int YETI_SOCKET_FLAG_CANCELLABLE = 1;

class Socket;

class SocketAddress
{
public:
    SocketAddress() : m_port_(0) {}
    SocketAddress(const IpAddress & address, IpPort port)
        : m_ipaddress_(address)
        , m_port_(port)
    {
    }

    YETI_Result set_ipaddress(const IpAddress & address)
    {
        m_ipaddress_ = address;
        return YETI_SUCCESS;
    }

    const IpAddress & get_ipaddress() const 
    {
        return m_ipaddress_;
    }

    YETI_Result set_port(IpPort port)
    {
        m_port_ = port;
        return YETI_SUCCESS;
    }

    IpPort get_port() const 
    {
        return m_port_;
    }

    String to_string() const;

    bool operator==(const SocketAddress & other) const;

private:
    IpAddress m_ipaddress_;
    IpPort m_port_;
};

typedef struct {
    SocketAddress local_address;
    SocketAddress remote_address;
} SocketInfo;

class SocketInterface
{
public:
    virtual ~SocketInterface() {}

    virtual YETI_Result bind(const SocketAddress & address, bool reuse_address = true) = 0;
    virtual YETI_Result connect(const SocketAddress & address, YETI_Timeout timeout) = 0;
    virtual YETI_Result wait_for_connection(YETI_Timeout timeout) = 0;
    virtual YETI_Result get_input_stream(InputStreamReference & stream) = 0;
    virtual YETI_Result get_output_stream(OutputStreamReference & stream) = 0;
    virtual YETI_Result get_info(SocketInfo & info) = 0;
    virtual YETI_Result set_read_timeout(YETI_Timeout timeout) = 0;
    virtual YETI_Result set_write_timeout(YETI_Timeout timeout) = 0;
    virtual YETI_Result cancel(bool shutdown = true) = 0;
};

class UdpSocketInterface
{
public:
    virtual ~UdpSocketInterface() {}

    virtual YETI_Result send(const DataBuffer & packet,
        const SocketAddress * address = NULL) = 0;
    virtual YETI_Result receive(DataBuffer & packet,
        SocketAddress * address = NULL) = 0;
};

class UdpMulticastSocketInterface
{
public:
    virtual ~UdpMulticastSocketInterface() {}

    virtual YETI_Result join_group(const IpAddress & group,
        const IpAddress & iface) = 0;
    virtual YETI_Result leave_group(const IpAddress & group,
        const IpAddress & iface) = 0;
    virtual YETI_Result set_timetolive(unsigned char ttl) = 0;
    virtual YETI_Result set_interface(const IpAddress & iface) = 0;
};

class TcpServerSocketInterface
{
public:
    virtual ~TcpServerSocketInterface() {}

    virtual YETI_Result listen(unsigned int max_clients) = 0;
    virtual YETI_Result wait_for_new_client(Socket *& client,
        YETI_Timeout timeout,
        YETI_Flags flags) = 0;
};

class Socket : public SocketInterface
{
public:
    explicit Socket(SocketInterface * delegate) : m_socket_delegate_(delegate) {}
    virtual ~Socket();

    YETI_Result bind(const SocketAddress & address, bool reuse_address = true) {
        return m_socket_delegate_->bind(address, reuse_address);
    }

    YETI_Result connect(const SocketAddress & address, YETI_Timeout timeout = YETI_TIMEOUT_INFINITE) {
        return m_socket_delegate_->connect(address, timeout);
    }

    YETI_Result wait_for_connection(YETI_Timeout timeout = YETI_TIMEOUT_INFINITE) {
        return m_socket_delegate_->wait_for_connection(timeout);
    }

    YETI_Result get_input_stream(InputStreamReference & stream) {
        return m_socket_delegate_->get_input_stream(stream);
    }

    YETI_Result get_output_stream(OutputStreamReference & stream) {
        return m_socket_delegate_->get_output_stream(stream);
    }

    YETI_Result get_info(SocketInfo & info) {
        return m_socket_delegate_->get_info(info);
    }

    YETI_Result set_read_timeout(YETI_Timeout timeout) {
        return m_socket_delegate_->set_read_timeout(timeout);
    }

    YETI_Result set_write_timeout(YETI_Timeout timeout) {
        return m_socket_delegate_->set_write_timeout(timeout);
    }

    YETI_Result cancel(bool shutdown = true) {
        return m_socket_delegate_->cancel(shutdown);
    }

protected:
    Socket() {}
    SocketInterface * m_socket_delegate_;
};

typedef Reference<Socket> SocketReference;

class UdpSocket : public Socket, public UdpSocketInterface
{
public:
    UdpSocket(YETI_Flags flags = YETI_SOCKET_FLAG_CANCELLABLE);
    virtual ~UdpSocket();

    YETI_Result send(const DataBuffer & packet, const SocketAddress * address = NULL) {
        return m_udpsocket_delegate_->send(packet, address);
    }

    YETI_Result receive(DataBuffer & packet, SocketAddress * address = NULL) {
        return m_udpsocket_delegate_->receive(packet, address);
    }

protected:
    UdpSocket(UdpSocketInterface * delegate);
    UdpSocketInterface * m_udpsocket_delegate_;
};

class UdpMulticastSocket : public UdpSocket, public UdpMulticastSocketInterface
{
public:
    UdpMulticastSocket(YETI_Flags flags = YETI_SOCKET_FLAG_CANCELLABLE);
    virtual ~UdpMulticastSocket();

    YETI_Result join_group(const IpAddress & group, const IpAddress & iface = IpAddress::Any) {
        return m_udpmulticastsocket_delegate_->join_group(group, iface);
    }

    YETI_Result leave_group(const IpAddress & group, const IpAddress & iface = IpAddress::Any) {
        return m_udpmulticastsocket_delegate_->leave_group(group, iface);
    }

    YETI_Result set_timetolive(unsigned char ttl) {
        return m_udpmulticastsocket_delegate_->set_timetolive(ttl);
    }

    YETI_Result set_interface(const IpAddress & iface) {
        return m_udpmulticastsocket_delegate_->set_interface(iface);
    }

private:
    UdpMulticastSocketInterface * m_udpmulticastsocket_delegate_;
};

class TcpClientSocket : public Socket
{
public:
    TcpClientSocket(YETI_Flags flags = YETI_SOCKET_FLAG_CANCELLABLE);
    virtual ~TcpClientSocket();
};

class TcpServerSocket : public Socket, public TcpServerSocketInterface
{
public:
    TcpServerSocket(YETI_Flags flags = YETI_SOCKET_FLAG_CANCELLABLE);
    virtual ~TcpServerSocket();

    YETI_Result listen(unsigned int max_clients) {
        return m_tcpserversocket_delegate_->listen(max_clients);
    }

    YETI_Result wait_for_new_client(Socket *& client, YETI_Timeout timeout = YETI_TIMEOUT_INFINITE, YETI_Flags flags = 0) {
        return m_tcpserversocket_delegate_->wait_for_new_client(client, timeout, flags);
    }
protected:
    TcpServerSocketInterface * m_tcpserversocket_delegate_;
};

NAMEEND

#endif // _CXL_YETI_SOCKET_H_
