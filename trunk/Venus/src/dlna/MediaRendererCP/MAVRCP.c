#include "MAVRCP.h"

#include "ILibParsers.h"
#include "ILibThreadPool.h"
#include "MicroAVRCP.h"
#include "DMRCP_ControlPoint.h"
#if defined(_POSIX)
#include <pthread.h>
#endif

struct _tDmrInfo
{
    void * dmr_token;
    void * render;
} ;

struct _tMAVRCP
{
    void * avrender_list;
    sem_t avrender_lock;

    void * avrender_stackchain;
    void * avrender_threadpool;
    void * avrender_rendercp;
    void * avrender_ip_monitor_timer;

    int avrender_ip_addr_len;
    int * avrender_ip_addr_list;
} mavrcp;

/* callback functions define */
Callback_AVRenderSink           avrender_add            = NULL;
Callback_AVRenderSink           avrender_del            = NULL;
Callback_AVRenderUpdateSink     avrender_update         = NULL;

Callback_GetDevCapSink          getdevcap_callback      = NULL;
Callback_CommonSink             play_callback           = NULL;
Callback_CommonSink             seek_callback           = NULL;
Callback_CommonSink             stop_callback           = NULL;
Callback_CommonSink             pause_callback          = NULL;
Callback_CommonSink             next_callback           = NULL;
Callback_CommonSink             prev_callback           = NULL;
Callback_CommonSink             seturi_callback         = NULL;
#if defined(INCLUDE_FEATURE_VOLUME)
Callback_CommonSink             setvol_callback         = NULL;
Callback_CommonSink             setmute_callback        = NULL;
#endif
Callback_CommonSink             setplaymode_callback    = NULL;
Callback_GetMediaInfoSink       getmediainfo_callback   = NULL;
Callback_GetPositionSink        getposition_callback    = NULL;
Callback_GetTransportInfoSink   gettransportinfo_callback = NULL;

/************************************************************************/
/* Callback Functions                                                   */
/************************************************************************/
void OnGetDevCapSink(struct AVRendererConnection * avrc,int ErrorCode, void *Tag, char* PlayMedia,char* RecMedia,char* RecQualityModes)
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "Success!\n" );
        printf( "PlayMedia:%s\n", PlayMedia );
        printf( "RecMedia:%s\n", RecMedia );
        printf( "RecQualityModes:%s\n", RecQualityModes );
    }
#endif

    if ( getdevcap_callback != NULL )
    {
        getdevcap_callback(ErrorCode, PlayMedia, RecMedia, RecQualityModes);
    }
}

void OnPlaySink( struct AVRendererConnection * avrc, int ErrorCode, void * Tag )
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "OnPlaySink: Success!\n" );
        printf( "ProtocolInfo:%s\n", avrc->ProtocolInfo );	
        printf( "MediaUri:%s\n", avrc->MediaUri );	
        printf( "TrackUri:%s\n", avrc->TrackUri );
    }
#endif

    if ( play_callback != NULL )
    {
        play_callback(ErrorCode);
    }
}

void OnSeekSink(struct AVRendererConnection * avrc,int ErrorCode, void *Tag)
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "OnSeekSink: Success!\n" );
    }
#endif

    if ( seek_callback != NULL )
    {
        seek_callback(ErrorCode);
    }
}

void OnStopSink( struct AVRendererConnection * avrc, int ErrorCode, void * Tag )
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "OnStopSink: Success!\n" );
    }
#endif

    if ( stop_callback != NULL )
    {
        stop_callback(ErrorCode);
    }
}

void OnPauseSink( struct AVRendererConnection * avrc, int ErrorCode, void * Tag )
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "OnPauseSink: Success!\n" );
    }
#endif

    if ( pause_callback != NULL )
    {
        pause_callback(ErrorCode);
    }
}

void OnNextSink( struct AVRendererConnection * avrc, int ErrorCode, void * Tag )
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "OnNextSink: Success!\n" );
    }
#endif

    if ( next_callback != NULL )
    {
        next_callback(ErrorCode);
    }
}

