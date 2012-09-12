#if defined(__SYMBIAN32__)
//
// Symbian Includes
//
#include <libc\stddef.h>
#include <libc\sys\types.h>
#include <libc\sys\socket.h>
#include <libc\sys\errno.h>
#include <libc\netinet\in.h>
#include <libc\arpa\inet.h>

#define NULL 0
#endif
#ifdef MEMORY_CHECK
#include <assert.h>
#define MEMCHECK(x) x
#else
#define MEMCHECK(x)
#endif

#if defined(__APPLE__)
#define SOL_IP IPPROTO_IP
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
#include "ILibAsyncSocket.h"
#if defined(__SYMBIAN32__)
#include "ILibSocketWrapper.h"
#endif
#if defined(WIN32) && !defined(_WIN32_WCE)
#include <crtdbg.h>
#endif


#if defined(WIN32) && defined(WIN32_QWAVE)
#include "Qos2.h"
#endif

#define DEBUGSTATEMENT(x)


#ifdef SEMAPHORE_TRACKING
#define SEM_TRACK(x) x
void AsyncSocket_TrackLock(const char* MethodName, int Occurance, void *data)
{
    char v[100];

    sprintf(v,"  LOCK[%s, %d] (%x)\r\n",MethodName,Occurance,data);
#ifdef WIN32
    OutputDebugString(v);
#else
    printf(v);
#endif
}
void AsyncSocket_TrackUnLock(const char* MethodName, int Occurance, void *data)
{
    char v[100];

    sprintf(v,"UNLOCK[%s, %d] (%x)\r\n",MethodName,Occurance,data);
#ifdef WIN32
    OutputDebugString(v);
#else
    printf(v);
#endif
}
#else
#define SEM_TRACK(x)
#endif

struct ILibAsyncSocket_SendData
{
    char* buffer;
    int bufferSize;
    int bytesSent;

    int remoteAddress;
    unsigned short remotePort;

    int UserFree;
    struct ILibAsyncSocket_SendData *Next;
};

struct ILibAsyncSocketModule
{
    void (*PreSelect)(void* object,void *readset, void *writeset, void *errorset, int* blocktime);
    void (*PostSelect)(void* object,int slct, void *readset, void *writeset, void *errorset);
    void (*Destroy)(void* object);
    void *Chain;

    unsigned int PendingBytesToSend;
    unsigned int TotalBytesSent;

#if defined(_WIN32_WCE) || defined(WIN32)
    SOCKET internalSocket;
#elif defined(_POSIX)
    int internalSocket;
#endif

    int RemoteIPAddress;
    int RemotePort;
    int LocalIPAddress;
    int LocalIPAddress2;

    struct sockaddr_in addr;

    ILibAsyncSocket_OnData OnData;
    ILibAsyncSocket_OnConnect OnConnect;
    ILibAsyncSocket_OnDisconnect OnDisconnect;
    ILibAsyncSocket_OnSendOK OnSendOK;
    ILibAsyncSocket_OnInterrupt OnInterrupt;

    ILibAsyncSocket_OnBufferSizeExceeded OnBufferSizeExceeded;
    ILibAsyncSocket_OnBufferReAllocated OnBufferReAllocated;

    void *LifeTime;
    void *TimeoutTimer;

    void *user;
    int PAUSE;

    int FinConnect;         // 连接标记，1为已经连接，0为未连接
    int BeginPointer;
    int EndPointer;

    char * buffer;          // 接收缓冲
    int MallocSize;         // 大小
    int InitialSize;        // 初始大小

    struct ILibAsyncSocket_SendData *PendingSend_Head; // 发送缓冲
    struct ILibAsyncSocket_SendData *PendingSend_Tail;
    sem_t SendLock;

    void * ReplaceSocketTimer;
    int MaxBufferSize;
    int MaxBufferSizeExceeded;
    void *MaxBufferSizeUserObject;

#if defined(WIN32) && defined(WIN32_QWAVE)
    HANDLE QoSHandle;
    QOS_FLOWID QosFlowID;
#endif
    int QOSInitialized;
    enum ILibAsyncSocket_QOS_Priority CurrentQOSPriorty;
};
struct ILibAsyncSocket_ReplaceSocketData
{
    void *newSocket;
    void *user;
    ILibAsyncSocket_OnReplaceSocket OnCallback;
    struct ILibAsyncSocketModule *module;
};
void ILibAsyncSocket_PostSelect(void* object,int slct, void *readset, void *writeset, void *errorset);
void ILibAsyncSocket_PreSelect(void* object,void *readset, void *writeset, void *errorset, int* blocktime);

void ILibAsyncSocket_Destroy(void *socketModule)
{
    struct ILibAsyncSocketModule* module = (struct ILibAsyncSocketModule*)socketModule;
    struct ILibAsyncSocket_SendData *temp,*current;

    // 中断通知
    if (!ILibAsyncSocket_IsFree(module)) {
        if (module->OnInterrupt!=NULL) {
            module->OnInterrupt(module, module->user);
        }
    }

    // 关闭套接字
    if (module->internalSocket != ~0) {
        if (module->CurrentQOSPriorty != ILibAsyncSocket_QOS_NONE) {
            ILibAsyncSocket_UnitializeQOS(module);
        }
#if defined(_WIN32_WCE) || defined(WIN32)
#if defined(WINSOCK2)
        shutdown(module->internalSocket, SD_BOTH);
#endif
        closesocket(module->internalSocket);
#elif defined(_POSIX)
        shutdown(module->internalSocket, SHUT_RDWR);
        close(module->internalSocket);
#endif
    }

    // 清空内部缓冲
    if (module->buffer != NULL) {
        free(module->buffer);
        module->buffer = NULL;
        module->MallocSize = 0;
    }

    // 清空所有附加的发送数据
    temp = current=module->PendingSend_Head;
    while (current != NULL) {
        temp = current->Next;
        if (current->UserFree == 0) {
            free(current->buffer);
        }
        free(current);
        current = temp;
    }

    sem_destroy(&(module->SendLock));
}

/*! \fn ILibAsyncSocket_SetReAllocateNotificationCallback(ILibAsyncSocket_SocketModule AsyncSocketToken, ILibAsyncSocket_OnBufferReAllocated Callback)
\brief Set the callback handler for when the internal data buffer has been resized
\param AsyncSocketToken The specific connection to set the callback with
\param Callback The callback handler to set
*/
void ILibAsyncSocket_SetReAllocateNotificationCallback(ILibAsyncSocket_SocketModule AsyncSocketToken, ILibAsyncSocket_OnBufferReAllocated Callback)
{
    if(AsyncSocketToken!=NULL)
    {
        ((struct ILibAsyncSocketModule*)AsyncSocketToken)->OnBufferReAllocated = Callback;
    }
}

