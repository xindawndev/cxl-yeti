#if (defined(_WIN32) || defined(_WIN32_WCE) || defined(_XBOX)) && !defined(__SYMBIAN32__)
#   if !defined(__WINSOCK__) 
#       define __WINSOCK__ 
#   endif
#endif

#if defined(__WINSOCK__) && !defined(_XBOX)
#   define STRICT
#   define YETI_WIN32_USE_WINSOCK2
#   ifdef YETI_WIN32_USE_WINSOCK2
#       include <winsock2.h>
#       include <ws2tcpip.h> 
#   else
#       include <winsock.h>
#   endif
#   include <windows.h>
// XBox
#elif defined(_XBOX)
#   include <xtl.h>
#   include <winsockx.h>
#elif defined(__TCS__)
// Trimedia includes
#   include <sockets.h>
#elif defined(__PSP__)
// PSP includes
#   include <psptypes.h>
#   include <kernel.h>
#   include <pspnet.h>
#   include <pspnet_error.h>
#   include <pspnet_inet.h>
#   include <pspnet_resolver.h>
#   include <pspnet_apctl.h>
#   include <pspnet_ap_dialog_dummy.h>
#   include <errno.h>
#   include <wlan.h>
#   include <pspnet/sys/socket.h>
#   include <pspnet/sys/select.h>
#   include <pspnet/netinet/in.h>
#elif defined(__PPU__)
// PS3 includes
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <sys/time.h>
#   include <sys/select.h>
#   include <netinet/in.h>
#   include <netinet/tcp.h>
#   include <netdb.h>
#   include <fcntl.h>
#   include <unistd.h>
#   include <stdio.h>
#   include <netex/net.h>
#   include <netex/errno.h>
#else
// default includes
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <sys/select.h>
#   include <sys/time.h>
#   include <sys/ioctl.h>
#   include <netinet/in.h>
#   if !defined(__SYMBIAN32__)
#       include <netinet/tcp.h>
#   endif
#   include <netdb.h>
#   include <fcntl.h>
#   include <unistd.h>
#   include <string.h>
#   include <stdio.h>
#   include <errno.h>
#   include <signal.h>
#endif 

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiStreams.h"
#include "YetiThreads.h"
#include "YetiSocket.h"
#include "YetiUtil.h"
#include "YetiConstants.h"
#include "YetiLogging.h"

YETI_SET_LOCAL_LOGGER("yeti.sockets.bsd")

#if defined(__WINSOCK__)
#if defined(_XBOX)
#include "YetiXboxNetwork.h"
#define SO_ERROR    0x1007          /* unsupported */
#else
#include "YetiWin32Network.h"
#endif

NAMEBEG
static WinsockSystem & WinsockInitializer = WinsockSystem::Initializer; 
NAMEEND

#if defined(set_port)
#undef set_port
#endif

#define EWOULDBLOCK  WSAEWOULDBLOCK
#define EINPROGRESS  WSAEINPROGRESS
#define ECONNREFUSED WSAECONNREFUSED
#define ECONNABORTED WSAECONNABORTED
#define ECONNRESET   WSAECONNRESET
#define ETIMEDOUT    WSAETIMEDOUT
#define ENETRESET    WSAENETRESET
#define EADDRINUSE   WSAEADDRINUSE
#define ENETDOWN     WSAENETDOWN
#define ENETUNREACH  WSAENETUNREACH
#define ENOTCONN     WSAENOTCONN
#if !defined(EAGAIN)
#define EAGAIN       WSAEWOULDBLOCK 
#define EINTR        WSAEINTR
#endif
#if !defined(SHUT_RDWR)
#define SHUT_RDWR SD_BOTH 
#endif

#if !defined(__MINGW32__)
typedef int         ssize_t;
#endif
typedef int         socklen_t;
typedef char*       SocketBuffer;
typedef const char* SocketConstBuffer;
typedef char*       SocketOption;
typedef SOCKET      SocketFd;

#define GetSocketError()                  WSAGetLastError()
#define YETI_BSD_SOCKET_IS_INVALID(_s)    ((_s) == INVALID_SOCKET)
#define YETI_BSD_SOCKET_CALL_FAILED(_e)   ((_e) == SOCKET_ERROR)
#define YETI_BSD_SOCKET_SELECT_FAILED(_e) ((_e) == SOCKET_ERROR)

/*----------------------------------------------------------------------
|   Trimedia adaptation layer
+---------------------------------------------------------------------*/
#elif defined(__TCS__)  // trimedia PSOS w/ Target TCP
typedef void*       SocketBuffer;
typedef const void* SocketConstBuffer;
typedef void*       SocketOption;
typedef int         SocketFd;

#define GetSocketError()                 errno
#define YETI_BSD_SOCKET_IS_INVALID(_s)    ((_s)  < 0)
#define YETI_BSD_SOCKET_CALL_FAILED(_e)   ((_e)  < 0)
#define YETI_BSD_SOCKET_SELECT_FAILED(_e) ((_e)  < 0)

/*----------------------------------------------------------------------
|   PSP adaptation layer
+---------------------------------------------------------------------*/
#elif defined(__PSP__)
typedef SceNetInetSocklen_t socklen_t;
#define timeval SceNetInetTimeval
#define inet_addr sceNetInetInetAddr
#define select sceNetInetSelect
#define socket sceNetInetSocket
#define connect sceNetInetConnect
#define bind sceNetInetBind
#define accept sceNetInetAccept
#define getpeername sceNetInetGetpeername
#define getsockopt sceNetInetGetsockopt
#define setsockopt sceNetInetSetsockopt
#define listen sceNetInetListen
#define getsockname sceNetInetGetsockname
#define sockaddr SceNetInetSockaddr
#define sockaddr_in SceNetInetSockaddrIn
#define in_addr SceNetInetInAddr
#define send  sceNetInetSend
#define sendto sceNetInetSendto
#define recv  sceNetInetRecv
#define recvfrom sceNetInetRecvfrom
#define closesocket sceNetInetClose
#define htonl sceNetHtonl
#define htons sceNetHtons
#define ntohl sceNetNtohl
#define ntohs sceNetNtohs
#define SOL_SOCKET SCE_NET_INET_SOL_SOCKET
#define AF_INET SCE_NET_INET_AF_INET
#define SOCK_STREAM SCE_NET_INET_SOCK_STREAM
#define SOCK_DGRAM SCE_NET_INET_SOCK_DGRAM
#define SO_BROADCAST SCE_NET_INET_SO_BROADCAST
#define SO_ERROR SCE_NET_INET_SO_ERROR
#define IPPROTO_IP SCE_NET_INET_IPPROTO_IP
#define IP_ADD_MEMBERSHIP SCE_NET_INET_IP_ADD_MEMBERSHIP
#define IP_MULTICAST_IF SCE_NET_INET_IP_MULTICAST_IF
#define IP_MULTICAST_TTL SCE_NET_INET_IP_MULTICAST_TTL
#define SO_REUSEADDR SCE_NET_INET_SO_REUSEADDR
#define INADDR_ANY SCE_NET_INET_INADDR_ANY
#define ip_mreq SceNetInetIpMreq
#ifdef fd_set
#undef fd_set
#endif
#define fd_set SceNetInetFdSet
#ifdef FD_ZERO
#undef FD_ZERO
#endif
#define FD_ZERO SceNetInetFD_ZERO
#ifdef FD_SET
#undef FD_SET
#endif
#define FD_SET SceNetInetFD_SET
#ifdef FD_CLR
#undef FD_CLR
#endif
#define FD_CLR SceNetInetFD_CLR
#ifdef FD_ISSET
#undef FD_ISSET
#endif
#define FD_ISSET SceNetInetFD_ISSET

