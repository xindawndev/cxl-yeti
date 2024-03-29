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
#include "ILibAsyncServerSocket.h"
#include "ILibAsyncSocket.h"

#define DEBUGSTATEMENT(x)

#if defined(WIN32) && !defined(_WIN32_WCE)
#include <crtdbg.h>
#endif

struct ILibAsyncServerSocketModule
{
    void (*PreSelect)(void* object,void *readset, void *writeset, void *errorset, int* blocktime);
    void (*PostSelect)(void* object,int slct, void *readset, void *writeset, void *errorset);
    void (*Destroy)(void* object);
    void *Chain;

    int MaxConnection;
    void **AsyncSockets;
    ILibServerScope scope;

#ifdef _WIN32_WCE
    SOCKET ListenSocket;
#elif WIN32
    SOCKET ListenSocket;
#elif defined(_POSIX)
    int ListenSocket;
#endif
    unsigned short portNumber;
    int listening;

    ILibAsyncServerSocket_OnReceive OnReceive;
    ILibAsyncServerSocket_OnConnect OnConnect;
    ILibAsyncServerSocket_OnDisconnect OnDisconnect;
    ILibAsyncServerSocket_OnInterrupt OnInterrupt;
    ILibAsyncServerSocket_OnSendOK OnSendOK;

    void *Tag;
};
struct ILibAsyncServerSocket_Data
{
    struct ILibAsyncServerSocketModule *module;
    ILibAsyncServerSocket_BufferReAllocated Callback;
    void *user;
};

void *ILibAsyncServerSocket_GetTag(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule)
{
    struct ILibAsyncServerSocketModule *module = (struct ILibAsyncServerSocketModule*)ILibAsyncSocketModule;
    return (module->Tag);
}

void ILibAsyncServerSocket_SetTag(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule, void *tag)
{
    struct ILibAsyncServerSocketModule *module = (struct ILibAsyncServerSocketModule*)ILibAsyncSocketModule;
    module->Tag = tag;
}

void ILibAsyncServerSocket_OnInterruptSink(ILibAsyncSocket_SocketModule socketModule, void *user)
{
    struct ILibAsyncServerSocket_Data *data = (struct ILibAsyncServerSocket_Data*)user;
    if (data->module->OnInterrupt != NULL) {
        data->module->OnInterrupt(data->module,socketModule,data->user);
    }
    free(user);
}

void ILibAsyncServerSocket_PreSelect(void * socketModule, void *readset, void *writeset, void *errorset, int* blocktime)
{
    struct ILibAsyncServerSocketModule *module = (struct ILibAsyncServerSocketModule*)socketModule;
    int flags, i;

    // 没有开始监听
    if (module->listening == 0) {
        // Set the socket to non-block mode, so we can play nice and share the thread
#ifdef _WIN32_WCE
        flags = 1;
        ioctlsocket(module->ListenSocket,FIONBIO,&flags);
#elif WIN32
        flags = 1;
        ioctlsocket(module->ListenSocket, FIONBIO, &flags);
#elif _POSIX
        flags = fcntl(module->ListenSocket,F_GETFL,0);
        fcntl(module->ListenSocket,F_SETFL,O_NONBLOCK|flags);
#endif

        // Put the socket in Listen, and add it to the fdset for the Select loop
        module->listening = 1;
        listen(module->ListenSocket, 4);
        FD_SET(module->ListenSocket, (fd_set*)readset);
    } else {
        // Only put the ListenSocket in the readset, if we are able to handle a new socket
        for (i = 0; i < module->MaxConnection; ++i) {
            if (ILibAsyncSocket_IsFree(module->AsyncSockets[i]) != 0) {
                FD_SET(module->ListenSocket, (fd_set*)readset);
                break;
            }
        }
    }
}

// 设置接收缓冲区大小变更回调函数
void ILibAsyncServerSocket_SetReAllocateNotificationCallback(ILibAsyncServerSocket_ServerModule AsyncServerSocketToken, ILibAsyncServerSocket_ConnectionToken ConnectionToken, ILibAsyncServerSocket_BufferReAllocated Callback)
{
    struct ILibAsyncServerSocket_Data *data = (struct ILibAsyncServerSocket_Data*)ILibAsyncSocket_GetUser(ConnectionToken);
    data->Callback = Callback;
}

