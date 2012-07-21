#include "Yeti.h"

#if (defined(_WIN32) || defined(_WIN32_WCE) || defined(_XBOX)) && !defined(__SYMBIAN32__)
#   if !defined(__WINSOCK__)
#       define __WINSOCK__
#   endif
#endif

#if defined(__WINSOCK__) &&!defined(_XBOX)
#   define STRICT
#   define YETI_WIN32_USE_WINSOCK2
#   ifdef YETI_WIN32_USE_WINSOCK2
#       include <winsock2.h>
#       include <ws2tcpip.h>
#   else
#       include <winsock.h>
#   endif
#   include <windows.h>
#   include "YetiConfig.h"
#else
#   include <sys/types.h>
#   include <netinet/in.h>
#   include <netdb.h>
#   include <errno.h>
#endif