#define RESOLVER_TIMEOUT (5 * 1000 * 1000)
#define RESOLVER_RETRY    5

typedef void*       SocketBuffer;
typedef const void* SocketConstBuffer;
typedef void*       SocketOption;
typedef int         SocketFd;

#define GetSocketError()                 sceNetInetGetErrno()
#define YETI_BSD_SOCKET_IS_INVALID(_s)    ((_s) < 0)
#define YETI_BSD_SOCKET_CALL_FAILED(_e)   ((_e) < 0)
#define YETI_BSD_SOCKET_SELECT_FAILED(_e) ((_e) < 0)

/*----------------------------------------------------------------------
|   PS3 adaptation layer
+---------------------------------------------------------------------*/
#elif defined(__PPU__)
#undef EWOULDBLOCK    
#undef ECONNREFUSED  
#undef ECONNABORTED  
#undef ECONNRESET    
#undef ETIMEDOUT     
#undef ENETRESET     
#undef EADDRINUSE    
#undef ENETDOWN      
#undef ENETUNREACH   
#undef EAGAIN        
#undef EINTR     
#undef EINPROGRESS

#define EWOULDBLOCK   SYS_NET_EWOULDBLOCK 
#define ECONNREFUSED  SYS_NET_ECONNREFUSED
#define ECONNABORTED  SYS_NET_ECONNABORTED
#define ECONNRESET    SYS_NET_ECONNRESET
#define ETIMEDOUT     SYS_NET_ETIMEDOUT
#define ENETRESET     SYS_NET_ENETRESET
#define EADDRINUSE    SYS_NET_EADDRINUSE
#define ENETDOWN      SYS_NET_ENETDOWN
#define ENETUNREACH   SYS_NET_ENETUNREACH
#define EAGAIN        SYS_NET_EAGAIN
#define EINTR         SYS_NET_EINTR
#define EINPROGRESS   SYS_NET_EINPROGRESS

typedef void*        SocketBuffer;
typedef const void*  SocketConstBuffer;
typedef void*        SocketOption;
typedef int          SocketFd;

#define closesocket  socketclose
#define select       socketselect

#define GetSocketError()                 sys_net_errno
#define YETI_BSD_SOCKET_IS_INVALID(_s)    ((_s) < 0)
#define YETI_BSD_SOCKET_CALL_FAILED(_e)   ((_e) < 0)
#define YETI_BSD_SOCKET_SELECT_FAILED(_e) ((_e) < 0)

// network initializer 
static struct YETI_Ps3NetworkInitializer {
    YETI_Ps3NetworkInitializer() {
        sys_net_initialize_network();
    }
    ~YETI_Ps3NetworkInitializer() {
        sys_net_finalize_network();
    }
} Ps3NetworkInitializer;

/*----------------------------------------------------------------------
|   Default adaptation layer
+---------------------------------------------------------------------*/
#else  
typedef void*       SocketBuffer;
typedef const void* SocketConstBuffer;
typedef void*       SocketOption;
typedef int         SocketFd;

#define closesocket  close
#define ioctlsocket  ioctl

#define GetSocketError()                 errno
#define YETI_BSD_SOCKET_IS_INVALID(_s)    ((_s)  < 0)
#define YETI_BSD_SOCKET_CALL_FAILED(_e)   ((_e)  < 0)
#define YETI_BSD_SOCKET_SELECT_FAILED(_e) ((_e)  < 0)

#endif

NAMEBEG

const int YETI_TCP_SERVER_SOCKET_DEFAULT_LISTEN_COUNT = 20;

static void socket_address_to_inet_address(const SocketAddress & socket_address,
                                           struct sockaddr_in * inet_address)
{
    for (int i = 0; i < 8; ++i) inet_address->sin_zero[i] = 0;
    inet_address->sin_family = AF_INET;
    inet_address->sin_port = htons(socket_address.get_port());
    inet_address->sin_addr.s_addr = htonl(socket_address.get_ipaddress().as_long());
}

static void inet_address_to_socket_address(const struct sockaddr_in * inet_address,
                                           SocketAddress & socket_address)
{
    socket_address.set_port(ntohs(inet_address->sin_port));
    socket_address.set_ipaddress(IpAddress(ntohl(inet_address->sin_addr.s_addr)));
}

static YETI_Result map_error_code(int error)
{
    switch (error)
    {
    case ECONNRESET:
    case ENETRESET:
        //case ENOTCONN:
        //case ESHUTDOWN:
        return YETI_ERROR_CONNECTION_ABORTED;

    case ECONNREFUSED:
        return YETI_ERROR_CONNECTION_REFUSED;

    case ETIMEDOUT:
        return YETI_ERROR_TIMEOUT;

    case EADDRINUSE:
        return YETI_ERROR_ADDRESS_IN_USE;

    case ENETDOWN:
        return YETI_ERROR_NETWORK_DOWN;

    case ENETUNREACH:
        return YETI_ERROR_NETWORK_UNREACHABLE;

    case EINPROGRESS:
    case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
    case EWOULDBLOCK:
#endif
        return YETI_ERROR_WOULD_BLOCK;

#if defined(EPIPE)
    case EPIPE:
        return YETI_ERROR_CONNECTION_RESET;
#endif

#if defined(ENOTCONN)
    case ENOTCONN:
        return YETI_ERROR_NOT_CONNECTED;
#endif

#if defined(EINTR)
    case EINTR:
        return YETI_ERROR_INTERRUPTED;
#endif

#if defined(EACCES)
    case EACCES:
        return YETI_ERROR_PERMISSION_DENIED;
#endif

    default:
        return YETI_ERROR_ERRNO(error);
    }
}

#if defined(YETI_CONFIG_HAVE_GETADDRINFO)
static YETI_Result map_get_addr_info_error_code(int error_code)
{
    switch (error_code)
    {
    case EAI_AGAIN:
        return YETI_ERROR_TIMEOUT;
    default:
        return YETI_ERROR_HOST_UNKNOWN;
    }
}
#endif

#if defined(_XBOX)