void ILibAsyncServerSocket_PostSelect(void* socketModule,int slct, void *readset, void *writeset, void *errorset)
{
    struct ILibAsyncServerSocket_Data *data;
    struct sockaddr_in addr;
    int addrlen;
    struct ILibAsyncServerSocketModule *module = (struct ILibAsyncServerSocketModule*)socketModule;
    int i,flags;
    struct sockaddr_in receivingAddress;
    int receivingAddressLength = sizeof(struct sockaddr_in);
#ifdef _WIN32_WCE
    SOCKET NewSocket;
#elif WIN32
    SOCKET NewSocket;
#elif defined( _POSIX)
    int NewSocket;
#endif

    if (FD_ISSET(module->ListenSocket,(fd_set*)readset) != 0) {
        // There are pending TCP connection requests
        for (i = 0; i < module->MaxConnection; ++i) {
            // Check to see if we have available resources to handle this connection request
            if(ILibAsyncSocket_IsFree(module->AsyncSockets[i])!=0)
            {
                addrlen = sizeof(addr);
                NewSocket = accept(module->ListenSocket, (struct sockaddr*)&addr ,&addrlen);

                if (NewSocket != ~0) {
                    switch(module->scope) {
                    case ILibServerScope_LocalLoopback:
                        // Check that the caller ip address is the same as the receive IP address
                        getsockname(NewSocket, (struct sockaddr*)&receivingAddress, &receivingAddressLength);
                        if(receivingAddress.sin_addr.s_addr != addr.sin_addr.s_addr)
                        {
#if defined(WIN32) || defined(_WIN32_WCE)
                            closesocket(NewSocket);
#else
                            close(NewSocket);
#endif
                            NewSocket = ~0;
                        }
                        break;
                    case ILibServerScope_LocalSegment:
                        getsockname(NewSocket, (struct sockaddr*)&receivingAddress, &receivingAddressLength);
                        break;
                    default:
                        break;
                    }
                }
                if (NewSocket != ~0) {
                    // Set this new socket to non-blocking mode, so we can play nice and share thread
#ifdef _WIN32_WCE
                    flags = 1;
                    ioctlsocket(NewSocket,FIONBIO,&flags);
#elif WIN32
                    flags = 1;
                    ioctlsocket(NewSocket,FIONBIO,&flags);
#elif _POSIX
                    flags = fcntl(NewSocket,F_GETFL,0);
                    fcntl(NewSocket,F_SETFL,O_NONBLOCK|flags);
#endif
                    // Instantiate a module to contain all the data about this connection
                    data = (struct ILibAsyncServerSocket_Data*)malloc(sizeof(struct ILibAsyncServerSocket_Data));
                    memset(data,0,sizeof(struct ILibAsyncServerSocket_Data));
                    data->module = socketModule;

                    ILibAsyncSocket_UseThisSocket(module->AsyncSockets[i], &NewSocket, &ILibAsyncServerSocket_OnInterruptSink, data);
                    ILibAsyncSocket_SetRemoteAddress(module->AsyncSockets[i],addr.sin_addr.s_addr);

                    // Notify the user about this new connection
                    if (module->OnConnect != NULL) {
                        module->OnConnect(module,module->AsyncSockets[i], &(data->user));
                    }
                } else {
                    break;
                }
            }
        }
    }
}

void ILibAsyncServerSocket_Destroy(void *socketModule)
{
    struct ILibAsyncServerSocketModule *module =(struct ILibAsyncServerSocketModule*)socketModule;

    free(module->AsyncSockets);
#ifdef _WIN32_WCE
    closesocket(module->ListenSocket);
#elif WIN32
    closesocket(module->ListenSocket);
#elif _POSIX
    close(module->ListenSocket);
#endif
}

void ILibAsyncServerSocket_OnData(ILibAsyncSocket_SocketModule socketModule,char* buffer,int *p_beginPointer, int endPointer,void (**OnInterrupt)(void *AsyncSocketMoudle, void *user),void **user, int *PAUSE)
{
    struct ILibAsyncServerSocket_Data *data = (struct ILibAsyncServerSocket_Data*)(*user);
    int bpointer = *p_beginPointer;

    // Pass the received data up
    if (data != NULL && data->module->OnReceive != NULL) {
        data->module->OnReceive(data->module,socketModule,buffer,&bpointer,endPointer,&(data->module->OnInterrupt),&(data->user),PAUSE);
        if (ILibAsyncSocket_IsFree(socketModule)) {
            *p_beginPointer = endPointer;
        } else {
            *p_beginPointer = bpointer;
        }
    }
}