void OnPrevSink( struct AVRendererConnection * avrc, int ErrorCode, void * Tag )
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "OnPrevSink: Success!\n" );
    }
#endif

    if ( prev_callback != NULL )
    {
        prev_callback(ErrorCode);
    }
}

void OnSetUriSink( struct AVRendererConnection * avrc, int ErrorCode, void * Tag )
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "OnSetUriSink: Success!\n" );
    }
#endif

    if ( seturi_callback != NULL )
    {
        seturi_callback(ErrorCode);
    }
}

#if defined(INCLUDE_FEATURE_VOLUME)
void OnSetVolumeSink( struct AVRendererConnection * avrc, int ErrorCode, void * Tag )
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "OnSetVolumeSink: Success!\n" );
    }
#endif

    if ( setvol_callback != NULL )
    {
        setvol_callback(ErrorCode);
    }
}

void OnSetMuteSink( struct AVRendererConnection * avrc, int ErrorCode, void * Tag )
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "OnSetMuteSink: Success!\n" );
    }
#endif

    if ( setmute_callback != NULL )
    {
        setmute_callback(ErrorCode);
    }
}
#endif

void OnSetPlayModeSink( struct AVRendererConnection * avrc, int ErrorCode, void * Tag )
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "OnSetPlayModeSink: Success!\n" );
    }
#endif

    if ( setplaymode_callback != NULL )
    {
        setplaymode_callback(ErrorCode);
    }
}

void OnGetMediaInfoSink( struct AVRendererConnection * avrc,int ErrorCode, int nrTracks, int mediaDuration, char * curUrI, char * nextURI,void * Tag )
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "OnGetMediaInfoSink: Success!\n" );
        printf( "NrTracks = %d, MediaDuration = %d, CurUrl = %s, NextUrl = %s\n", nrTracks, mediaDuration, curUrI, nextURI );
    }
#endif

    if ( getmediainfo_callback != NULL )
    {
        getmediainfo_callback(ErrorCode, nrTracks, mediaDuration, curUrI, nextURI);
    }
}

void OnGetPositionSink(struct AVRendererConnection * avrc, int ErrorCode, int RelativeSeconds, int AbsoluteSeconds, int RelativeCounter, int AbsoluteCounter,void *Tag )
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "OnGetPositionSink: Success!\n" );
        printf( "RelativeSeconds = %d, AbsoluteSeconds = %d, RelativeCounter = %d, AbsoluteCounter = %d\n", RelativeSeconds, AbsoluteSeconds, RelativeCounter, AbsoluteCounter );
    }
#endif

    if ( getposition_callback != NULL )
    {
        getposition_callback(ErrorCode, RelativeSeconds, AbsoluteSeconds, RelativeCounter, AbsoluteCounter);
    }
}

void OnGetTransportInfoSink(struct AVRendererConnection * avrc, int ErrorCode, char* CurrentTransportState, char* CurrentTransportStatus, char* CurrentSpeed,void *Tag )
{
#if defined(_DEBUG) || defined(DEBUG)
    printf( "(%d) %s: ErrorCode:%d\n", __LINE__, __FUNCTION__, ErrorCode );
    if ( !ErrorCode )
    {
        printf( "OnGetTransportInfoSink: Success!\n" );
        printf( "CurrentTransportState = %s, CurrentTransportStatus = %s, CurrentSpeed = %s\n", CurrentTransportState ? CurrentTransportState : "", CurrentTransportStatus ? CurrentTransportStatus : "", CurrentSpeed ? CurrentSpeed : "" );
    }
#endif

    if ( gettransportinfo_callback != NULL )
    {
        gettransportinfo_callback(ErrorCode, CurrentTransportState, CurrentTransportStatus, CurrentSpeed);
    }
}


/************************************************************************/
/* Interface                                                            */
/************************************************************************/
struct _tDmrInfo * _getDmrInfo( char * udn )
{
    struct _tDmrInfo * ret_val = NULL;

    if ( NULL == udn )
        return NULL;

    if ( mavrcp.avrender_list == NULL )
        return NULL;