struct hostent {
    char    * h_name;           /* official name of host */
    char    * * h_aliases;      /* alias list */
    short   h_addrtype;         /* host address type */
    short   h_length;           /* length of address */
    char    * * h_addr_list;    /* list of addresses */
#define h_addr  h_addr_list[0]  /* address, for backward compat */
};

typedef struct {
    struct hostent server;
    char name[128];
    char addr[16];
    char* addr_list[4];
} HostEnt;

static struct hostent* 
gethostbyname(const char* name)
{
    struct hostent* host = NULL;
    HostEnt*        host_entry = new HostEnt;
    WSAEVENT        hEvent = WSACreateEvent();
    XNDNS*          pDns = NULL;

    INT err = XNetDnsLookup(name, hEvent, &pDns);
    WaitForSingleObject(hEvent, INFINITE);
    if (pDns) {
        if (pDns->iStatus == 0) {
            strcpy(host_entry->name, name);
            host_entry->addr_list[0] = host_entry->addr;
            memcpy(host_entry->addr, &(pDns->aina[0].s_addr), 4);
            host_entry->server.h_name = host_entry->name;
            host_entry->server.h_aliases = 0;
            host_entry->server.h_addrtype = AF_INET;
            host_entry->server.h_length = 4;
            host_entry->server.h_addr_list = new char*[4];

            host_entry->server.h_addr_list[0] = host_entry->addr_list[0];
            host_entry->server.h_addr_list[1] = 0;

            host = (struct hostent*)host_entry;
        }
        XNetDnsRelease(pDns);
    }
    WSACloseEvent(hEvent);
    return host;
};

#endif // _XBOX

YETI_Result IpAddress::resolve_name(const char * name, YETI_Timeout timeout /* = YETI_TIMEOUT_INFINITE */)
{
    if (name == NULL || name[0] == '\0') return YETI_ERROR_HOST_UNKNOWN;
    IpAddress numerical_address;
    if (YETI_SUCCEEDED(numerical_address.parse(name))) {
        return set(numerical_address.as_long());
    }
#if defined(__TCS__)
    set(getHostByName(name));
#elif defined(__PSP__)
    int rid;
    char buf[1024];
    int buflen = sizeof(buf);

    int ret = sceNetResolverCreate(&rid, buf, buflen);
    if(ret < 0){
        return YETI_FAILURE;
    }
    ret = sceNetResolverStartNtoA(rid, name, &address->sin_addr,
        RESOLVER_TIMEOUT, RESOLVER_RETRY);
    if(ret < 0){
        return YETI_ERROR_HOST_UNKNOWN;
    }
    sceNetResolverDelete(rid);
#elif defined(YETI_CONFIG_HAVE_GETADDRINFO)
    struct addrinfo * infos = NULL;
    int result = getaddrinfo(name,
        NULL,
        NULL,
        &infos);
    if (result != 0) {
        return map_get_addr_info_error_code(result);
    }
    bool found = false;
    for (struct addrinfo * info = infos; !found && info; info = info->ai_next) {
        if (info->ai_family != AF_INET) continue;
        if (info->ai_addrlen != sizeof(struct sockaddr_in)) continue;
        if (info->ai_protocol != 0 && info->ai_protocol != IPPROTO_TCP) continue;
        struct sockaddr_in * inet_addr = (struct sockaddr_in *)info->ai_addr;
        set(ntohl(inet_addr->sin_addr.s_addr));
        found = true;
    }
    freeaddrinfo(infos);
    if (!found) {
        return YETI_ERROR_HOST_UNKNOWN;
    }
#else
    struct hostent * host_entry = gethostbyname(name);
    if (host_entry == NULL ||
        host_entry->h_addrtype != AF_INET) {
            return YETI_ERROR_HOST_UNKNOWN;
    }
    MemoryCopy(m_address_, host_entry->h_addr, 4);

#if defined(_XBOX)
    delete host_entry;   
#endif

#endif

    return YETI_SUCCESS;
}

class BsdSocketFd
{
public:
    BsdSocketFd(SocketFd fd, YETI_Flags flags)
        : m_socket_fd_(fd)
    , m_read_timeout_(YETI_TIMEOUT_INFINITE)
    , m_write_timeout_(YETI_TIMEOUT_INFINITE)
    , m_position_(0)
    , m_cancelled_(false)
    , m_cancellable_((flags & YETI_SOCKET_FLAG_CANCELLABLE) != 0){
        set_blocking_mode(false);
#if !defined(__WINSOCK__)
        if (flags & YETI_SOCKET_FLAG_CANCELLABLE) {
            int result = socketpair(AF_UNIX, SOCK_DGRAM, 0, m_cancel_fds_);
            if (result != 0) {
                YETI_LOG_WARNING_1("socketpair failed (%d)", GetSocketError());
                m_cancel_fds_[0] = m_cancel_fds_[1] = -1;
                m_cancellable_ = false;
            }
        } else {
            m_cancel_fds_[0] = m_cancel_fds_[1] = -1;
        }
#endif
    }

    ~BsdSocketFd() {
#if !defined(__WINSOCK__)
        if (m_cancellable_) {
            close(m_cancel_fds_[0]);
            close(m_cancel_fds_[1]);
        }
#endif
        closesocket(m_socket_fd_);
    }

    YETI_Result set_blocking_mode(bool blocking);
    YETI_Result wait_until_readable();
    YETI_Result wait_until_writeable();

    SocketFd        m_socket_fd_;
    YETI_Timeout    m_read_timeout_;
    YETI_Timeout    m_write_timeout_;
    YETI_Position   m_position_;
    volatile bool   m_cancelled_;
    bool            m_cancellable_;
#if !defined(__WINSOCK__)
    SocketFd        m_cancel_fds_[2];
#endif

private:
    friend class BsdTcpServerSocket;
    friend class BsdTcpClientSocket;
    YETI_Result _wait_for_condition(bool readable, bool writeable, bool async_connect, YETI_Timeout timeout);
};

typedef Reference<BsdSocketFd> BsdSocketFdReference;

#if defined(__WINSOCK__) || defined(__TCS__)

YETI_Result BsdSocketFd::set_blocking_mode(bool blocking)
{
    unsigned long args = blocking ? 0 : 1;
    if (ioctlsocket(m_socket_fd_, FIONBIO, &args)) {
        return YETI_ERROR_SOCKET_CONTROL_FAILED;
    }

    return YETI_SUCCESS;
}

#elif defined(__PSP__) || defined(__PPU__)

YETI_Result BsdSocketFd::set_blocking_mode(bool blocking)
{
    int args = blocking ? 0 : 1;
    if (setsockopt(m_socket_fd_, SOL_SOCKET, SO_NBIO, &args, sizeof(args))) {
        return YETI_ERROR_SOCKET_CONTROL_FAILED;
    }

    return YETI_SUCCESS;
}

#else

