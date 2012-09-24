#if defined(WIN32)
#ifndef MICROSTACK_NO_STDAFX
#include "stdafx.h"
#endif
#define _CRTDBG_MAP_ALLOC
#include <TCHAR.h>
#endif

#if defined(WINSOCK2)
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(WINSOCK1)
#include <winsock.h>
#include <wininet.h>
#endif

#if defined(_POSIX)
#include <pthread.h>
#endif

#include <stdio.h>

#if defined(WIN32)
#include <crtdbg.h>
#endif

#include "TestAPW.h"

void *MicroStackChain;
void *ILib_Pool;

APW apw;

int WaitForExit = 0;

void *ILib_Monitor;
int ILib_IPAddressLength;
int *ILib_IPAddressList;

void Common__CP_IPAddressListChanged();


#if defined(_POSIX)

void ILib_IPAddressMonitor(void *data)
{
    int length;
    int *list;

    length = ILibGetLocalIPAddressList(&list);
    if(length!=ILib_IPAddressLength || memcmp((void*)list,(void*)ILib_IPAddressList,sizeof(int)*length)!=0)
    {
        Common__CP_IPAddressListChanged();

        free(ILib_IPAddressList);
        ILib_IPAddressList = list;
        ILib_IPAddressLength = length;
    }
    else
    {
        free(list);
    }

    ILibLifeTime_Add(ILib_Monitor,NULL,4,(void*)&ILib_IPAddressMonitor,NULL);
}

void ILib_LinuxQuit(void *data)
{
    if(ILib_Pool!=NULL)
    {
        printf("Stopping Thread Pool...\r\n");
        ILibThreadPool_Destroy(ILib_Pool);
        printf("Thread Pool Destroyed...\r\n");
    }

    if(MicroStackChain!=NULL)
    {
        ILibStopChain(MicroStackChain);
        MicroStackChain = NULL;
    }
}

void BreakSink(int s)
{
    if(WaitForExit==0)
    {
        ILibLifeTime_Add(ILib_Monitor,NULL,0,(void*)&ILib_LinuxQuit,NULL);
        WaitForExit = 1;
    }
}

#else

HANDLE ILib_IPAddressMonitorTerminator;
HANDLE ILib_IPAddressMonitorThread;
DWORD ILib_MonitorSocketReserved;
WSAOVERLAPPED ILib_MonitorSocketStateObject;
SOCKET ILib_MonitorSocket;

void CALLBACK ILib_IPAddressMonitor(
                                    IN DWORD dwError,
                                    IN DWORD cbTransferred,
                                    IN LPWSAOVERLAPPED lpOverlapped,
                                    IN DWORD dwFlags
                                    )
{
    Common__CP_IPAddressListChanged();

    WSAIoctl(ILib_MonitorSocket,SIO_ADDRESS_LIST_CHANGE,NULL,0,NULL,0,&ILib_MonitorSocketReserved,&ILib_MonitorSocketStateObject,&ILib_IPAddressMonitor);
}

DWORD WINAPI ILib_IPAddressMonitorLoop(LPVOID args)
{
    ILib_MonitorSocket = socket(AF_INET,SOCK_DGRAM,0);
    WSAIoctl(ILib_MonitorSocket,SIO_ADDRESS_LIST_CHANGE,NULL,0,NULL,0,&ILib_MonitorSocketReserved,&ILib_MonitorSocketStateObject,&ILib_IPAddressMonitor);
    while(WaitForSingleObjectEx(ILib_IPAddressMonitorTerminator,INFINITE,TRUE)!=WAIT_OBJECT_0);
    return 0;
}

DWORD WINAPI DMR_CommandLoop(LPVOID args)
{
    char cmdLine[1024] = { 0 };
    int ret = 0;

    do
    {
        printf( "DMR_Program~# " );
        fgets( cmdLine, 1024, stdin );
        ret = ( strcmp( cmdLine, "exit\n" ) != 0 ) ? 0 : -1;
        /*ret = paraseCmd( cmdLine );*/
    } while ( ret != -1 );

    closesocket(ILib_MonitorSocket);
    SetEvent(ILib_IPAddressMonitorTerminator);
    WaitForSingleObject(ILib_IPAddressMonitorThread,INFINITE);
    CloseHandle(ILib_IPAddressMonitorTerminator);

    ILibStopChain(MicroStackChain);
    return 0;
}
#endif