/*! \fn ILibCreateAsyncSocketModule(void *Chain, int initialBufferSize, ILibAsyncSocket_OnData OnData, ILibAsyncSocket_OnConnect OnConnect, ILibAsyncSocket_OnDisconnect OnDisconnect,ILibAsyncSocket_OnSendOK OnSendOK)
\brief Creates a new AsyncSocketModule
\param Chain The chain to add this module to. (Chain must <B>not</B> be running)
\param initialBufferSize The initial size of the receive buffer
\param OnData Function Pointer that triggers when Data is received
\param OnConnect Function Pointer that triggers upon successfull connection establishment
\param OnDisconnect Function Pointer that triggers upon disconnect
\param OnSendOK Function Pointer that triggers when pending sends are complete
\returns An ILibAsyncSocket token
*/
ILibAsyncSocket_SocketModule ILibCreateAsyncSocketModule(void *Chain, int initialBufferSize, ILibAsyncSocket_OnData OnData, ILibAsyncSocket_OnConnect OnConnect, ILibAsyncSocket_OnDisconnect OnDisconnect,ILibAsyncSocket_OnSendOK OnSendOK)
{
    struct ILibAsyncSocketModule *RetVal    = (struct ILibAsyncSocketModule*)malloc(sizeof(struct ILibAsyncSocketModule));
    memset(RetVal,0,sizeof(struct ILibAsyncSocketModule));
    RetVal->PreSelect                       = &ILibAsyncSocket_PreSelect;
    RetVal->PostSelect                      = &ILibAsyncSocket_PostSelect;
    RetVal->Destroy                         = &ILibAsyncSocket_Destroy;

    RetVal->internalSocket                  = -1;
    RetVal->OnData                          = OnData;
    RetVal->OnConnect                       = OnConnect;
    RetVal->OnDisconnect                    = OnDisconnect;
    RetVal->OnSendOK                        = OnSendOK;
    RetVal->buffer                          = (char*)malloc(initialBufferSize);
    RetVal->InitialSize                     = initialBufferSize;
    RetVal->MallocSize                      = initialBufferSize;

    RetVal->LifeTime                        = ILibCreateLifeTime(Chain);
    RetVal->TimeoutTimer                    = ILibCreateLifeTime(Chain);
    RetVal->ReplaceSocketTimer              = ILibCreateLifeTime(Chain);

    sem_init(&(RetVal->SendLock),0,1);

    RetVal->Chain = Chain;
    ILibAddToChain(Chain, RetVal);

    return((void*)RetVal);
}

// 清空发送缓冲数据
void ILibAsyncSocket_ClearPendingSend(ILibAsyncSocket_SocketModule socketModule)
{
    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;
    struct ILibAsyncSocket_SendData *data, *temp;

    data = module->PendingSend_Head;
    module->PendingSend_Tail = NULL;
    while (data != NULL) {
        temp = data->Next;
        if (data->UserFree == 0) { // 清空非用户管理的缓冲区
            free(data->buffer);
        }
        free(data); // 删除释放当前节点
        data = temp;
    }
    module->PendingSend_Head = NULL;
    module->PendingBytesToSend = 0;
}

/*! \fn ILibAsyncSocket_SendTo(ILibAsyncSocket_SocketModule socketModule, char* buffer, int length, int remoteAddress, unsigned short remotePort, enum ILibAsyncSocket_MemoryOwnership UserFree)
\brief Sends data on an AsyncSocket module to a specific destination. (Valid only for <B>UDP</B>)
\param socketModule The ILibAsyncSocket module to send data on
\param buffer The buffer to send
\param length The length of the buffer to send
\param remoteAddress The IPAddress of the destination 
\param remotePort The Port number of the destination
\param UserFree Flag indicating memory ownership. 
\returns \a ILibAsyncSocket_SendStatus indicating the send status
*/
enum ILibAsyncSocket_SendStatus ILibAsyncSocket_SendTo(ILibAsyncSocket_SocketModule socketModule, char* buffer, int length, int remoteAddress, unsigned short remotePort, enum ILibAsyncSocket_MemoryOwnership UserFree)
{
    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;
    struct ILibAsyncSocket_SendData *data;
    int unblock = 0;
    int bytesSent;

    struct sockaddr_in dest;
    int destlen = sizeof(dest);

    if (socketModule == NULL) {
        return (ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR);
    }

    data = (struct ILibAsyncSocket_SendData*)malloc(sizeof(struct ILibAsyncSocket_SendData));
    memset(data,0,sizeof(struct ILibAsyncSocket_SendData));

    data->buffer        = buffer;
    data->bufferSize    = length;
    data->bytesSent     = 0;
    data->UserFree      = UserFree;
    data->remoteAddress = remoteAddress;
    data->remotePort    = remotePort;
    data->Next          = NULL;

    SEM_TRACK(AsyncSocket_TrackLock("ILibAsyncSocket_Send",1,module);)
        sem_wait(&(module->SendLock));
    if (module->internalSocket == ~0) {
        // 套接字关闭，发送失败
        if (UserFree == 0) { free(buffer); }
        free(data);
        SEM_TRACK(AsyncSocket_TrackUnLock("ILibAsyncSocket_Send",2,module);)
        sem_post(&(module->SendLock));
        return (ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR);
    }

    module->PendingBytesToSend += length;
    if (module->PendingSend_Tail != NULL) {
        // 插入发送缓冲尾部
        module->PendingSend_Tail->Next  = data;
        module->PendingSend_Tail        = data;
        unblock                         = 1;
        if (UserFree == ILibAsyncSocket_MemoryOwnership_USER) {
            // 外部管理缓冲，需要拷贝
            data->buffer = (char*)malloc(data->bufferSize);
            memcpy(data->buffer,buffer,length);
            MEMCHECK(assert(length <= data->bufferSize);)

            data->UserFree = ILibAsyncSocket_MemoryOwnership_CHAIN;
        }
    } else {
        // 发送缓冲无附加数据，直接发送
        module->PendingSend_Tail = data;
        module->PendingSend_Head = data;

        if (module->PendingSend_Head->remoteAddress == 0 && module->PendingSend_Head->remotePort == 0) {
#if defined(MSG_NOSIGNAL)
            bytesSent = send(module->internalSocket,module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent,module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent,MSG_NOSIGNAL);
#else
    #if defined(WIN32)
            bytesSent = send(module->internalSocket,module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent,module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent,0);
    #else
            signal(SIGPIPE, SIG_IGN);
            bytesSent = send(module->internalSocket,module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent,module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent,0);
    #endif
#endif
        } else {
            dest.sin_addr.s_addr    = module->PendingSend_Head->remoteAddress;
            dest.sin_port           = htons(module->PendingSend_Head->remotePort);
            dest.sin_family         = AF_INET;
#if defined(MSG_NOSIGNAL)
            bytesSent = sendto(module->internalSocket,module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent,module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent,MSG_NOSIGNAL,(struct sockaddr*)&dest,destlen);
#else
    #if defined(WIN32)
            bytesSent = sendto(module->internalSocket,module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent,module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent,0,(struct sockaddr*)&dest,destlen);
    #else
            signal(SIGPIPE, SIG_IGN);
            bytesSent = sendto(module->internalSocket,module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent,module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent,0,(struct sockaddr*)&dest,destlen);
    #endif
#endif
        }
        if (bytesSent > 0) {
            //
            module->PendingSend_Head->bytesSent += bytesSent;
            module->PendingBytesToSend          -= bytesSent;
            module->TotalBytesSent              += bytesSent;
        }
        if (bytesSent == -1) {
#if defined(_WIN32_WCE) || defined(WIN32)
            bytesSent = WSAGetLastError();
            if(bytesSent!=WSAEWOULDBLOCK)
#elif defined(_POSIX)
            if(errno!=EWOULDBLOCK)
#endif
            {
                //
                // Most likely the socket closed while we tried to send
                //
                if (UserFree == 0) { free(buffer); }
                module->PendingSend_Head = module->PendingSend_Tail = NULL;
                free(data);
                SEM_TRACK(AsyncSocket_TrackUnLock("ILibAsyncSocket_Send",3,module);)
                sem_post(&(module->SendLock));

                //
                //Ensure Calling On_Disconnect with MicroStackThread
                //
                ILibLifeTime_Add(module->LifeTime, socketModule, 0, &ILibAsyncSocket_Disconnect, NULL);

                return (ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR);
            }
        }
        if (module->PendingSend_Head->bytesSent == module->PendingSend_Head->bufferSize) {
            // All of the data has been sent
            if (UserFree == 0) { free(module->PendingSend_Head->buffer); }
            module->PendingSend_Tail = NULL;
            free(module->PendingSend_Head);
            module->PendingSend_Head = NULL;
        } else {
            //
            // All of the data wasn't sent, so we need to copy the buffer
            // if we don't own the memory, because the user may free the
            // memory, before we have a chance to complete sending it.
            //
            if (UserFree == ILibAsyncSocket_MemoryOwnership_USER) {
                data->buffer = (char*)malloc(data->bufferSize);
                memcpy(data->buffer,buffer,length);
                MEMCHECK(assert(length <= data->bufferSize);)

                data->UserFree = ILibAsyncSocket_MemoryOwnership_CHAIN;
            }
            unblock = 1;
        }
    }
    SEM_TRACK(AsyncSocket_TrackUnLock("ILibAsyncSocket_Send",4,module);)
    sem_post(&(module->SendLock));
    if (unblock != 0) { ILibForceUnBlockChain(module->Chain); }
    return (unblock);
}