YETI_Result BsdSocketFd::set_blocking_mode(bool blocking)
{
    int flags = fcntl(m_socket_fd_, F_GETFL, 0);
    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    if (fcntl(m_socket_fd_, F_SETFL, flags)) {
        return YETI_ERROR_SOCKET_CONTROL_FAILED;
    }
    return YETI_SUCCESS;
}

#endif

YETI_Result BsdSocketFd::wait_until_readable()
{
    return _wait_for_condition(true, false, false, m_read_timeout_);
}

YETI_Result BsdSocketFd::wait_until_writeable()
{
    return _wait_for_condition(false, true, false, m_write_timeout_);
}

YETI_Result BsdSocketFd::_wait_for_condition(bool readable, bool writeable, bool async_connect, YETI_Timeout timeout)
{
    YETI_Result result = YETI_SUCCESS;
    int max_fd = (int)m_socket_fd_;
    fd_set read_set;
    fd_set write_set;
    fd_set except_set;

    FD_ZERO(&read_set);
    if (readable) FD_SET(m_socket_fd_, &read_set);
    FD_ZERO(&write_set);
    if (writeable) FD_SET(m_socket_fd_, &write_set);
    FD_ZERO(&except_set);
    FD_SET(m_socket_fd_, &except_set);

#if !defined(__WINSOCK__)
    if (m_cancellable_ && timeout) {
        if (m_cancel_fds_[1] > max_fd) max_fd = m_cancel_fds_[1];
        FD_SET(m_cancel_fds_[1], &read_set);
    }
#endif

    struct timeval timeout_value;
    if (timeout != YETI_TIMEOUT_INFINITE) {
        timeout_value.tv_sec = timeout / 1000;
        timeout_value.tv_usec = 1000 * (timeout - 1000 * (timeout / 1000));
    }

    YETI_LOG_FINER_2("waiting for condition (%s %s)", readable ? "read" : "", writeable ? "write" : "");

    int io_result = ::select(max_fd + 1, &read_set, &write_set, &except_set, timeout == YETI_TIMEOUT_INFINITE ? NULL : &timeout_value);
    YETI_LOG_FINER_1("select returned %d", io_result);

    if (m_cancelled_) return YETI_ERROR_CANCELLED;

    if (io_result == 0) {
        if (timeout == 0) {
            result = YETI_ERROR_WOULD_BLOCK;
        } else {
            result = YETI_ERROR_TIMEOUT;
        }
    } else if (YETI_BSD_SOCKET_SELECT_FAILED(io_result)) {
        result = map_error_code(GetSocketError());
    } else if ((readable && FD_ISSET(m_socket_fd_, &read_set)) ||
        (writeable && FD_ISSET(m_socket_fd_, &write_set))) {
            if (async_connect) {
                int error = 0;
                socklen_t length = sizeof(error);
                io_result = getsockopt(m_socket_fd_, SOL_SOCKET, SO_ERROR, (SocketOption)&error, &length);
                if (YETI_BSD_SOCKET_CALL_FAILED(io_result)) {
                    result = map_error_code(GetSocketError());
                } else if (error) {
                    result = map_error_code(error);
                } else {
                    result = YETI_SUCCESS;
                }
            } else {
                result = YETI_SUCCESS;
            }
    } else if (FD_ISSET(m_socket_fd_, &except_set)) {
        YETI_LOG_FINE("select socket exception is set");
        int error = 0;
        socklen_t length = sizeof(error);
        io_result = getsockopt(m_socket_fd_, SOL_SOCKET, SO_ERROR, (SocketOption)&error, &length);
        if (YETI_BSD_SOCKET_CALL_FAILED(io_result)) {
            result = map_error_code(GetSocketError());
        } else if (error) {
            result = map_error_code(error);
        } else {
            result = YETI_FAILURE;
        }
    } else {
        YETI_LOG_FINE("unexected select state");
        result = YETI_FAILURE;
    }

    if (YETI_FAILED(result)) {
        YETI_LOG_FINER_1("select result = %d", result);
    }

    return result;
}

class BsdSocketStream
{
public:
    BsdSocketStream(BsdSocketFdReference & socket_fd)
        : m_socket_fd_reference_(socket_fd) {}

    YETI_Result seek(YETI_Position) { return YETI_ERROR_NOT_SUPPORTED; }
    YETI_Result tell(YETI_Position & where) {
        where = 0;
        return YETI_SUCCESS;
    }

protected:
    virtual ~BsdSocketStream() {}
    BsdSocketFdReference m_socket_fd_reference_;
};

class BsdSocketInputStream : public InputStream, private BsdSocketStream
{
public:
    BsdSocketInputStream(BsdSocketFdReference & socket_fd)
        : BsdSocketStream(socket_fd) {}

    YETI_Result read(void * buffer, YETI_Size bytes_to_read, YETI_Size * bytes_read /* = NULL */);
    YETI_Result seek(YETI_Position offset) {
        return BsdSocketStream::seek(offset);
    }
    YETI_Result tell(YETI_Position & where) {
        return BsdSocketStream::tell(where);
    }
    YETI_Result get_size(YETI_LargeSize & size);
    YETI_Result get_available(YETI_LargeSize & available);
};

YETI_Result BsdSocketInputStream::read(void * buffer, YETI_Size bytes_to_read, YETI_Size * bytes_read)
{
    if (m_socket_fd_reference_->m_read_timeout_) {
        YETI_Result result = m_socket_fd_reference_->wait_until_readable();
        if (result != YETI_SUCCESS) return result;
    }
    
    YETI_LOG_FINEST_1("reading %d from socket", (int)bytes_to_read);
    ssize_t nb_read = recv(m_socket_fd_reference_->m_socket_fd_, (SocketBuffer)buffer, bytes_to_read, 0);
    YETI_LOG_FINEST_1("recv returned %d", (int)nb_read);

    if (nb_read <= 0) {
        if (bytes_read) *bytes_read = 0;
        if (m_socket_fd_reference_->m_cancelled_) return YETI_ERROR_CANCELLED;
        if (nb_read == 0) {
            YETI_LOG_FINE("socket end of stream");
            return YETI_ERROR_EOS;
        } else {
            YETI_Result result = map_error_code(GetSocketError());
            YETI_LOG_FINE_1("socket result = %d", result);
            return result;
        }
    }
    if (bytes_read) *bytes_read = nb_read;
    m_socket_fd_reference_->m_position_ += nb_read;
    return YETI_SUCCESS;
}

YETI_Result BsdSocketInputStream::get_size(YETI_LargeSize & size)
{
    size = 0;
    return YETI_ERROR_NOT_SUPPORTED;
}

