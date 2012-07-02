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
#   include "YetiWin32Network.h"
NAMEBEG
static WinsockSystem & WinsockInitializer = WinsockSystem::Initializer;
NAMEEND
#else
#   include <sys/types.h>
#   include <netinet/in.h>
#   include <netdb.h>
#   include <errno.h>
#endif

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiNetwork.h"
#include "YetiUtil.h"
#include "YetiConstants.h"
#include "YetiResults.h"
#include "YetiSocket.h"

NAMEBEG

#if defined(YETI_CONFIG_HAVE_GETADDRINFO)

const unsigned int YETI_BSD_NETWORK_MAX_ADDR_LIST_LENGTH = 1024;

static YETI_Result map_get_addrinfo_errorcode(int error_code)
{
    switch (error_code) {
        case EAI_AGAIN:
            return YETI_ERROR_TIMEOUT;
        default:
            return YETI_ERROR_HOST_UNKNOWN;
    }
}

YETI_Result NetworkNameResolver::resolve(const char * name, List<IpAddress> & addresses, YETI_Timeout timeout /* = YETI_TIMEOUT_INFINITE */)
{
    addresses.clear();

    struct addrinfo * infos = NULL;
    int result = getaddrinfo(name, /* hostname */
        NULL, /* servname */
        NULL, /* hints */
        &infos /* res */);
    if (result != 0) {
        return map_get_addrinfo_errorcode(result);
    }

    for (struct addrinfo * info = infos; info && addresses.get_item_count() < YETI_BSD_NETWORK_MAX_ADDR_LIST_LENGTH; info = info->ai_next) {
        if (info->ai_family != AF_INET) continue;
        if (info->ai_addrlen != sizeof(struct sockaddr_in)) continue;
        if (info->ai_protocol != 0 && info->ai_protocol != IPPROTO_TCP) continue;
        struct sockaddr_in * inet_addr = (struct sockaddr_in *)info->ai_addr;
        IpAddress address(ntohl(inet_addr->sin_addr.s_addr));
        addresses.add(address);
    }

    freeaddrinfo(infos);

    return YETI_SUCCESS;
}

#endif

NAMEEND