// 断开连接
void ILibAsyncSocket_Disconnect(ILibAsyncSocket_SocketModule socketModule)
{
#if defined(_WIN32_WCE) || defined(WIN32)
    SOCKET s;
#elif defined(_POSIX) || defined(__SYMBIAN32__)
    int s;
#endif

    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;
    if (module == NULL) { return; }


    if (!ILibIsChainBeingDestroyed(module->Chain)) {
        ILibLifeTime_Remove(module->TimeoutTimer, module);
    }

    SEM_TRACK(AsyncSocket_TrackLock("ILibAsyncSocket_Disconnect",1,module);)
    sem_wait(&(module->SendLock));

    if (module->internalSocket != ~0) {
        module->PAUSE = 1;
        s = module->internalSocket; // 直接close可能导致unblock chain
        if (module->CurrentQOSPriorty != ILibAsyncSocket_QOS_NONE) {
            ILibAsyncSocket_UnitializeQOS(module);
        }
        module->internalSocket = ~0;
        if (s != -1) {
#if defined(_WIN32_WCE) || defined(WIN32)
#if defined(WINSOCK2)
            shutdown(s, SD_BOTH);
#endif
            closesocket(s);
#elif defined(_POSIX)
            shutdown(s,SHUT_RDWR);
            close(s);
#endif
        }

        ILibAsyncSocket_ClearPendingSend(socketModule);
        SEM_TRACK(AsyncSocket_TrackUnLock("ILibAsyncSocket_Disconnect",2,module);)
        sem_post(&(module->SendLock));
        if (module->OnDisconnect != NULL) {
            // 触发连接断开回调
            module->OnDisconnect(module, module->user);
        }
    } else {
        SEM_TRACK(AsyncSocket_TrackUnLock("ILibAsyncSocket_Disconnect",3,module);)
        sem_post(&(module->SendLock));
    }
}

// 连接超时回调函数
void ILibAsyncSocket_ConnectTimeout(void *socketModule)
{
    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;

    sem_wait(&(module->SendLock));
    // 连接超时，关闭套接字
#if defined(_WIN32_WCE) || defined(WIN32)
#if defined(WINSOCK2)
    shutdown(module->internalSocket, SD_BOTH);
#endif
    closesocket(module->internalSocket);
#elif defined(_POSIX)
    shutdown(module->internalSocket,SHUT_RDWR);
    close(module->internalSocket);
#endif
    module->internalSocket = ~0;
    sem_post(&(module->SendLock));

    if (module->OnConnect != NULL) { // 触发连接超时回调
        module->OnConnect(module,0,module->user);
    }    
}

// 尝试建立TCP连接
void ILibAsyncSocket_ConnectTo(void* socketModule, int localInterface, int remoteInterface, int remotePortNumber, ILibAsyncSocket_OnInterrupt InterruptPtr,void *user)
{
    int flags;
    struct sockaddr_in addr;
    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;

    if (module == NULL) { return; }

    module->RemoteIPAddress     = remoteInterface;
    module->RemotePort          = remotePortNumber;
    module->PendingBytesToSend  = 0;
    module->TotalBytesSent      = 0;
    module->PAUSE               = 0;
    module->user                = user;
    module->OnInterrupt         = InterruptPtr;
    module->buffer              = (char*)realloc(module->buffer,module->InitialSize);
    module->MallocSize          = module->InitialSize;
    memset((char *)&addr, 0,sizeof(addr));
    addr.sin_family             = AF_INET;
    addr.sin_addr.s_addr        = remoteInterface;
    addr.sin_port               = htons((unsigned short)remotePortNumber);

    // If there isn't a socket already allocated, we need to allocate one
    if (module->internalSocket == -1) {
#if defined(WIN32) || defined(_WIN32_WCE)
        ILibGetStreamSocket(localInterface, 0, (HANDLE*)&(module->internalSocket));
#else
        ILibGetStreamSocket(localInterface,0,&(module->internalSocket));
#endif
    }

    // Initialise the buffer pointers, since no data is in them yet.
    module->FinConnect      = 0;
    module->BeginPointer    = 0;
    module->EndPointer      = 0;

    // Set the socket to non-blocking mode, because we need to play nice
    // and share the MicroStack thread
#if defined(_WIN32_WCE) || defined(WIN32)
    flags = 1;
    ioctlsocket(module->internalSocket, FIONBIO, &flags);
#elif defined(_POSIX)
    flags = fcntl(module->internalSocket, F_GETFL, 0);
    fcntl(module->internalSocket, F_SETFL, O_NONBLOCK | flags);
#endif

    // Turn on keep-alives for the socket
    flags = 1;
    setsockopt(module->internalSocket,SOL_SOCKET,SO_KEEPALIVE,(char*)&flags,sizeof(flags));
    // Connect the socket, and force the chain to unblock, since the select statement
    // doesn't have us in the fdset yet.
    connect(module->internalSocket, (struct sockaddr*)&addr, sizeof(addr));
    // Sometimes a Connection attempt can fail, without triggering the FD_SET. We will force
    // a failure after 30 seconds.
    ILibLifeTime_Add(module->TimeoutTimer,module, CONNECTTIME, &ILibAsyncSocket_ConnectTimeout, NULL);
    ILibForceUnBlockChain(module->Chain);
}

