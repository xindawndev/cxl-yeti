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

//YETI_SET_LOCAL_LOGGER("yeti.sockets.bsd")

NAMEBEG

NAMEEND
