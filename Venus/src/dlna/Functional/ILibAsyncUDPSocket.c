#ifdef MEMORY_CHECK
    #include <assert.h>
    #define MEMCHECK(x) x
#else
    #define MEMCHECK(x)
#endif

#if defined(WIN32)
    #define _CRTDBG_MAP_ALLOC
#endif

#if defined(WINSOCK2)
    #include <winsock2.h>
    #include <ws2tcpip.h>
#elif defined(WINSOCK1)
    #include <winsock.h>
    #include <wininet.h>
#endif
#include "ILibParsers.h"
#include "ILibAsyncUDPSocket.h"
#include "ILibAsyncSocket.h"

#if defined(WIN32) && !defined(_WIN32_WCE)
    #include <crtdbg.h>
#endif

struct ILibAsyncUDPSocket_Data
{
    void *user1;
    void *user2;

    ILibAsyncSocket_SocketModule UDPSocket;
    unsigned short BoundPortNumber;

    ILibAsyncUDPSocket_OnData OnData;
    ILibAsyncUDPSocket_OnSendOK OnSendOK;
};

void ILibAsyncUDPSocket_OnDataSink(ILibAsyncSocket_SocketModule socketModule,char* buffer,int *p_beginPointer, int endPointer,ILibAsyncSocket_OnInterrupt* OnInterrupt, void **user, int *PAUSE)
{
    struct ILibAsyncUDPSocket_Data *data = (struct ILibAsyncUDPSocket_Data*)*user;

    if (data->OnData != NULL) {
        data->OnData(
            socketModule,
            buffer, 
            endPointer,
            ILibAsyncSocket_GetRemoteInterface(socketModule),
            ILibAsyncSocket_GetRemotePort(socketModule),
            data->user1, 
            data->user2, 
            PAUSE);
    }
    *p_beginPointer = endPointer;
}

void ILibAsyncUDPSocket_OnSendOKSink(ILibAsyncSocket_SocketModule socketModule, void *user)
{
    struct ILibAsyncUDPSocket_Data *data = (struct ILibAsyncUDPSocket_Data*)user;
    if (data->OnSendOK != NULL) {
        data->OnSendOK(socketModule, data->user1, data->user2);
    }
}

void ILibAsyncUDPSocket_OnDisconnect(ILibAsyncSocket_SocketModule socketModule, void *user)
{
    free(user);
}

ILibAsyncUDPSocket_SocketModule ILibAsyncUDPSocket_CreateEx(void *Chain, int BufferSize, int localInterface, unsigned short localPortStartRange, unsigned short localPortEndRange, enum ILibAsyncUDPSocket_Reuse reuse, ILibAsyncUDPSocket_OnData OnData, ILibAsyncUDPSocket_OnSendOK OnSendOK, void *user)
{
    void *RetVal = NULL;
    struct ILibAsyncUDPSocket_Data *data = (struct ILibAsyncUDPSocket_Data*)malloc(sizeof(struct ILibAsyncUDPSocket_Data));
#if defined(WIN32) || defined(_WIN32_WCE)
    SOCKET newSocket;
#else
    int newSocket;
#endif
    struct sockaddr_in local;
    int ra = (int)reuse;
    int rv;
    int count = 0;

    memset(data,0,sizeof(struct ILibAsyncUDPSocket_Data));
    data->OnData = OnData;
    data->OnSendOK = OnSendOK;
    data->user1 = user;


    newSocket = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&local,0,sizeof(struct sockaddr_in));

    local.sin_addr.s_addr = localInterface;
    local.sin_family = AF_INET;
    local.sin_port = htons(localPortStartRange);

#if defined(__APPLE__)
    if (setsockopt((int)*TheSocket, SOL_SOCKET, SO_REUSEPORT, (char*)&ra, sizeof(ra)) < 0)
#else
    rv=setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,(char*)&ra, sizeof(ra));