#if defined(__PPU__)
YETI_Result BsdSocketInputStream::get_available(YETI_LargeSize&)
{
    return YETI_ERROR_NOT_SUPPORTED;
}
#else
YETI_Result BsdSocketInputStream::get_available(YETI_LargeSize & available)
{
    unsigned long ready = 0;
    int io_result = ioctlsocket(m_socket_fd_reference_->m_socket_fd_, FIONREAD, &ready);
    if (YETI_BSD_SOCKET_CALL_FAILED(io_result)) {
        available = 0;
        return YETI_ERROR_SOCKET_CONTROL_FAILED;
    } else {
        available = ready;
        return YETI_SUCCESS;
    }
}
#endif

class BsdSocketOutputStream : public OutputStream, private BsdSocketStream
{
public:
    BsdSocketOutputStream(BsdSocketFdReference & socket_fd)
        : BsdSocketStream(socket_fd) {}

    YETI_Result write(const void * buffer, YETI_Size bytes_to_write, YETI_Size * bytes_written /* = NULL */);
    YETI_Result seek(YETI_Position offset) {
        return BsdSocketStream::seek(offset);
    }
    YETI_Result tell(YETI_Position & where) {
        return BsdSocketStream::tell(where);
    }
    YETI_Result flush();
};

YETI_Result BsdSocketOutputStream::write(const void * buffer, YETI_Size bytes_to_write, YETI_Size * bytes_written )
{
    if (m_socket_fd_reference_->m_write_timeout_) {
        YETI_Result result = m_socket_fd_reference_->wait_until_writeable();
        if (result != YETI_SUCCESS) return result;
    }

    int flags = 0;
#if defined(MSG_NOSIGNAL)
    flags |= MSG_NOSGNAL;
#endif
    YETI_LOG_FINEST_1("writing %d to socket", (int)bytes_to_write);
    ssize_t nb_written = send(m_socket_fd_reference_->m_socket_fd_, (SocketConstBuffer)buffer, bytes_to_write, flags);
    YETI_LOG_FINEST_1("send returned %d", (int)nb_written);

    if (nb_written <= 0) {
        if (bytes_written) *bytes_written = 0;
        if (m_socket_fd_reference_->m_cancelled_) return YETI_ERROR_CANCELLED;

        if (nb_written == 0) {
            YETI_LOG_FINE("connection reset");
            return YETI_ERROR_CONNECTION_RESET;
        } else {
            YETI_Result result = map_error_code(GetSocketError());
            YETI_LOG_FINE_1("socket result = %d", result);
            return result;
        }
    }

    if (bytes_written) *bytes_written = nb_written;
    m_socket_fd_reference_->m_position_ += nb_written;

    return YETI_SUCCESS;
}

YETI_Result BsdSocketOutputStream::flush()
{
    int args = 0;
    socklen_t size = sizeof(args);

    YETI_LOG_FINEST("flushing socket");

    if (getsockopt(m_socket_fd_reference_->m_socket_fd_, IPPROTO_TCP, TCP_NODELAY, (char *)&args, &size)) {
        return YETI_ERROR_GETSOCKOPT_FAILED;
    }

    if (args == 1) return YETI_SUCCESS;

    args = 1;
    if (setsockopt(m_socket_fd_reference_->m_socket_fd_, IPPROTO_TCP, TCP_NODELAY, (const char *)&args, sizeof(args))) {
        return YETI_ERROR_SETSOCKOPT_FAILED;
    }

    int flags = 0;
#if defined(MSG_NOSIGNAL)
    flags |= MSG_NOSIGNAL;
#endif
    char dummy = 0;
    send(m_socket_fd_reference_->m_socket_fd_, &dummy, 0, flags);
    args = 0;
    if (setsockopt(m_socket_fd_reference_->m_socket_fd_, IPPROTO_TCP, TCP_NODELAY, (const char *)&args, sizeof(args))) {
        return YETI_ERROR_SETSOCKOPT_FAILED;
    }

    return YETI_SUCCESS;
}

class BsdSocket : public SocketInterface
{
public:
    BsdSocket(SocketFd fd, YETI_Flags flags);
    virtual ~BsdSocket();

    YETI_Result refresh_info();

    virtual YETI_Result bind(const SocketAddress & address, bool reuse_address = true);
    virtual YETI_Result connect(const SocketAddress & address, YETI_Timeout timeout);
    virtual YETI_Result wait_for_connection(YETI_Timeout timeout);
    virtual YETI_Result get_input_stream(InputStreamReference & stream);
    virtual YETI_Result get_output_stream(OutputStreamReference & stream);
    virtual YETI_Result get_info(SocketInfo & info);
    virtual YETI_Result set_read_timeout(YETI_Timeout timeout);
    virtual YETI_Result set_write_timeout(YETI_Timeout timeout);
    virtual YETI_Result cancel(bool shutdown);

protected:
    BsdSocketFdReference m_socket_fd_reference_;
    SocketInfo m_info_;
};

BsdSocket::BsdSocket(SocketFd fd, YETI_Flags flags)
: m_socket_fd_reference_(new BsdSocketFd(fd, flags))
{
#if defined(SO_NOSIGPIPE)
    int option = 1;
    setsockopt(m_socket_fd_reference_->m_socket_fd_, SOL_SOCKET, SO_NOSIGPIPE, (SocketOption)&option, sizeof(option));
#elif defined(SIGPIPE)
    signal(SIGPIPE, SIG_IGN);
#endif
    refresh_info();
}

BsdSocket::~BsdSocket()
{
    m_socket_fd_reference_ = NULL;
}

YETI_Result BsdSocket::bind(const SocketAddress & address, bool reuse_address /* = true */)
{
#if !defined(__WIN32__) && !defined(_XBOX)
    int option_ra = 1;
    setsockopt(m_socket_fd_reference_->m_socket_fd_, SOL_SOCKET, SO_REUSEADDR, (SocketOption)&option_ra, sizeof(option_ra));
#endif

    if (reuse_address) {
        int option = 1;
#if defined(SO_REUSEPORT)
        setsockopt(m_socket_fd_reference_->m_socket_fd_, SOL_SOCKET, SO_REUSEPORT, (SocketOption)&option, sizeof(option));
#else
        setsockopt(m_socket_fd_reference_->m_socket_fd_, SOL_SOCKET, SO_REUSEADDR, (SocketOption)&option, sizeof(option));
#endif
    }

    struct sockaddr_in inet_address;
    socket_address_to_inet_address(address, &inet_address);

#if defined(_XBOX)
    if( address.get_ipaddress().as_long() != IpAddress::Any.as_long() ) {
        socket_address_to_inet_address(SocketAddress(IpAddress::Any, address.get_port()), &inet_address);
    }
#endif

    if (::bind(m_socket_fd_reference_->m_socket_fd_, (struct sockaddr *)&inet_address, sizeof(inet_address))  < 0) {
        return map_error_code(GetSocketError());
    }

    refresh_info();

    return YETI_SUCCESS;
}

YETI_Result BsdSocket::connect(const SocketAddress & address, YETI_Timeout timeout)
{
    return YETI_ERROR_NOT_SUPPORTED;
}