// 处理套接字读
void ILibProcessAsyncSocket(struct ILibAsyncSocketModule *Reader, int pendingRead)
{
    int bytesReceived;
    char *temp;
    int addrlen = sizeof(Reader->addr);

    int iBeginPointer=0;
    int iEndPointer=0;
    int iPointer=0;

#if defined(_POSIX) && defined(IP_PKTINFO)
    int opt;
    struct msghdr msgh;
    struct cmsghdr *cmsg;
    struct iovec iov;
    char cbuf[1024];
    int toaddr;
#endif

    //
    // If the thing isn't paused, and the user set the pointers such that we still have data
    // in our buffers, we need to call the user back with that data, before we attempt to read
    // more data off the network
    //
    if (!pendingRead) {
        if(Reader->internalSocket!=~0 && Reader->PAUSE<=0 && Reader->BeginPointer!=Reader->EndPointer)
        {
            iBeginPointer = Reader->BeginPointer;
            iEndPointer = Reader->EndPointer;
            iPointer = 0;

            while(Reader->internalSocket!=~0 && Reader->PAUSE<=0 && Reader->BeginPointer!=Reader->EndPointer && Reader->EndPointer!=0)
            {                
                Reader->EndPointer = Reader->EndPointer-Reader->BeginPointer;
                Reader->BeginPointer = 0;
                if(Reader->OnData!=NULL)
                {
                    Reader->OnData(Reader,Reader->buffer + iBeginPointer,&(iPointer),Reader->EndPointer,&(Reader->OnInterrupt),&(Reader->user),&(Reader->PAUSE));
                }
                iBeginPointer += iPointer;
                Reader->EndPointer -= iPointer;
                if(iPointer==0)
                {
                    break;
                }
                iPointer = 0;
            }
            Reader->BeginPointer = iBeginPointer;
            Reader->EndPointer = iEndPointer;
        }
    }

    /* Reading Body Only */
    if (Reader->BeginPointer == Reader->EndPointer) {
        Reader->BeginPointer = 0;
        Reader->EndPointer = 0;
    }
    if (!pendingRead || Reader->PAUSE > 0) { // 暂停，直接返回
        return;
    }

    // If we need to grow the buffer, do it now
    if (Reader->MallocSize - Reader->EndPointer < 1024) {
        //
        // This memory reallocation sometimes causes Insure++
        // to incorrectly report a READ_DANGLING (usually in 
        // a call to ILibWebServer_StreamHeader_Raw.)
        // 
        // We verified that the problem is with Insure++ by
        // noting the value of 'temp' (0x008fa8e8), 
        // 'Reader->buffer' (0x00c55e80), and
        // 'MEMORYCHUNKSIZE' (0x00001800).
        //
        // When Insure++ reported the error, it (incorrectly) 
        // claimed that a pointer to memory address 0x00c55ea4
        // was invalid, while (correctly) citing the old memory
        // (0x008fa8e8-0x008fb0e7) as freed memory.
        // Normally Insure++ reports that the invalid pointer 
        // is pointing to someplace in the deallocated block,
        // but that wasn't the case.
        //
        if (Reader->MaxBufferSize==0 || Reader->MallocSize < Reader->MaxBufferSize)
        {
            if(Reader->MaxBufferSize>0 && (Reader->MaxBufferSize - Reader->MallocSize < MEMORYCHUNKSIZE))
            {
                Reader->MallocSize = Reader->MaxBufferSize;
            }
            else
            {
                Reader->MallocSize += MEMORYCHUNKSIZE;
            }
            temp = Reader->buffer;
            Reader->buffer = (char*)realloc(Reader->buffer,Reader->MallocSize);
            //
            // If this realloc moved the buffer somewhere, we need to inform people of it
            //
            if(Reader->buffer!=temp && Reader->OnBufferReAllocated!=NULL)
            {
                Reader->OnBufferReAllocated(Reader,Reader->user,Reader->buffer-temp);
            }
        }
        else
        {
            //
            // If we grow the buffer anymore, it will exceed the maximum allowed buffer size
            //
            Reader->MaxBufferSizeExceeded = 1;
            if(Reader->OnBufferSizeExceeded!=NULL)
            {
                Reader->OnBufferSizeExceeded(Reader,Reader->MaxBufferSizeUserObject);
            }
            ILibAsyncSocket_Disconnect(Reader);
            return;
        }
    }
    else if(Reader->BeginPointer!=0)
    {
        //
        // We can save some cycles by moving the data back to the top
        // of the buffer, instead of just allocating more memory.
        //
        temp = Reader->buffer + Reader->BeginPointer;;
        memmove(Reader->buffer,temp,Reader->EndPointer-Reader->BeginPointer);
        Reader->EndPointer -= Reader->BeginPointer;
        Reader->BeginPointer = 0;

        //
        // Even though we didn't allocate new memory, we still moved data in the buffer, 
        // so we need to inform people of that, because it might be important
        //
        if(Reader->OnBufferReAllocated!=NULL)
        {
            Reader->OnBufferReAllocated(Reader,Reader->user,temp-Reader->buffer);
        }
    }

    sem_wait(&(Reader->SendLock));

#if defined(_POSIX) && defined(IP_PKTINFO)
    if(Reader->LocalIPAddress2!=0)
    {
        opt = 1;
        toaddr = Reader->LocalIPAddress2;
        setsockopt(Reader->internalSocket, SOL_IP, IP_PKTINFO, &opt, sizeof(opt));


        iov.iov_base = Reader->buffer+Reader->EndPointer;
        iov.iov_len = Reader->MallocSize-Reader->EndPointer;
        msgh.msg_control = cbuf;
        msgh.msg_controllen = sizeof(cbuf);
        msgh.msg_name = &(Reader->addr);
        msgh.msg_namelen = addrlen;
        msgh.msg_iov = &iov;
        msgh.msg_iovlen = 1;

        bytesReceived = recvmsg(Reader->internalSocket,&msgh,MSG_PEEK);

        for(cmsg = CMSG_FIRSTHDR(&msgh);
            cmsg != NULL && cmsg->cmsg_len >= sizeof(*cmsg);
            cmsg = CMSG_NXTHDR(&msgh,cmsg))
        {
            if(cmsg->cmsg_level == SOL_IP &&
                cmsg->cmsg_type == IP_PKTINFO)
            {
                toaddr = ((struct in_pktinfo*)CMSG_DATA(cmsg))->ipi_spec_dst.s_addr;
                break;
            }
        }
        if(Reader->LocalIPAddress2!=toaddr)
        {
            bytesReceived = recvmsg(Reader->internalSocket,&msgh,0);
            sem_post(&(Reader->SendLock));
            return;
        }
    }
#endif

    bytesReceived = recvfrom(Reader->internalSocket,Reader->buffer+Reader->EndPointer,Reader->MallocSize-Reader->EndPointer,0,(struct sockaddr *)&(Reader->addr),&addrlen);

    if(Reader->addr.sin_addr.s_addr!=0)
    {
        Reader->RemoteIPAddress = Reader->addr.sin_addr.s_addr;
        Reader->RemotePort = (int)ntohs(Reader->addr.sin_port);
#if defined(WIN32) || defined(_WIN32_WCE)
        if(bytesReceived<=0 && WSAGetLastError()==WSAEMSGSIZE)
        {
            bytesReceived = Reader->MallocSize;
        }
#endif
    }

    if(bytesReceived<=0)
    {
        //
        // This means the socket was gracefully closed by the remote endpoint
        //
        SEM_TRACK(AsyncSocket_TrackLock("ILibProcessAsyncSocket",1,Reader);)
            ILibAsyncSocket_ClearPendingSend(Reader);
        SEM_TRACK(AsyncSocket_TrackUnLock("ILibProcessAsyncSocket",2,Reader);)
            if(Reader->CurrentQOSPriorty!=ILibAsyncSocket_QOS_NONE)
            {
                ILibAsyncSocket_UnitializeQOS(Reader);
            }
#if defined(_WIN32_WCE) || defined(WIN32)
#if defined(WINSOCK2)
            shutdown(Reader->internalSocket,SD_BOTH);
#endif
            closesocket(Reader->internalSocket);
#elif defined(_POSIX)
            shutdown(Reader->internalSocket,SHUT_RDWR);
            close(Reader->internalSocket);
#elif defined(__SYMBIAN32__)
            ILibSocketWrapper_close(Reader->internalSocket);
#endif

            Reader->internalSocket = ~0;
            sem_post(&(Reader->SendLock));

            //
            // Inform the user the socket has closed
            //
            if(Reader->OnDisconnect!=NULL)
            {
                Reader->OnDisconnect(Reader,Reader->user);
            }

            //
            // If we need to free the buffer, do so
            //
            if(Reader->buffer!=NULL)
            {
                free(Reader->buffer);
                Reader->buffer = NULL;
                Reader->MallocSize = 0;
            }
    }
    else
    {
        sem_post(&(Reader->SendLock));

        //
        // Data was read, so increment our counters
        //
        Reader->EndPointer += bytesReceived;

        //
        // Tell the user we have some data
        //
        if(Reader->OnData!=NULL)
        {
            iBeginPointer = Reader->BeginPointer;
            iPointer = 0;
            Reader->OnData(Reader,Reader->buffer + Reader->BeginPointer,&(iPointer),Reader->EndPointer - Reader->BeginPointer,&(Reader->OnInterrupt),&(Reader->user),&(Reader->PAUSE));
            Reader->BeginPointer += iPointer;
        }
        //
        // If the user set the pointers, and we still have data, call them back with the data
        //
        if(Reader->internalSocket!=~0 && Reader->PAUSE<=0 && Reader->BeginPointer!=Reader->EndPointer && Reader->BeginPointer!=0)
        {
            iBeginPointer = Reader->BeginPointer;
            iEndPointer = Reader->EndPointer;
            iPointer = 0;

            while(Reader->internalSocket!=~0 && Reader->PAUSE<=0 && Reader->BeginPointer!=Reader->EndPointer && Reader->EndPointer!=0)
            {                
                Reader->EndPointer = Reader->EndPointer-Reader->BeginPointer;
                Reader->BeginPointer = 0;
                if(Reader->OnData!=NULL)
                {
                    Reader->OnData(Reader,Reader->buffer + iBeginPointer,&(iPointer),Reader->EndPointer,&(Reader->OnInterrupt),&(Reader->user),&(Reader->PAUSE));
                }
                iBeginPointer += iPointer;
                Reader->EndPointer -= iPointer;
                if(iPointer==0)
                {
                    break;
                }
                iPointer = 0;
            }
            Reader->BeginPointer = iBeginPointer;
            Reader->EndPointer = iEndPointer;
        }

        //
        // If the user consumed all of the buffer, we can recycle it
        //
        if(Reader->BeginPointer==Reader->EndPointer)
        {
            Reader->BeginPointer = 0;
            Reader->EndPointer = 0;
        }
    }
}