#endif

    if (localPortStartRange!=localPortEndRange) {
        do {
            // Choose a random port from 50000 to 65500, which is what IANA says to use
            // for non standard ports
            if (++count >= 20) break;
            local.sin_port =  htons((unsigned short)(localPortStartRange + ((unsigned short)rand() % (localPortEndRange-localPortStartRange))));
        } while(bind(newSocket, (struct sockaddr *) &(local), sizeof(local))!=0);
        if (count >= 20) {
            // 超出绑定次数
#if defined(WIN32)
            closesocket(newSocket);
#else
            close(newSocket);
#endif
            freesafe(data);
            return NULL;
        }
    } else if(bind(newSocket, (struct sockaddr *) &(local), sizeof(local))!=0) {
        // Could not bind to this port
#if defined(WIN32)
        closesocket(newSocket);
#else
        close(newSocket);
#endif
        freesafe(data);
        return (NULL);
    }

    data->BoundPortNumber = ntohs(local.sin_port);

    RetVal = ILibCreateAsyncSocketModule(Chain,BufferSize,&ILibAsyncUDPSocket_OnDataSink,NULL,&ILibAsyncUDPSocket_OnDisconnect,&ILibAsyncUDPSocket_OnSendOKSink);
    ILibAsyncSocket_UseThisSocket(RetVal,&newSocket,&ILibAsyncUDPSocket_OnDisconnect,data);
    return(RetVal);
}

/*! \fn int ILibAsyncUDPSocket_JoinMulticastGroup(ILibAsyncUDPSocket_SocketModule module, int localInterface, int remoteInterface)
    \brief Joins a multicast group
    \param module The ILibAsyncUDPSocket_SocketModule to join the multicast group
    \param localInterface The local IP address in network order, to join the multicast group
    \param remoteInterface The multicast ip address in network order, to join
    \returns 0 = Success, Nonzero = Failure
*/
int ILibAsyncUDPSocket_JoinMulticastGroup(ILibAsyncUDPSocket_SocketModule module, int localInterface, int remoteInterface)
{
    struct ip_mreq mreq;
#if defined(WIN32) || defined(_WIN32_WCE)
    SOCKET s = *((SOCKET*)ILibAsyncSocket_GetSocket(module));
#else
    int s = *((int*)ILibAsyncSocket_GetSocket(module));
#endif
    int RetVal;

    mreq.imr_multiaddr.s_addr = remoteInterface;
    mreq.imr_interface.s_addr = localInterface;
    
    // Join the multicast group
    RetVal = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,(char*)&mreq, sizeof(mreq));
    if (RetVal == 0) {
        ILibAsyncSocket_SetLocalInterface2(module,localInterface);
    } else {
        ILibAsyncSocket_SetLocalInterface2(module,0);
    }
    return (RetVal);
}

/*! \fn int ILibAsyncUDPSocket_SetMulticastInterface(ILibAsyncUDPSocket_SocketModule module, int localInterface)
    \brief Sets the local interface to use, when multicasting
    \param module The ILibAsyncUDPSocket_SocketModule handle to set the interface on
    \param localInterface The local IP address in network order, to use when multicasting
    \returns 0 = Success, Nonzero = Failure
*/
int ILibAsyncUDPSocket_SetMulticastInterface(ILibAsyncUDPSocket_SocketModule module, int localInterface)
{
#if !defined(_WIN32_WCE) || (defined(_WIN32_WCE) && _WIN32_WCE>=400)
    struct in_addr interface_addr;
#if defined(WIN32) || defined(_WIN32_WCE)
    SOCKET s = *((SOCKET*)ILibAsyncSocket_GetSocket(module));
#else
    int s = *((int*)ILibAsyncSocket_GetSocket(module));
#endif
    memset((char *)&interface_addr, 0, sizeof(interface_addr));

    interface_addr.s_addr = localInterface;
    return(setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF,(char*)&interface_addr, sizeof(interface_addr)));
#else
    return(1);
#endif
}

/*! \fn int ILibAsyncUDPSocket_SetMulticastTTL(ILibAsyncUDPSocket_SocketModule module, unsigned char TTL)
    \brief Sets the Multicast TTL value
    \param module The ILibAsyncUDPSocket_SocketModule handle to set the Multicast TTL value
    \param TTL The Multicast-TTL value to use
    \returns 0 = Success, Nonzero = Failure
*/
int ILibAsyncUDPSocket_SetMulticastTTL(ILibAsyncUDPSocket_SocketModule module, unsigned char TTL)
{
#if defined(WIN32) || defined(_WIN32_WCE)
    SOCKET s = *((SOCKET*)ILibAsyncSocket_GetSocket(module));
#else
    int s = *((int*)ILibAsyncSocket_GetSocket(module));
#endif
    return(setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL,(char*)&TTL, sizeof(TTL)));
}