YETI_Result BsdSocket::wait_for_connection(YETI_Timeout timeout)
{
    return YETI_ERROR_NOT_SUPPORTED;
}

YETI_Result BsdSocket::get_input_stream(InputStreamReference & stream)
{
    stream = NULL;
    if (m_socket_fd_reference_.is_null()) return YETI_ERROR_INVALID_STATE;

    stream = new BsdSocketInputStream(m_socket_fd_reference_);

    return YETI_SUCCESS;
}

YETI_Result BsdSocket::get_output_stream(OutputStreamReference & stream)
{
    stream = NULL;

    if (m_socket_fd_reference_.is_null()) return YETI_ERROR_INVALID_STATE;

    stream = new BsdSocketOutputStream(m_socket_fd_reference_);

    return YETI_SUCCESS;
}

YETI_Result BsdSocket::get_info(SocketInfo & info)
{
    info = m_info_;
    return YETI_SUCCESS;
}

YETI_Result BsdSocket::refresh_info()
{
    if (m_socket_fd_reference_.is_null()) return YETI_ERROR_INVALID_STATE;

    struct sockaddr_in inet_address;
    socklen_t name_length = sizeof(inet_address);

    if (getsockname(m_socket_fd_reference_->m_socket_fd_, (struct sockaddr *)&inet_address, &name_length) == 0) {
        m_info_.local_address.set_ipaddress(ntohl(inet_address.sin_addr.s_addr));
        m_info_.local_address.set_port(ntohs(inet_address.sin_port));
    }

    if (getpeername(m_socket_fd_reference_->m_socket_fd_, (struct sockaddr *)&inet_address, &name_length) == 0) {
        m_info_.remote_address.set_ipaddress(ntohl(inet_address.sin_addr.s_addr));
        m_info_.remote_address.set_port(ntohs(inet_address.sin_port));
    }

    return YETI_SUCCESS;
}

YETI_Result BsdSocket::set_read_timeout(YETI_Timeout timeout)
{
    m_socket_fd_reference_->m_read_timeout_ = timeout;
    return YETI_SUCCESS;
}

YETI_Result BsdSocket::set_write_timeout(YETI_Timeout timeout)
{
    m_socket_fd_reference_->m_write_timeout_ = timeout;
    ::setsockopt(m_socket_fd_reference_->m_socket_fd_, SOL_SOCKET, SO_SNDTIMEO, (SocketOption)&timeout, sizeof(timeout));

    return YETI_SUCCESS;
}

YETI_Result BsdSocket::cancel(bool shutdown)
{
    m_socket_fd_reference_->m_cancelled_ = true;
    if (shutdown) {
        int result = ::shutdown(m_socket_fd_reference_->m_socket_fd_, SHUT_RDWR);
        if (YETI_BSD_SOCKET_CALL_FAILED(result)) {
            YETI_LOG_FINE_1("shutdown faild (%d)", map_error_code(GetSocketError()));
        }
    }

#if !defined(__WINSOCK__)
    if (m_socket_fd_reference_->m_cancelled_) {
        char dummy = 0;
        ::send(m_socket_fd_reference_->m_cancel_fds_[0], &dummy, 1, 0);
    }
#else
    closesocket(m_socket_fd_reference_->m_socket_fd_);
#endif

    return YETI_SUCCESS;
}

Socket::~Socket()
{
    delete m_socket_delegate_;
}

class BsdUdpSocket : public UdpSocketInterface, protected BsdSocket
{
public:
    BsdUdpSocket(YETI_Flags flags);
    virtual ~BsdUdpSocket() {}

    virtual YETI_Result bind(const SocketAddress & address, bool reuse_address = true);
    virtual YETI_Result connect(const SocketAddress & address, YETI_Timeout timeout);

    virtual YETI_Result send(const DataBuffer & packet,
        const SocketAddress * address);
    virtual YETI_Result receive(DataBuffer & packet,
        SocketAddress * address);

    friend class UdpSocket;
};

BsdUdpSocket::BsdUdpSocket(YETI_Flags flags)
: BsdSocket(::socket(AF_INET, SOCK_DGRAM, 0), flags)
{
    int option = 1;
    ::setsockopt(m_socket_fd_reference_->m_socket_fd_, SOL_SOCKET, SO_BROADCAST, (SocketOption)&option, sizeof(option));
#if defined(_XBOX)
    if (!YETI_BSD_SOCKET_IS_INVALID(m_socket_fd_reference_->m_socket_fd_)) {
        *(DWORD *)((char *)m_socket_fd_reference_->m_socket_fd_ + 0xc) |= 0x02000000;
    }
#endif
}

YETI_Result BsdUdpSocket::bind(const SocketAddress & address, bool reuse_address /* = true */)
{
    if (reuse_address) {
#if defined(SO_REUSEPORT)
    YETI_LOG_FINE("etting SO_REUSEPORT option on socket");
    int option = 1;
    setsockopt(m_socket_fd_reference_->m_socket_fd_, 
        SOL_SOCKET, 
        SO_REUSEPORT, 
        (SocketOption)&option, 
        sizeof(option))
#endif
    }

    return BsdSocket::bind(address, reuse_address);
}

YETI_Result BsdUdpSocket::connect(const SocketAddress & address, YETI_Timeout timeout)
{
    struct sockaddr_in inet_address;
    socket_address_to_inet_address(address, &inet_address);

    YETI_LOG_FINER_2("connecting to %s, port %d", address.get_ipaddress().to_string().get_chars(), address.get_port());

    int io_result = ::connect(m_socket_fd_reference_->m_socket_fd_, (struct sockaddr *)&inet_address, sizeof(inet_address));
    if (YETI_BSD_SOCKET_CALL_FAILED(io_result)) {
        YETI_Result result = map_error_code(GetSocketError());
        YETI_LOG_FINE_1("socket error %d", result);
        return result;
    }

    refresh_info();

    return YETI_SUCCESS;
}

YETI_Result BsdUdpSocket::send(const DataBuffer & packet, const SocketAddress * address)
{
    const YETI_Byte * buffer = packet.get_data();
    ssize_t buffer_length = packet.get_data_size();

    if (m_socket_fd_reference_->m_write_timeout_) {
        YETI_Result result = m_socket_fd_reference_->wait_until_writeable();
        if (result != YETI_SUCCESS) return result;
    }

    int io_result;
    if (address) {
        struct sockaddr_in inet_address;
        socket_address_to_inet_address(*address, &inet_address);
        YETI_LOG_FINEST_2("sending datagram to %s port %d", address->get_ipaddress().to_string().get_chars(), address->get_port());
        io_result = ::sendto(m_socket_fd_reference_->m_socket_fd_, (SocketConstBuffer)buffer, buffer_length, 0, (struct sockaddr *)&inet_address, sizeof(inet_address));
    } else {
        int flags = 0;
#if defined(MSG_NOSIGNAL)
        flags |= MSG_NOSIGNAL;
#endif
        YETI_LOG_FINEST("sending datagram");
        io_result = ::send(m_socket_fd_reference_->m_socket_fd_, (SocketConstBuffer)buffer, buffer_length, flags);
    }

    YETI_LOG_FINEST_1("send/sendto returned %d", (int)io_result);
    if (m_socket_fd_reference_->m_cancelled_) return YETI_ERROR_CANCELLED;
    if (YETI_BSD_SOCKET_CALL_FAILED(io_result)) {
        YETI_Result result = map_error_code(GetSocketError());
        YETI_LOG_FINE_1("socket error %d", result);
        return result;
    }

    m_socket_fd_reference_->m_position_ += buffer_length;
    return YETI_SUCCESS;
}