// 获取用户绑定数据
void * ILibAsyncSocket_GetUser(ILibAsyncSocket_SocketModule socketModule)
{
    return (socketModule == NULL ? NULL: ((struct ILibAsyncSocketModule*)socketModule)->user);
}

void ILibAsyncSocket_PreSelect(void* socketModule, void *readset, void *writeset, void *errorset, int* blocktime)
{
    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;

    SEM_TRACK(AsyncSocket_TrackLock("ILibAsyncSocket_PreSelect", 1, module);)
    sem_wait(&(module->SendLock));

    if (module->internalSocket != -1) {
        if (module->PAUSE < 0) {
            *blocktime = 0;
        }

        if (module->FinConnect == 0) {
            // 尚未连接
            FD_SET(module->internalSocket,(fd_set*)writeset);
            FD_SET(module->internalSocket,(fd_set*)errorset);
        } else {
            if (module->PAUSE == 0) { // Only if this is zero. < 0 is resume, so we want to process first
                /* Already Connected, just needs reading */
                FD_SET(module->internalSocket, (fd_set*)readset);
                FD_SET(module->internalSocket, (fd_set*)errorset);
            }
        }
    }

    if (module->PendingSend_Head != NULL) {
        // 有数据发送，需要确认套接字可写
        FD_SET(module->internalSocket, (fd_set*)writeset);
    }
    SEM_TRACK(AsyncSocket_TrackUnLock("ILibAsyncSocket_PreSelect",2,module);)
    sem_post(&(module->SendLock));
}

