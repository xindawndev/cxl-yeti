#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
//#include <net/if_arp.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiStreams.h"
#include "YetiThreads.h"
#include "YetiNetwork.h"
#include "YetiUtil.h"
#include "YetiConstants.h"
#include "YetiSocket.h"

#if defined(YETI_CONFIG_HAVE_NET_IF_DL_H)
#include <net/if_dl.h>
#endif
#if defined(YETI_CONFIG_HAVE_NET_IF_TYPES_H)
#include <net/if_types.h>
#endif

#if !defined(IFHWADDRLEN)
#define IFHWADDRLEN 6 // default to 48 bits
#endif
#if !defined(ARPHRD_ETHER)
#define ARPHRD_ETHER 1
#endif

#if defined(_SIZEOF_ADDR_IFREQ)
#define YETI_IFREQ_SIZE(ifr) _SIZEOF_ADDR_IFREQ(*ifr)
#elif defined(YETI_CONFIG_HAVE_SOCKADDR_SA_LEN)
#define YETI_IFREQ_SIZE(ifr) (sizeof(ifr->ifr_name) + ifr->ifr_addr.sa_len)
#else
#define YETI_IFREQ_SIZE(ifr) sizeof(*ifr)
#endif