    ILibHashTree_Lock( mavrcp.avrender_list );
    ret_val = (struct _tDmrInfo *)ILibGetEntry(mavrcp.avrender_list, (void *)udn, strlen(udn));
    ILibHashTree_UnLock( mavrcp.avrender_list );

    return ret_val;
}

void printDmrList()
{
    char Key[128] = { 0 };
    int Len = 128, i = 0;
    struct _tDmrInfo * Val = NULL;
    void * dmr_enum = NULL;

    if ( mavrcp.avrender_list == NULL ) return;
    ILibHashTree_Lock( mavrcp.avrender_list );

    dmr_enum = ILibHashTree_GetEnumerator( mavrcp.avrender_list );
    if ( dmr_enum == NULL ) return;
    while ( !ILibHashTree_MoveNext( dmr_enum ) )
    {
        ILibHashTree_GetValue( dmr_enum, (char **)&Key, &Len, ( ( void ** )( &Val ) ) );
        printf( "\t%d. %s [%s]\n", i++, ((struct AVRenderer*)(Val->render))->FriendlyName, ((struct AVRenderer*)(Val->render))->device->UDN );
    }
    ILibHashTree_DestroyEnumerator( dmr_enum );

    ILibHashTree_UnLock( mavrcp.avrender_list );
}

char * dmrGetDlnaDoc( char * udn)
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return NULL;
    }

    return RCP_GetDLNADOC( Val->render );
}

char * dmrGetDlnaCap( char * udn )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return NULL;
    }
    return RCP_GetDLNACAP( Val->render );
}

void dmrGetDevCap( char * udn )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_GetDeviceCap( ((struct AVRenderer *)(Val->render))->Connection, NULL, OnGetDevCapSink );
}

int dmrSupportPlayMode( char * udn, enum _ePlayModeEnum playmode )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return -1;
    }
    return RCP_SupportPlayMode( Val->render, playmode );
}

int dmrSupportVolume( char * udn )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return -1;
    }
    return RCP_SupportVolume( Val->render );
}

int dmrSupportMute( char * udn )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return -1;
    }
    return RCP_SupportMute( Val->render );
}

void dmrPlay( char * udn )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_Play( ((struct AVRenderer *)(Val->render))->Connection, NULL, OnPlaySink );
}

void dmrSeek( char * udn, int pos )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_Seek( ((struct AVRenderer *)(Val->render))->Connection, pos, NULL, OnSeekSink );
}

void dmrStop( char * udn )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_Stop( ((struct AVRenderer *)(Val->render))->Connection, NULL, OnStopSink );
}

void dmrPause( char * udn )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_Pause( ((struct AVRenderer *)(Val->render))->Connection, NULL, OnPauseSink );
}

void dmrNext( char * udn )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_Next( ((struct AVRenderer *)(Val->render))->Connection, NULL, OnNextSink );
}

void dmrPrev( char * udn )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_Prev( ((struct AVRenderer *)(Val->render))->Connection, NULL, OnPrevSink );
}

void dmrSetUri( char * udn, char * uri )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_SetUri( ((struct AVRenderer *)(Val->render))->Connection, uri, NULL, OnSetUriSink );
}

#if defined(INCLUDE_FEATURE_VOLUME)
void dmrSetVolume( char * udn, int vol )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_SetVolume( ((struct AVRenderer *)(Val->render))->Connection, "Master", ( char )( vol ), NULL, OnSetVolumeSink );
}

void dmrSetMute( char * udn, int ismute )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_SetMute( ((struct AVRenderer *)(Val->render))->Connection, "Master", ismute, NULL, OnSetMuteSink );
}
#endif

void dmrSetPlayMode( char * udn, enum _ePlayModeEnum playmode )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_SetPlayMode( ((struct AVRenderer *)(Val->render))->Connection, playmode, NULL, OnSetPlayModeSink );
}

void dmrGetMediaInfo( char * udn )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_GetMediaInfo( ((struct AVRenderer *)(Val->render))->Connection, NULL, OnGetMediaInfoSink );
}

void dmrGetPosition( char * udn )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_GetPosition( ((struct AVRenderer *)(Val->render))->Connection, NULL, OnGetPositionSink );
}