void ILibAsyncServerSocket_OnDisconnectSink(ILibAsyncSocket_SocketModule socketModule, void *user)
{
    struct ILibAsyncServerSocket_Data *data = (struct ILibAsyncServerSocket_Data*)user;

    // Pass this Disconnect event up
    if (data->module->OnDisconnect != NULL) {
        data->module->OnDisconnect(data->module,socketModule,data->user);
    }

    // If the chain is shutting down, we need to free some resources
    if (ILibIsChainBeingDestroyed(data->module->Chain)==0) {
        free(data);
    }
}

void ILibAsyncServerSocket_OnSendOKSink(ILibAsyncSocket_SocketModule socketModule, void *user)
{
    struct ILibAsyncServerSocket_Data *data = (struct ILibAsyncServerSocket_Data*)user;

    // Pass the OnSendOK event up
    if (data != NULL && data->module->OnSendOK != NULL) {
        data->module->OnSendOK(data->module, socketModule, data->user);
    }
}

void ILibAsyncServerSocket_OnBufferReAllocated(ILibAsyncSocket_SocketModule ConnectionToken, void *user, ptrdiff_t offSet)
{
    struct ILibAsyncServerSocket_Data *data = (struct ILibAsyncServerSocket_Data*)user;
    if (data != NULL && data->Callback != NULL) {
        // If someone above us, has registered for this callback, we need to fire it,
        // with the correct user object
        data->Callback(data->module,ConnectionToken,data->user,offSet);
    }
}

ILibAsyncServerSocket_ServerModule ILibCreateAsyncServerSocketModule(void *Chain, int MaxConnections, int PortNumber, int initialBufferSize, ILibAsyncServerSocket_OnConnect OnConnect,ILibAsyncServerSocket_OnDisconnect OnDisconnect,ILibAsyncServerSocket_OnReceive OnReceive,ILibAsyncServerSocket_OnInterrupt OnInterrupt, ILibAsyncServerSocket_OnSendOK OnSendOK)
{
    struct ILibAsyncServerSocketModule * RetVal;
    int i;

    // Instantiate a new AsyncServer module
    RetVal = (struct ILibAsyncServerSocketModule*)malloc(sizeof(struct ILibAsyncServerSocketModule));
    memset(RetVal,0,sizeof(struct ILibAsyncServerSocketModule));
    RetVal->PreSelect       = &ILibAsyncServerSocket_PreSelect;
    RetVal->PostSelect      = &ILibAsyncServerSocket_PostSelect;
    RetVal->Destroy         = &ILibAsyncServerSocket_Destroy;
    RetVal->Chain           = Chain;
    RetVal->OnConnect       = OnConnect;
    RetVal->OnDisconnect    = OnDisconnect;
    RetVal->OnInterrupt     = OnInterrupt;
    RetVal->OnSendOK        = OnSendOK;
    RetVal->OnReceive       = OnReceive;
    RetVal->MaxConnection   = MaxConnections;
    RetVal->AsyncSockets    = (void**)malloc(MaxConnections * sizeof(void*));
    RetVal->portNumber      = PortNumber;

    // Create our socket pool
    for (i = 0; i < MaxConnections; ++i) {
        RetVal->AsyncSockets[i] = ILibCreateAsyncSocketModule(Chain, initialBufferSize, &ILibAsyncServerSocket_OnData, NULL, &ILibAsyncServerSocket_OnDisconnectSink, &ILibAsyncServerSocket_OnSendOKSink);

        // We want to know about any buffer reallocations, because anything above us may want to know
        ILibAsyncSocket_SetReAllocateNotificationCallback(RetVal->AsyncSockets[i], &ILibAsyncServerSocket_OnBufferReAllocated);
    }
    ILibAddToChain(Chain,RetVal);

    // Get our listening socket
#if defined(WIN32) || defined(_WIN32_WCE)
    RetVal->portNumber = ILibGetStreamSocket(htonl(INADDR_ANY),RetVal->portNumber,(HANDLE*)&(RetVal->ListenSocket));
#else
    RetVal->portNumber = ILibGetStreamSocket(htonl(INADDR_ANY),RetVal->portNumber,&(RetVal->ListenSocket));
#endif

    return(RetVal);
}

unsigned short ILibAsyncServerSocket_GetPortNumber(ILibAsyncServerSocket_ServerModule ServerSocketModule)
{
    struct ILibAsyncServerSocketModule *ASSM = (struct ILibAsyncServerSocketModule*)ServerSocketModule;
    return (ASSM->portNumber);
}