#if defined(_POSIX)
void* ILibPoolThread(void *args)
{
    ILibThreadPool_AddThread(ILib_Pool);
    return(0);
}

void * DMR_CommandLoop( void *args )
{
    char cmdline[1024];
    int ret = 0;

    do
    {
        printf( "DMR_Program~# " );
        fgets( cmdline, 1024, stdin );
        ret = ( strcmp( cmdline, "exit\n" ) != 0 ) ? 0 : -1;
        //UpdateUi();
        /*ret = paraseCmd( cmdline );*/
    } while( ret != -1 );

    BreakSink( 0 );
    return(0);
}
#else

DWORD WINAPI ILibPoolThread(void *args)
{
    ILibThreadPool_AddThread(ILib_Pool);
    return(0);
}
#endif

void Common__CP_IPAddressListChanged()
{
    //DMR_Method_NotifyMicrostackOfIPAddressChange( testdmr );
}

int main( int argc, char **argv )
{
    int x;

#if defined(_POSIX)
    struct sigaction setup_action;
    sigset_t block_mask;
    pthread_t t, cmdloopthread;
#else
    DWORD ptid = 0;
    DWORD ptid2 = 0;
#endif

    MicroStackChain = ILibCreateChain();

    ILib_Pool = ILibThreadPool_Create();
    for( x = 0; x < 3; ++x )
    {
#if defined(_POSIX)
        pthread_create(&t,NULL,&ILibPoolThread,NULL);
#else
        CreateThread(NULL,0,&ILibPoolThread,NULL,0,&ptid);
#endif
    }

    apw = APW_Method_Create(MicroStackChain, ILib_Pool, 36667, "LeoChenPlayer", "74:E5:0B:10:74:72", "");

    if (apw == NULL) { // ´´½¨Ê§°Ü
        ILibThreadPool_Destroy(ILib_Pool);
        return -1;
    }

    apw->Event_SetAVTransportURI    = APW_Callback_SetAVTransportURI;
    apw->Event_GetAVProtocolInfo    = APW_Callback_GetAVProtocolInfo;
    apw->Event_SetPlayMode          = APW_Callback_SetPlayMode;
    apw->Event_Stop                 = APW_Callback_Stop;
    apw->Event_Play                 = APW_Callback_Play;
    apw->Event_Pause                = APW_Callback_Pause;
    apw->Event_SeekTrack            = APW_Callback_SeekTrack;
    apw->Event_SeekTrackPosition    = APW_Callback_SeekTrackPosition;
    apw->Event_SeekMediaPosition    = APW_Callback_SeekMediaPosition;
    apw->Event_Next                 = APW_Callback_Next;
    apw->Event_Previous             = APW_Callback_Previous;
    apw->Event_SelectPreset         = APW_Callback_SelectPreset;
    apw->Event_SetVolume            = APW_Callback_SetVolume;
    apw->Event_SetMute              = APW_Callback_SetMute;
    apw->Event_SetContrast          = APW_Callback_SetContrast;
    apw->Event_SetBrightness        = APW_Callback_SetBrightness;

#if defined(_POSIX)
    ILib_Monitor = ILibCreateLifeTime(MicroStackChain);

    ILib_IPAddressLength = ILibGetLocalIPAddressList(&ILib_IPAddressList);
    ILibLifeTime_Add(ILib_Monitor,NULL,4,(void*)&ILib_IPAddressMonitor,NULL);

    pthread_create(&cmdloopthread,NULL,&DMR_CommandLoop,NULL);

    sigemptyset (&block_mask);
    /* Block other terminal-generated signals while handler runs. */
    sigaddset (&block_mask, SIGINT);
    sigaddset (&block_mask, SIGQUIT);
    setup_action.sa_handler = BreakSink;
    setup_action.sa_mask = block_mask;
    setup_action.sa_flags = 0;
    sigaction (SIGINT, &setup_action, NULL);
    WaitForExit = 0;
#else
    CreateThread(NULL,0,&DMR_CommandLoop,NULL,0,&ptid);

    ILib_IPAddressMonitorTerminator = CreateEvent(NULL,TRUE,FALSE,NULL);
    ILib_IPAddressMonitorThread = CreateThread(NULL,0,&ILib_IPAddressMonitorLoop,NULL,0,&ptid2);
#endif
    ILibStartChain(MicroStackChain);
#if defined(_POSIX)
    free(ILib_IPAddressList);
#endif
    return 0;
}