YETI_Result BsdUdpSocket::receive(DataBuffer & packet, SocketAddress * address)
{
    YETI_Byte * buffer = packet.use_data();
    ssize_t buffer_size = packet.get_buffer_size();

    if (buffer_size == 0) return YETI_ERROR_INVALID_PARAMETERS;

    if (m_socket_fd_reference_->m_read_timeout_) {
        YETI_Result result = m_socket_fd_reference_->wait_until_readable();
        if (result != YETI_SUCCESS) return result;
    }

    int io_result = 0;
    if (address) {
        struct sockaddr_in inet_address;
        socklen_t inet_address_length = sizeof(inet_address);

        io_result = ::recvfrom(m_socket_fd_reference_->m_socket_fd_, (SocketBuffer)buffer, buffer_size, 0, (struct sockaddr *)&inet_address, &inet_address_length);

        if (!YETI_BSD_SOCKET_CALL_FAILED(io_result)) {
            if (inet_address_length == sizeof(inet_address)) {
                inet_address_to_socket_address(&inet_address, *address);
            }
        }

        YETI_LOG_FINEST_2("receiving datagram from %s port %d", 
            address->get_ipaddress().to_string().get_chars(), address->get_port());
    } else {
        YETI_LOG_FINEST("receiving datagram");
        io_result = ::recv(m_socket_fd_reference_->m_socket_fd_, (SocketBuffer)buffer, buffer_size, 0);
    }

    YETI_LOG_FINEST_1("recv/recvfrom returned %d", (int)io_result);
    if (m_socket_fd_reference_->m_cancelled_) {
        packet.set_data_size(0);
        return YETI_ERROR_CANCELLED;
    }
    if (YETI_BSD_SOCKET_CALL_FAILED(io_result)) {
        YETI_Result result = map_error_code(GetSocketError());
        YETI_LOG_FINE_1("socket error %d", result);
        packet.set_data_size(0);
        return result;
    }

    packet.set_data_size(io_result);
    m_socket_fd_reference_->m_position_ += io_result;
    
    return YETI_SUCCESS;
}

UdpSocket::UdpSocket(YETI_Flags flags /* = YETI_SOCKET_FLAG_CANCELLABLE */)
{
    BsdUdpSocket * delegate = new BsdUdpSocket(flags);
    m_socket_delegate_ = delegate;
    m_udpsocket_delegate_ = delegate;
}

UdpSocket::UdpSocket(UdpSocketInterface * delegate)
: m_udpsocket_delegate_(delegate)
{
}

UdpSocket::~UdpSocket()
{
    delete m_udpsocket_delegate_;
    m_udpsocket_delegate_ = NULL;
    m_socket_delegate_ = NULL;
}

class BsdUdpMulticastSocket : public UdpMulticastSocketInterface, protected BsdUdpSocket
{
public:
    BsdUdpMulticastSocket(YETI_Flags flags);
    ~BsdUdpMulticastSocket() {}

    virtual YETI_Result join_group(const IpAddress & group,
        const IpAddress & iface);
    virtual YETI_Result leave_group(const IpAddress & group,
        const IpAddress & iface);
    virtual YETI_Result set_timetolive(unsigned char ttl);
    virtual YETI_Result set_interface(const IpAddress & iface);

    friend class UdpMulticastSocket;
};

BsdUdpMulticastSocket::BsdUdpMulticastSocket(YETI_Flags flags)
: BsdUdpSocket(flags)
{
#if !defined(_XBOX)
    int option = 1;
    ::setsockopt(m_socket_fd_reference_->m_socket_fd_, IPPROTO_IP, IP_MULTICAST_LOOP, (SocketOption)&option, sizeof(option));
#endif
}

#if defined(_XBOX)
YETI_Result BsdUdpMulticastSocket::join_group(const IpAddress & group,
                                             const IpAddress & iface)
{
    return YETI_SUCCESS;
}
#else
YETI_Result BsdUdpMulticastSocket::join_group(const IpAddress & group,
                                             const IpAddress & iface)
{
    struct ip_mreq mreq;
    mreq.imr_interface.s_addr = htonl(iface.as_long());
    mreq.imr_multiaddr.s_addr = htonl(group.as_long());

    YETI_LOG_FINE_2("joining multicast addr %s group %s", 
        iface.to_string().get_chars(), group.to_string().get_chars());

    int io_result = ::setsockopt(m_socket_fd_reference_->m_socket_fd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (SocketOption)&mreq, sizeof(mreq));

    if (io_result == 0) {
        return YETI_SUCCESS;
    } else {
        YETI_Result result = map_error_code(GetSocketError());
        YETI_LOG_FINE_1("setsockopt error %d", result);
        return result;
    }
}
#endif

#if defined(_XBOX)
YETI_Result BsdUdpMulticastSocket::leave_group(const IpAddress & group, const IpAddress & iface)
{
    return YETI_SUCCESS;
}
#else
YETI_Result BsdUdpMulticastSocket::leave_group(const IpAddress & group, const IpAddress & iface)
{
    struct ip_mreq mreq;
    mreq.imr_interface.s_addr = htonl(iface.as_long());
    mreq.imr_multiaddr.s_addr = htonl(group.as_long());

    YETI_LOG_FINE_2("leaving multicast addr %s group %s", 
        iface.to_string().get_chars(), group.to_string().get_chars());

    int io_result = ::setsockopt(m_socket_fd_reference_->m_socket_fd_, IPPROTO_IP, IP_DROP_MEMBERSHIP, (SocketOption)&mreq, sizeof(mreq));

    if (io_result == 0) {
        return YETI_SUCCESS;
    } else {
        YETI_Result result = map_error_code(GetSocketError());
        YETI_LOG_FINE_1("setsockopt error %d", result);
        return result;
    }
}
#endif