void ILibAsyncSocket_PostSelect(void* socketModule,int slct, void *readset, void *writeset, void *errorset)
{
    int TriggerSendOK                       = 0;
    struct ILibAsyncSocket_SendData *temp;
    int bytesSent                           = 0;
    int flags;
    struct sockaddr_in receivingAddress;
    int receivingAddressLength              = sizeof(struct sockaddr_in);
    struct ILibAsyncSocketModule *module    = (struct ILibAsyncSocketModule*)socketModule;
    int TRY_TO_SEND                         = 1;

    int triggerReadSet                      = 0;
    int triggerResume                       = 0;
    int triggerWriteSet                     = 0;
    int triggerErrorSet                     = 0;

    struct sockaddr_in dest;
    int destlen = sizeof(dest);

    SEM_TRACK(AsyncSocket_TrackLock("ILibAsyncSocket_PostSelect",1,module);)
    sem_wait(&(module->SendLock));

    // 处理发送数据
    if (module->FinConnect != 0 && module->internalSocket != ~0 && FD_ISSET(module->internalSocket, (fd_set*)writeset) != 0) {
        // 套接字可写，发送数据
        while (TRY_TO_SEND != 0) {
            if (module->PendingSend_Head->remoteAddress == 0 && module->PendingSend_Head->remotePort == 0) {
#if defined(MSG_NOSIGNAL)
                bytesSent = send(module->internalSocket,module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent,module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent,MSG_NOSIGNAL);
#elif defined(WIN32)
                bytesSent = send(module->internalSocket,module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent,module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent,0);
#else
                signal(SIGPIPE, SIG_IGN);
                bytesSent = send(module->internalSocket,module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent,module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent,0);
#endif
            } else {
                dest.sin_addr.s_addr = module->PendingSend_Head->remoteAddress;
                dest.sin_port = htons(module->PendingSend_Head->remotePort);
                dest.sin_family = AF_INET;
#if defined(MSG_NOSIGNAL)
                bytesSent = sendto(module->internalSocket,module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent,module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent,MSG_NOSIGNAL,(struct sockaddr*)&dest,destlen);
#elif defined(WIN32)
                bytesSent = sendto(module->internalSocket,module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent,module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent,0,(struct sockaddr*)&dest,destlen);
#else
                signal(SIGPIPE, SIG_IGN);
                bytesSent = sendto(module->internalSocket,module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent,module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent,0,(struct sockaddr*)&dest,destlen);
#endif
            }
            if (bytesSent > 0) {
                module->PendingBytesToSend -= bytesSent;
                module->TotalBytesSent += bytesSent;
                module->PendingSend_Head->bytesSent += bytesSent;
                if (module->PendingSend_Head->bytesSent == module->PendingSend_Head->bufferSize) {
                    // 本数据块发送完毕
                    if (module->PendingSend_Head == module->PendingSend_Tail) { // 最后一块
                        module->PendingSend_Tail = NULL;
                    }
                    if (module->PendingSend_Head->UserFree == 0) { // 是否释放内部缓冲
                        free(module->PendingSend_Head->buffer);
                    }
                    temp = module->PendingSend_Head->Next;
                    free(module->PendingSend_Head);
                    module->PendingSend_Head = temp;
                    if (module->PendingSend_Head == NULL) { 
                        TRY_TO_SEND = 0;
                    }
                } else {
                    TRY_TO_SEND = 1;
                }
            }
            if (bytesSent == -1) {
                // 发送数据出错处理
                TRY_TO_SEND = 0;
#if defined(_WIN32_WCE) || defined(WIN32)
                bytesSent = WSAGetLastError();
                if (bytesSent != WSAEWOULDBLOCK){
#elif defined(_POSIX)
                if (errno != EWOULDBLOCK) {
#endif
                    // 发送错误，清空待发送数据，断开连接
                    ILibAsyncSocket_ClearPendingSend(socketModule);
                    ILibLifeTime_Add(module->LifeTime, socketModule, 0, &ILibAsyncSocket_Disconnect, NULL);
                }
            }
        }

        // 全部发送成功，触发OnSendOK
        if (module->PendingSend_Head == NULL && bytesSent != -1) { TriggerSendOK = 1; }
        SEM_TRACK(AsyncSocket_TrackUnLock("ILibAsyncSocket_PostSelect",2,module);)
        sem_post(&(module->SendLock));
        if (TriggerSendOK != 0) {
            module->OnSendOK(module, module->user);
        }
    } else {
        SEM_TRACK(AsyncSocket_TrackUnLock("ILibAsyncSocket_PostSelect",2,module);)
        sem_post(&(module->SendLock));
    }


    SEM_TRACK(AsyncSocket_TrackLock("ILibAsyncSocket_PostSelect",1,module);)
    sem_wait(&(module->SendLock));

    // 处理连接或者读请求
    if (module->internalSocket != ~0) {
        if (module->FinConnect == 0) {
            // 尚未连接，套接字在写集合表示连接成功
            if (FD_ISSET(module->internalSocket, (fd_set*)writeset) != 0) {
                // 连接成功，移除连接超时计时器
                ILibLifeTime_Remove(module->TimeoutTimer, module);
                getsockname(module->internalSocket, (struct sockaddr*)&receivingAddress, &receivingAddressLength);
                module->LocalIPAddress = receivingAddress.sin_addr.s_addr;
                module->FinConnect = 1; // 置位已连接标记
                module->PAUSE = 0;

                // 设定套接字非阻塞
#if defined(_WIN32_WCE) || defined(WIN32)
                flags = 1;
                ioctlsocket(module->internalSocket, FIONBIO, &flags);
#elif defined(_POSIX)
                flags = fcntl(module->internalSocket,F_GETFL,0);
                fcntl(module->internalSocket,F_SETFL,O_NONBLOCK|flags);
#endif
                triggerWriteSet = 1;
            }
            if (FD_ISSET(module->internalSocket, (fd_set*)errorset) != 0) {
                // 连接失败，删除连接超时计时器
                ILibLifeTime_Remove(module->TimeoutTimer,module);

#if defined(_WIN32_WCE) || defined(WIN32)
#if defined(WINSOCK2)    
                shutdown(module->internalSocket,SD_BOTH);
#endif
                closesocket(module->internalSocket);
#elif defined(_POSIX)
                shutdown(module->internalSocket,SHUT_RDWR);
                close(module->internalSocket);
#endif
                module->internalSocket = ~0;
                triggerErrorSet = 1;
            }

            SEM_TRACK(AsyncSocket_TrackUnLock("ILibAsyncSocket_PostSelect",4,module);)
            sem_post(&(module->SendLock));

            if (triggerErrorSet != 0 && module->OnConnect != NULL) { // 连接出错回调
                module->OnConnect(module, 0, module->user);
            } else if (triggerWriteSet != 0 && module->OnConnect != NULL) { // 连接成功回调
                module->OnConnect(module,-1,module->user);
            }
        } else {
            /* Check if PeerReset */
            if (FD_ISSET(module->internalSocket, (fd_set*)errorset) != 0) {
                if (module->CurrentQOSPriorty != ILibAsyncSocket_QOS_NONE) {
                    ILibAsyncSocket_UnitializeQOS(module);
                }
                // 关闭套接字
#if defined(_WIN32_WCE) || defined(WIN32)
#if defined(WINSOCK2)
                shutdown(module->internalSocket, SD_BOTH);
#endif
                closesocket(module->internalSocket);
#elif defined(_POSIX)
                shutdown(module->internalSocket, SHUT_RDWR);
                close(module->internalSocket);
#endif
                module->internalSocket = ~0;
                module->PAUSE = 1;

                ILibAsyncSocket_ClearPendingSend(socketModule);

                triggerErrorSet = 1;
            }

            // 已经连接，度数据
            if (FD_ISSET(module->internalSocket, (fd_set*)readset) != 0) {
                // 有数据可读
                triggerReadSet = 1;
            } else if (module->PAUSE < 0) {
                // Someone resumed a paused connection, but the FD_SET was not triggered
                // because there is no new data on the socket.
                triggerResume = 1;
                ++module->PAUSE;
            }

            SEM_TRACK(AsyncSocket_TrackUnLock("ILibAsyncSocket_PostSelect",4,module);)
            sem_post(&(module->SendLock));

            if (triggerErrorSet != 0 && module->OnDisconnect != NULL) { // 出错断开连接回调
                module->OnDisconnect(module, module->user);
            }
            if (triggerReadSet != 0 || triggerResume != 0) { // 处理读
                ILibProcessAsyncSocket(module, triggerReadSet);
            }
        }
    } else {
        SEM_TRACK(AsyncSocket_TrackUnLock("ILibAsyncSocket_PostSelect",4,module);)
        sem_post(&(module->SendLock));
    }
}

// 确认套接字是否在使用
int ILibAsyncSocket_IsFree(ILibAsyncSocket_SocketModule socketModule)
{
    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;
    return (module->internalSocket == ~0 ? 1: 0);
}

// 获取将要发送数据的大小
unsigned int ILibAsyncSocket_GetPendingBytesToSend(ILibAsyncSocket_SocketModule socketModule)
{
    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;
    return (module->PendingBytesToSend);
}

// 获取已经发送数据的大小
unsigned int ILibAsyncSocket_GetTotalBytesSent(ILibAsyncSocket_SocketModule socketModule)
{
    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;
    return(module->TotalBytesSent);
}

// 重置已发送数据大小
void ILibAsyncSocket_ResetTotalBytesSent(ILibAsyncSocket_SocketModule socketModule)
{
    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;
    module->TotalBytesSent = 0;
}

// 获取接收缓冲
void ILibAsyncSocket_GetBuffer(ILibAsyncSocket_SocketModule socketModule, char **buffer, int *BeginPointer, int *EndPointer)
{
    struct ILibAsyncSocketModule* module = (struct ILibAsyncSocketModule*)socketModule;

    if (module == NULL) { return; }
    *buffer = module->buffer;
    *BeginPointer = module->BeginPointer;
    *EndPointer = module->EndPointer;
}

// 设置远端地址
void ILibAsyncSocket_SetRemoteAddress(ILibAsyncSocket_SocketModule socketModule,int RemoteAddress)
{
    struct ILibAsyncSocketModule* module = (struct ILibAsyncSocketModule*)socketModule;
    if (module != NULL) {
        module->RemoteIPAddress = RemoteAddress;
    }
}

void OnILibAsyncSocket_ReplaceSocketDestroy(void *obj)
{
    struct ILibAsyncSocket_ReplaceSocketData *data = (struct ILibAsyncSocket_ReplaceSocketData*)obj;
    if(data->OnCallback!=NULL)
    {
        data->OnCallback(data->module, data->user);
    }
    free(data);
}
void OnILibAsyncSocket_ReplaceSocketSink(void *obj)
{
    struct ILibAsyncSocket_ReplaceSocketData *data = (struct ILibAsyncSocket_ReplaceSocketData*)obj;
    if(data->module->CurrentQOSPriorty!=ILibAsyncSocket_QOS_NONE)
    {
        ILibAsyncSocket_UnitializeQOS(data->module);
    }
    if(data->module->internalSocket!=~0)
    {
#if defined(_WIN32_WCE) || defined(WIN32)
        closesocket(data->module->internalSocket);
#elif defined(_POSIX)
        close(data->module->internalSocket);
#endif
        data->module->internalSocket = ~0;
    }

    ILibAsyncSocket_UseThisSocket(data->module,data->newSocket,NULL,data->module->user);
    if(data->OnCallback!=NULL)
    {
        data->OnCallback(data->module, data->user);
    }
    free(data);
}
void ILibAsyncSocket_ReplaceSocket(ILibAsyncSocket_SocketModule socketModule, void* newSocket, ILibAsyncSocket_OnReplaceSocket OnCallback, void *user)
{
    struct ILibAsyncSocketModule* module = (struct ILibAsyncSocketModule*)socketModule;
    struct ILibAsyncSocket_ReplaceSocketData *data = (struct ILibAsyncSocket_ReplaceSocketData*)malloc(sizeof(struct ILibAsyncSocket_ReplaceSocketData));
    memset(data,0,sizeof(struct ILibAsyncSocket_ReplaceSocketData));

    data->newSocket = newSocket;
    data->OnCallback = OnCallback;
    data->user = user;
    data->module = module;

    ILibLifeTime_Add(module->ReplaceSocketTimer,data,0,&OnILibAsyncSocket_ReplaceSocketSink,&OnILibAsyncSocket_ReplaceSocketDestroy);
}

/*! \fn ILibAsyncSocket_UseThisSocket(ILibAsyncSocket_SocketModule socketModule,void* UseThisSocket,ILibAsyncSocket_OnInterrupt InterruptPtr,void *user)
\brief Associates an actual socket with ILibAsyncSocket
\par
Instead of calling \a ConnectTo, you can call this method to associate with an already
connected socket.
\param socketModule The ILibAsyncSocket to associate
\param UseThisSocket The socket to associate
\param InterruptPtr Function Pointer that triggers when the TCP connection is interrupted
\param user User object to associate with this session
*/
void ILibAsyncSocket_UseThisSocket(ILibAsyncSocket_SocketModule socketModule,void* UseThisSocket,ILibAsyncSocket_OnInterrupt InterruptPtr,void *user)
{
#if defined(_WIN32_WCE) || defined(WIN32)
    SOCKET TheSocket = *((SOCKET*)UseThisSocket);
#elif defined(_POSIX)
    int TheSocket = *((int*)UseThisSocket);
#endif
    int flags;
    struct ILibAsyncSocketModule* module = (struct ILibAsyncSocketModule*)socketModule;

    if(module==NULL){return;}

    module->PendingBytesToSend = 0;
    module->TotalBytesSent = 0;
    module->internalSocket = TheSocket;
    module->OnInterrupt = InterruptPtr;
    module->user = user;
    module->FinConnect = 1;
    module->PAUSE = 0;

    //
    // If the buffer is too small/big, we need to realloc it to the minimum specified size
    //
    module->buffer = (char*)realloc(module->buffer,module->InitialSize);
    module->MallocSize = module->InitialSize;
    module->FinConnect = 1;
    module->BeginPointer = 0;
    module->EndPointer = 0;

    //
    // Make sure the socket is non-blocking, so we can play nice and share the thread
    //
#if defined(_WIN32_WCE) || defined(WIN32)
    flags = 1;
    ioctlsocket(module->internalSocket,FIONBIO,&flags);
#elif defined(_POSIX)
    flags = fcntl(module->internalSocket,F_GETFL,0);
    fcntl(module->internalSocket,F_SETFL,O_NONBLOCK|flags);
#endif
}

/*! \fn ILibAsyncSocket_GetRemoteInterface(ILibAsyncSocket_SocketModule socketModule)
\brief Returns the Remote Interface of a connected session
\param socketModule The ILibAsyncSocket to query
\returns The remote interface
*/
int ILibAsyncSocket_GetRemoteInterface(ILibAsyncSocket_SocketModule socketModule)
{
    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;
    return(module->RemoteIPAddress);
}
/*! \fn ILibAsyncSocket_GetRemotePort(ILibAsyncSocket_SocketModule socketModule)
\brief Returns the Port number of the origin of the last received packet
\param socketModule The ILibAsyncSocket to query
\returns The remote port
*/
unsigned short ILibAsyncSocket_GetRemotePort(ILibAsyncSocket_SocketModule socketModule)
{
    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;
    return(module->RemotePort);
}

/*! \fn ILibAsyncSocket_GetLocalInterface(ILibAsyncSocket_SocketModule socketModule)
\brief Returns the Local Interface of a connected session, in network order
\param socketModule The ILibAsyncSocket to query
\returns The local interface
*/
int ILibAsyncSocket_GetLocalInterface(ILibAsyncSocket_SocketModule socketModule)
{
    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;
    struct sockaddr_in receivingAddress;
    int receivingAddressLength = sizeof(struct sockaddr_in);

    if(module->LocalIPAddress2!=0)
    {
        return(module->LocalIPAddress2);
    }
    else
    {
#if !defined(__SYMBIAN32__)
        getsockname(module->internalSocket,(struct sockaddr*)&receivingAddress,&receivingAddressLength);
#else
        ILibSocketWrapper_getsockname(module->internalSocket, (struct sockaddr*)&receivingAddress, &receivingAddressLength);
#endif
        return(receivingAddress.sin_addr.s_addr);
    }
}

// 获取本地端口，主机序
unsigned short ILibAsyncSocket_GetLocalPort(ILibAsyncSocket_SocketModule socketModule)
{
    struct ILibAsyncSocketModule *module = (struct ILibAsyncSocketModule*)socketModule;
    struct sockaddr_in receivingAddress;
    int receivingAddressLength = sizeof(struct sockaddr_in);

    getsockname(module->internalSocket, (struct sockaddr*)&receivingAddress, &receivingAddressLength);

    return(ntohs(receivingAddress.sin_port));
}

/*! \fn ILibAsyncSocket_Resume(ILibAsyncSocket_SocketModule socketModule)
\brief Resumes a paused session
\par
Sessions can be paused, such that further data is not read from the socket until resumed
\param socketModule The ILibAsyncSocket to resume
*/
void ILibAsyncSocket_Resume(ILibAsyncSocket_SocketModule socketModule)
{
    struct ILibAsyncSocketModule *sm = (struct ILibAsyncSocketModule*)socketModule;
    if(sm!=NULL)
    {
        sm->PAUSE = -1;
        ILibForceUnBlockChain(sm->Chain);
    }
}

// 获取raw socket
void* ILibAsyncSocket_GetSocket(ILibAsyncSocket_SocketModule module)
{
    struct ILibAsyncSocketModule *sm = (struct ILibAsyncSocketModule*)module;
    return (&(sm->internalSocket));
}

void ILibAsyncSocket_SetLocalInterface2(ILibAsyncSocket_SocketModule module, int localInterface2)
{
    struct ILibAsyncSocketModule *sm = (struct ILibAsyncSocketModule*)module;
    sm->LocalIPAddress2 = localInterface2;
}

void ILibAsyncSocket_SetMaximumBufferSize(ILibAsyncSocket_SocketModule module, int maxSize, ILibAsyncSocket_OnBufferSizeExceeded OnBufferSizeExceededCallback, void *user)
{
    struct ILibAsyncSocketModule *sm = (struct ILibAsyncSocketModule*)module;
    sm->MaxBufferSize = maxSize;
    sm->OnBufferSizeExceeded = OnBufferSizeExceededCallback;
    sm->MaxBufferSizeUserObject = user;
}

int ILibAsyncSocket_WasClosedBecauseBufferSizeExceeded(ILibAsyncSocket_SocketModule socketModule)
{
    struct ILibAsyncSocketModule *sm = (struct ILibAsyncSocketModule*)socketModule;
    return(sm->MaxBufferSizeExceeded);
}

int ILibAsyncSocket_InitializeQOS(ILibAsyncSocket_SocketModule module)
{

    struct ILibAsyncSocketModule *sm = (struct ILibAsyncSocketModule*)module;
#if defined(WIN32) && defined(WIN32_QWAVE)
    QOS_VERSION version;
    version.MajorVersion = 1;
    version.MinorVersion = 0;
#endif

    if(sm->QOSInitialized==0)
    {
#if defined(WIN32) && defined(WIN32_QWAVE)
        sm->QOSInitialized = !QOSCreateHandle(&version, &(sm->QoSHandle));
#endif
    }
    return(sm->QOSInitialized);
}

void ILibAsyncSocket_SetQOSPriority(ILibAsyncSocket_SocketModule module, enum ILibAsyncSocket_QOS_Priority priority)
{
    struct ILibAsyncSocketModule *sm = (struct ILibAsyncSocketModule*)module;
#if defined(WIN32) && defined(WIN32_QWAVE)
    QOS_TRAFFIC_TYPE qtt;
    BOOL r;
    DWORD lastErr;
#endif
    switch(priority)
    {
    case ILibAsyncSocket_QOS_BACKGROUND:    
#if defined(WIN32) && defined(WIN32_QWAVE)
        qtt = QOSTrafficTypeBackground;
#endif
        break;
    case ILibAsyncSocket_QOS_EXCELLENT_EFFORT:
#if defined(WIN32) && defined(WIN32_QWAVE)
        qtt = QOSTrafficTypeExcellentEffort;
#endif
        break;
    case ILibAsyncSocket_QOS_AUDIO_VIDEO:
#if defined(WIN32) && defined(WIN32_QWAVE)
        qtt = QOSTrafficTypeAudioVideo;
#endif
        break;
    case ILibAsyncSocket_QOS_VOICE:    
#if defined(WIN32) && defined(WIN32_QWAVE)
        qtt = QOSTrafficTypeVoice;
#endif            
        break;
    case ILibAsyncSocket_QOS_CONTROL:
#if defined(WIN32) && defined(WIN32_QWAVE)
        qtt = QOSTrafficTypeControl;
#endif    
        break;
    default:
#if defined(WIN32) && defined(WIN32_QWAVE)
        qtt = QOSTrafficTypeBestEffort;
#endif    
        break;
    }

    if(sm->CurrentQOSPriorty != priority)
    {
#if defined(WIN32) && defined(WIN32_QWAVE)
        if(sm->QosFlowID==0)
        {
            r = QOSAddSocketToFlow(sm->QoSHandle,sm->internalSocket,NULL,qtt,QOS_NON_ADAPTIVE_FLOW,&(sm->QosFlowID));
        }
        else
        {
            r = QOSSetFlow(sm->QoSHandle,sm->QosFlowID,QOSSetTrafficType,sizeof(QOS_TRAFFIC_TYPE),&qtt,0,NULL);
            if(r==0 && GetLastError()==ERROR_NOT_FOUND)
            {
                sm->QosFlowID = 0;
                r = QOSAddSocketToFlow(sm->QoSHandle,sm->internalSocket,NULL,qtt,QOS_NON_ADAPTIVE_FLOW,&(sm->QosFlowID));
            }
        }
        if(r==0)
        {
            lastErr = GetLastError();
            if(lastErr == ERROR_NOT_SUPPORTED)
            {
                printf("QOS not supported!\r\n");
            }
        }
        else
        {
            printf("Successfully set QOS priority\r\n");
        }
#endif
        sm->CurrentQOSPriorty = priority;
    }
}

void ILibAsyncSocket_UnitializeQOS(ILibAsyncSocket_SocketModule module)
{
    struct ILibAsyncSocketModule *sm = (struct ILibAsyncSocketModule*)module;
#if defined(WIN32) && defined(WIN32_QWAVE)    
    BOOL r;
    sm->CurrentQOSPriorty = ILibAsyncSocket_QOS_NONE;
    r = QOSRemoveSocketFromFlow(sm->QoSHandle,0,sm->QosFlowID,0);
    r = QOSCloseHandle(sm->QoSHandle);    
    sm->QoSHandle = 0;
    sm->QosFlowID = 0;
#endif
    sm->QOSInitialized = 0;
}

