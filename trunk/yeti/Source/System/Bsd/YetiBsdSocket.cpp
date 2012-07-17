#if (defined(_WIN32) || defined(_WIN32_WCE) || defined(_XBOX)) && !defined(__SYMBIAN32__)
#   if !defined(__WINSOCK__) 
#       define __WINSOCK__ 
#   endif
#endif

#if defined(__WINSOCK__) && !defined(_XBOX)
#   define STRICT
#   define NPT_WIN32_USE_WINSOCK2
#   ifdef NPT_WIN32_USE_WINSOCK2
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
#define NPT_BSD_SOCKET_IS_INVALID(_s)    ((_s) < 0)
#define NPT_BSD_SOCKET_CALL_FAILED(_e)   ((_e) < 0)
#define NPT_BSD_SOCKET_SELECT_FAILED(_e) ((_e) < 0)

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

static void socket_address_to_inet_address(const SocketAddress & socket_address,
                                           struct sockaddr_in * inet_address)
{
    for (int i = 0; i < 8; ++i) inet_address->sin_zero[i] = 0;
    inet_address->sin_family = AF_INET;
    inet_address->sin_port = htons(socket_address.get_port());
    inet_address->sin_addr.s_addr = htonl(socket_address.get_ipaddress().as_long());
}

NAMEEND