#if defined(_XBOX)
YETI_Result BsdUdpMulticastSocket::set_interface(const IpAddress & iface)
{
    return YETI_SUCCESS;
}
#else
YETI_Result BsdUdpMulticastSocket::set_interface(const IpAddress & iface)
{
    struct in_addr iface_addr;
    iface_addr.s_addr = htonl(iface.as_long());

    YETI_LOG_FINE_1("setting multicast interface %s", iface.to_string().get_chars()); 
    int io_result = ::setsockopt(m_socket_fd_reference_->m_socket_fd_, IPPROTO_IP, IP_MULTICAST_IF, (char *)&iface_addr, sizeof(iface_addr));
    if (io_result == 0) {
        return YETI_SUCCESS;
    } else {
        YETI_Result result = map_error_code(GetSocketError());
        YETI_LOG_FINE_1("setsockopt error %d", result);
        return result;
    }
}
#endif

#if defined(_XBOX)
YETI_Result BsdUdpMulticastSocket::set_timetolive(unsigned char ttl)
{
    return YETI_ERROR_NOT_IMPLEMENTED;
}
#else
YETI_Result BsdUdpMulticastSocket::set_timetolive(unsigned char ttl)
{
    unsigned char ttl_opt = ttl;

    YETI_LOG_FINE_1("setting multicast TTL to %d", (int)ttl);
    int io_result = ::setsockopt(m_socket_fd_reference_->m_socket_fd_, IPPROTO_IP, IP_MULTICAST_TTL, (SocketOption)&ttl_opt, sizeof(ttl_opt));
    if (io_result == 0) {
        return YETI_SUCCESS;
    } else {
        YETI_Result result = map_error_code(GetSocketError());
        YETI_LOG_FINE_1("setsockopt error %d", result);
        return result;
    }
}
#endif

UdpMulticastSocket::UdpMulticastSocket(YETI_Flags flags /* = YETI_SOCKET_FLAG_CANCELLABLE */)
{
    BsdUdpMulticastSocket * delegate = new BsdUdpMulticastSocket(flags);
    m_socket_delegate_ = delegate;
    m_udpsocket_delegate_ = delegate;
    m_udpmulticastsocket_delegate_ = delegate;
}

UdpMulticastSocket::~UdpMulticastSocket()
{
    delete m_udpmulticastsocket_delegate_;

    m_socket_delegate_ = NULL;
    m_udpsocket_delegate_ = NULL;
    m_udpmulticastsocket_delegate_ = NULL;
}

class BsdTcpClientSocket : protected BsdSocket
{
public:
    BsdTcpClientSocket(YETI_Flags flags)
        : BsdSocket(::socket(AF_INET, SOCK_STREAM, 0), flags) {}
    ~BsdTcpClientSocket() {}

    YETI_Result connect(const SocketAddress & address, YETI_Timeout timeout);
    YETI_Result wait_for_connection(YETI_Timeout timeout);

protected:
    friend class TcpClientSocket;
};

YETI_Result BsdTcpClientSocket::connect(const SocketAddress & address, YETI_Timeout timeout)
{
    struct sockaddr_in inet_address;
    socket_address_to_inet_address(address, &inet_address);

    int io_result = ::connect(m_socket_fd_reference_->m_socket_fd_, (struct sockaddr *)&inet_address, sizeof(inet_address));
    if (io_result == 0) {
        YETI_LOG_FINE("immediate connection");
        refresh_info();
        return YETI_SUCCESS;
    }

    YETI_Result result = map_error_code(GetSocketError());
    if (timeout && result == YETI_ERROR_WOULD_BLOCK) {
        return wait_for_connection(timeout);
    }

    return result;
}

YETI_Result BsdTcpClientSocket::wait_for_connection(YETI_Timeout timeout)
{
    YETI_Result result = m_socket_fd_reference_->_wait_for_condition(true, true, true, timeout);

    refresh_info();
    
    return result;
}

TcpClientSocket::TcpClientSocket(YETI_Flags flags /* = YETI_SOCKET_FLAG_CANCELLABLE */)
: Socket(new BsdTcpClientSocket(flags))
{
}

TcpClientSocket::~TcpClientSocket()
{
    delete m_socket_delegate_;
    m_socket_delegate_ = NULL;
}

class BsdTcpServerSocket : public TcpServerSocketInterface, protected BsdSocket
{
public:
    BsdTcpServerSocket(YETI_Flags flags)
        : BsdSocket(socket(AF_INET, SOCK_STREAM, 0), flags), m_listen_max_(0) {}
    ~BsdTcpServerSocket() {}

    YETI_Result get_input_stream(InputStreamReference & stream) {
        stream = NULL;
        return YETI_ERROR_NOT_SUPPORTED;
    }

    YETI_Result get_output_stream(OutputStreamReference & stream) {
        stream = NULL;
        return YETI_ERROR_NOT_SUPPORTED;
    }

    YETI_Result listen(unsigned int max_clients);
    YETI_Result wait_for_new_client(Socket *& client,
        YETI_Timeout timeout,
        YETI_Flags flags);

protected:
    unsigned int m_listen_max_;
    friend class TcpServerSocket;
};

YETI_Result BsdTcpServerSocket::listen(unsigned int max_clients)
{
    if (::listen(m_socket_fd_reference_->m_socket_fd_, max_clients) < 0) {
        m_listen_max_ = 0;
        return YETI_ERROR_LISTEN_FAILED;
    }
    m_listen_max_ = max_clients;
    return YETI_SUCCESS;
}

YETI_Result BsdTcpServerSocket::wait_for_new_client(Socket *& client,
                                YETI_Timeout timeout,
                                YETI_Flags flags)
{
    client = NULL;
    if (m_listen_max_ == 0) {
        listen(YETI_TCP_SERVER_SOCKET_DEFAULT_LISTEN_COUNT);
    }

    YETI_LOG_FINER("waiting until socket is readable or writeable");
    YETI_Result result = m_socket_fd_reference_->_wait_for_condition(true, true, false, timeout);
    if (result != YETI_SUCCESS) return result;
    YETI_LOG_FINER("accepting connection");

    struct sockaddr_in inet_address;
    socklen_t namelen = sizeof(inet_address);
    SocketFd socket_fd = ::accept(m_socket_fd_reference_->m_socket_fd_, (struct sockaddr *)&inet_address, &namelen);
    if (YETI_BSD_SOCKET_IS_INVALID(socket_fd)) {
        if (m_socket_fd_reference_->m_cancelled_) return YETI_ERROR_CANCELLED;
        result = map_error_code(GetSocketError());
        YETI_LOG_FINE_1("socket error %d", result);
        return result;
    } else {
        client = new Socket(new BsdSocket(socket_fd, flags));
    }

    return result;
}

TcpServerSocket::TcpServerSocket(YETI_Flags flags)
{
    BsdTcpServerSocket * delegate = new BsdTcpServerSocket(flags);
    m_socket_delegate_ = delegate;
    m_tcpserversocket_delegate_ = delegate;
}

TcpServerSocket::~TcpServerSocket()
{
    delete m_tcpserversocket_delegate_;
    m_socket_delegate_ = NULL;
    m_tcpserversocket_delegate_ = NULL;
}

NAMEEND