void dmrGetTransportInfo( char * udn )
{
    struct _tDmrInfo * Val = _getDmrInfo( udn );
    if ( Val == NULL )
    {
        printf( "You choice a invalid device!\n" );
        return;
    }
    RCP_GetTransportInfo( ((struct AVRenderer *)(Val->render))->Connection, NULL, OnGetTransportInfoSink);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
void OnRenderStateChanged( struct AVRenderer * sender,struct AVRendererConnection * Connection )
{
    struct _tRenderStateInfo renderstate;
    int i;
    memset( (void *)&renderstate, 0, sizeof(struct _tRenderStateInfo) );

    sem_wait( &(mavrcp.avrender_lock) );
#if defined(_DEBUG) || defined(DEBUG)

    printf( "%s State changed                                       Begin\n", sender->FriendlyName );
    printf( "ProtocolInfo = %s\n", Connection->ProtocolInfo );
    printf( "MediaUri = %s\n", Connection->MediaUri );
    printf( "TrackUri = %s\n", Connection->TrackUri );
    printf( "TransportStatus = %s\n", Connection->TransportStatus );

    printf( "TotalTracks = %d\n", Connection->TotalTracks );
    printf( "TrackNumber = %d\n", Connection->TrackNumber );

    printf( "Play State = " );
    switch ( Connection->PlayState )
    {
    case MAVRCP_PLAYING:
        printf( "AVRCP_PLAYING\n" );
        break;
    case MAVRCP_STOPPED:
        printf( "AVRCP_STOPPED\n" );
        break;
    case MAVRCP_PAUSED:
        printf( "AVRCP_PAUSED\n" );
        break;
    case MAVRCP_RECORDING:
        printf( "AVRCP_RECORDING\n" );
        break;
    case MAVRCP_TRANSITIONING:
        printf( "AVRCP_TRANSITIONING\n" );
        break;
    case MAVRCP_NO_MEDIA:
        printf( "AVRCP_NO_MEDIA\n" );
        break;
    default:
        printf( "AVRCP_UNKNOWN\n" );
        break;
    }
    printf( "Play Mode = " );
    switch ( Connection->PlayMode )
    {
    case MAVRCP_NORMAL:
        printf( "AVRCP_NORMAL\n" );
        break;
    case MAVRCP_REPEAT_ALL:
        printf( "AVRCP_REPEAT_ALL\n" );
        break;
    case MAVRCP_REPEAT_ONE:
        printf( "AVRCP_REPEAT_ONE\n" );
        break;
    case MAVRCP_RANDOM:
        printf( "AVRCP_RANDOM\n" );
        break;
    case MAVRCP_SHUFFLE:
        printf( "AVRCP_SHUFFLE\n" );
        break;
    case MAVRCP_INTRO:
        printf( "AVRCP_INTRO\n" );
        break;
    default:
        printf( "AVRCP_INVALID\n" );
        break;
    }

    printf( "Duration = %d\n", Connection->Duration );

    printf( "TrackPosition = %s\n", Connection->TrackPosition );
    //struct CdsObject* TrackObject;

    for ( i = 0; i < Connection->SupportedRecordQualityModesLength; ++i )
    {
        printf( "SupportedRecordQualityModes[%d] = %s\n", i, Connection->SupportedRecordQualityModes[i] );
    }

    for ( i = 0; i < Connection->PlayMediaLength; ++i )
    {
        printf( "PlayMedia[%d] = %s\n", i, Connection->PlayMedia[i] );
    }

    for ( i = 0; i < Connection->RecordMediaLength; ++i )
    {
        printf( "RecordMedia[%d] = %s\n", i, Connection->RecordMedia[i] );
    }

    for ( i = 0; i < Connection->ChannelCount; ++i )
    {
        printf( "Channel[%d] = %s\n", i, Connection->Channel[i] );
    }

    if (Connection->Volume)
        printf( "Volume = %d\n", Connection->Volume[0] );
    if (Connection->Mute)
        printf( "Mute = %d\n", *( Connection->Mute ) );

    printf( "%s State changed                                       End\n", sender->FriendlyName );
#endif
    renderstate.CMID                                    =Connection->CMID;
    renderstate.AVTID                                   =Connection->AVTID;
    renderstate.RCID                                    =Connection->RCID;
    renderstate.Error                                   =Connection->Error;
    renderstate.ProtocolInfo                            =Connection->ProtocolInfo;
    renderstate.MediaUri                                =Connection->MediaUri;
    renderstate.TrackUri                                =Connection->TrackUri;
    renderstate.TransportStatus                         =Connection->TransportStatus;
    renderstate.TotalTracks                             =Connection->TotalTracks;
    renderstate.TrackNumber                             =Connection->TrackNumber;
    renderstate.PlayState                               =Connection->PlayState;
    renderstate.PlayMode                                =Connection->PlayMode;
    renderstate.Duration                                =Connection->Duration;
    memcpy(renderstate.TrackPosition, Connection->TrackPosition, 32);
    renderstate.SupportedRecordQualityModes             =Connection->SupportedRecordQualityModes;
    renderstate.SupportedRecordQualityModesLength       =Connection->SupportedRecordQualityModesLength;
    renderstate.RecordMedia                             =Connection->RecordMedia;
    renderstate.RecordMediaLength                       =Connection->RecordMediaLength;
    renderstate.PlayMedia                               =Connection->PlayMedia;
    renderstate.PlayMediaLength                         =Connection->PlayMediaLength;
    renderstate.ChannelCount                            =Connection->ChannelCount;
    renderstate.Channel                                 =Connection->Channel;
    renderstate.Volume                                  =Connection->Volume;
    renderstate.Mute                                    =Connection->Mute;
    sem_post( &(mavrcp.avrender_lock) );
    if (avrender_update != NULL)
    {
        avrender_update(sender->device->UDN, sender->device->FriendlyName, renderstate);
    }
}

void OnAddMediaRenderer(void* mediaRendererToken, struct AVRenderer* mediaRenderer)
{
    struct _tDmrInfo * dmr = NULL;

    dmr = ( struct _tDmrInfo * )malloc( sizeof( struct _tDmrInfo ) );
    if ( dmr == NULL )
    {
        fprintf(stderr, "malloc dmrinfo fail!\n");
        return;
    }

    dmr->dmr_token = mediaRendererToken;
    dmr->render = mediaRenderer;
    mediaRenderer->StateChanged = OnRenderStateChanged;

    ILibAddEntry( mavrcp.avrender_list, mediaRenderer->device->UDN, strlen( mediaRenderer->device->UDN ), ( void * )dmr );
    printf( "Add AVRender %s[%s]\n", mediaRenderer->FriendlyName, mediaRenderer->device->UDN );
    if ( avrender_add != NULL )
    {
        avrender_add(mediaRenderer->device->UDN, mediaRenderer->device->FriendlyName);
    }
}

void OnRemoveMediaRenderer(void* mediaRendererToken, struct AVRenderer* mediaRenderer)
{
    void * dmrinfo = NULL;
    dmrinfo = ILibGetEntry( mavrcp.avrender_list, mediaRenderer->device->UDN, strlen( mediaRenderer->device->UDN ) );
    ILibDeleteEntry( mavrcp.avrender_list, mediaRenderer->device->UDN, strlen( mediaRenderer->device->UDN ) );
    free( dmrinfo );

    printf( "Remove AVRender %s[%s]...\n", mediaRenderer->FriendlyName, mediaRenderer->device->UDN );
    if ( avrender_del != NULL )
    {
        avrender_del(mediaRenderer->device->UDN, mediaRenderer->device->FriendlyName);
    }
}

/************************************************************************/
/*                                                                      */
/************************************************************************/

/* thread pool add */
#if defined(WIN32)
DWORD WINAPI pool_thread(void *args)
{
    ILibThreadPool_AddThread(args);
    return (0);
}
#else
void* pool_thread(void *args)
{
    ILibThreadPool_AddThread(args);
    return (0);
}
#endif

void IPAddressMonitor(void *data)
{
    int length;
    int *list;

    length = ILibGetLocalIPAddressList(&list);
    if (length != mavrcp.avrender_ip_addr_len || memcmp((void *)list, (void *)mavrcp.avrender_ip_addr_list, sizeof(int)*length) != 0)
    {
        RCP_IPAddressChanged(mavrcp.avrender_rendercp);

        freesafe(mavrcp.avrender_ip_addr_list);
        mavrcp.avrender_ip_addr_list = list;
        mavrcp.avrender_ip_addr_len = length;
    }
    else
    {
        free(list);
    }

    ILibLifeTime_Add(mavrcp.avrender_ip_monitor_timer, data, 4, (void *)&IPAddressMonitor, NULL);
}

#if defined(WIN32)

DWORD WINAPI start_chain(void *args)
{
    ILibStartChain(mavrcp.avrender_stackchain);

    return (0);
}

#else

void* start_chain(void *args)
{
    ILibStartChain(mavrcp.avrender_stackchain);

    return (0);
}

#endif

int startAVRCP(int threadpool_size)
{
    int count;
#if defined(WIN32)
    DWORD ptid = 0;
#else
    pthread_t t;
    // �߳��к���SIGPIPE�ź�
    sigset_t signal_mask;
    sigemptyset (&signal_mask);
    sigaddset (&signal_mask, SIGPIPE);
    int rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0) {
             printf("block sigpipe error\n");
    }
#endif

    sem_init(&(mavrcp.avrender_lock), 0, 1);
    mavrcp.avrender_list = ILibInitHashTree();

    mavrcp.avrender_stackchain = ILibCreateChain();
    mavrcp.avrender_threadpool = ILibThreadPool_Create();

    for( count = 0; count < threadpool_size; ++count )
    {
#if defined(WIN32)
        CreateThread(NULL, 0, &pool_thread, mavrcp.avrender_threadpool, 0, &ptid);
#else
        pthread_create(&t,NULL,&pool_thread,mavrcp.avrender_threadpool);
#endif
    }

    mavrcp.avrender_rendercp = CreateRendererCP(mavrcp.avrender_stackchain);

    RendererAdded = &OnAddMediaRenderer;
    RendererRemoved = &OnRemoveMediaRenderer;

    mavrcp.avrender_ip_monitor_timer = ILibCreateLifeTime(mavrcp.avrender_stackchain);
    mavrcp.avrender_ip_addr_len = ILibGetLocalIPAddressList(&(mavrcp.avrender_ip_addr_list));
    ILibLifeTime_Add(mavrcp.avrender_ip_monitor_timer, NULL, 4, (void*)&IPAddressMonitor,NULL);

#if defined(WIN32)
    CreateThread(NULL, 0, &start_chain, NULL, 0, &ptid);
#else
    pthread_create(&t,NULL,&start_chain,NULL);
#endif

    return 0;
}

void stopAVRCP()
{
    void * dmr_enum = NULL;
    char Key[128] = { 0 };
    int Len = 128;
    struct _tDmrInfo * Val = NULL;

    ILibStopChain(mavrcp.avrender_stackchain);
    ILibThreadPool_Destroy(mavrcp.avrender_threadpool);

    freesafe(mavrcp.avrender_ip_addr_list);
    sem_destroy(&(mavrcp.avrender_lock));

    if ( mavrcp.avrender_list == NULL ) return;
    ILibHashTree_Lock( mavrcp.avrender_list );

    dmr_enum = ILibHashTree_GetEnumerator( mavrcp.avrender_list );
    if ( dmr_enum == NULL ) return;
    while ( !ILibHashTree_MoveNext( dmr_enum ) )
    {
        ILibHashTree_GetValue( dmr_enum, (char **)&Key, &Len, ( ( void ** )( &Val ) ) );
        freesafe(Val);
    }
    ILibHashTree_DestroyEnumerator( dmr_enum );

    ILibHashTree_UnLock( mavrcp.avrender_list );
    ILibDestroyHashTree( mavrcp.avrender_list );
    mavrcp.avrender_ip_addr_list = NULL;
}
