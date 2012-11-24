#pragma once

#include "Platinum.h"
#include "PltMediaServer.h"
#include "PltSyncMediaBrowser.h"
#include "PltMediaController.h"
#include "NptMap.h"
#include "NptStack.h"
#include <collection.h>

namespace PLTWinRt {

    public enum class ErrorCode : int
    {
        EC_UNKNOWN = -2,
        EC_FAILURE = -1,
        EC_SUCCESS,
        EC_INVALID_PARAMETERS,
        EC_PERMISSION_DENIED,
        EC_OUT_OF_MEMORY,
        EC_NO_SUCH_NAME,
        EC_NO_SUCH_PROPERTY,
        EC_NO_SUCH_ITEM,
        EC_NO_SUCH_CLASS,
        EC_OVERFLOW,
        EC_INTERNAL,
        EC_INVALID_STATE,
        EC_INVALID_FORMAT,
        EC_INVALID_SYNTAX,
        EC_NOT_IMPLEMENTED,
        EC_NOT_SUPPORTED,
        EC_TIMEOUT,
        EC_WOULD_BLOCK,
        EC_TERMINATED,
        EC_OUT_OF_RANGE,
        EC_OUT_OF_RESOURCES,
        EC_NOT_ENOUGH_SPACE,
        EC_INTERRUPTED,
        EC_CANCELLED
    };

    public ref class DMR_MediaInfo sealed {
    public:
        property uint32    num_tracks;
        property Windows::Foundation::TimeSpan media_duration;
        property Platform::String^    cur_uri;
        property Platform::String^    cur_metadata;
        property Platform::String^    next_uri;
        property Platform::String^    next_metadata;
        property Platform::String^    play_medium;
        property Platform::String^    rec_medium;
        property Platform::String^    write_status;
    };

    public ref class DMR_PositionInfo sealed{
    public:
        property uint32    track;
        property Windows::Foundation::TimeSpan track_duration;
        property Platform::String^    track_metadata;
        property Platform::String^    track_uri;
        property Windows::Foundation::TimeSpan rel_time;
        property Windows::Foundation::TimeSpan abs_time;
        property int32     rel_count;
        property int32     abs_count;
    };

    public ref class DMR_TransportInfo sealed{
    public:
        property Platform::String^ cur_transport_state;
        property Platform::String^ cur_transport_status;
        property Platform::String^ cur_speed;
    };

    public ref class DMR_TransportSettings sealed{
    public:
        property Platform::String^ play_mode;
        property Platform::String^ rec_quality_mode;
    };

    public ref class DMR_ConnectionInfo sealed{
    public:
        property uint32 rcs_id;
        property uint32 avtransport_id;
        property Platform::String^ protocol_info;
        property Platform::String^ peer_connection_mgr;
        property uint32 peer_connection_id;
        property Platform::String^ direction;
        property Platform::String^ status;
    };

    public delegate void OnDeviceAdd(Platform::String^ device_id, Platform::String^ divice_name, bool is_dmr);
    public delegate void OnDeviceDel(Platform::String^ device_id, Platform::String^ divice_name, bool is_dmr);

    // AVTransport
    public delegate void OnDmrGetCurrentTransportActions(
        ErrorCode                ec , 
        Platform::String^  device_id ,
        Windows::Foundation::Collections::IVector<Platform::String^>^             actions );
    public delegate void OnDmrGetDeviceCapabilities(
        ErrorCode                ec , 
        Platform::String^  device_id ,
        Windows::Foundation::Collections::IVector<Platform::String^>^             play_media,
        Windows::Foundation::Collections::IVector<Platform::String^>^             rec_media,
        Windows::Foundation::Collections::IVector<Platform::String^>^             rec_quality_modes);
    public delegate void OnDmrGetMediaInfo(
        ErrorCode                ec ,
        Platform::String^  device_id ,
        DMR_MediaInfo^            info);
    public delegate void OnDmrGetPositionInfo(
        ErrorCode                ec ,
        Platform::String^  device_id ,
        DMR_PositionInfo^         info);
    public delegate void OnDmrGetTransportInfo(
        ErrorCode                ec ,
        Platform::String^  device_id ,
        DMR_TransportInfo^        info);
    public delegate void OnDmrGetTransportSettings(
        ErrorCode                ec ,
        Platform::String^  device_id ,
        DMR_TransportSettings^    settings);
    public delegate void OnDmrNext(
        ErrorCode                ec ,
        Platform::String^  device_id);
    public delegate void OnDmrPause(
        ErrorCode                ec ,
        Platform::String^  device_id);
    public delegate void OnDmrPlay(
        ErrorCode                ec ,
        Platform::String^  device_id);
    public delegate void OnDmrPrevious(
        ErrorCode                ec ,
        Platform::String^  device_id);
    public delegate void OnDmrSeek(
        ErrorCode                ec ,
        Platform::String^  device_id);
    public delegate void OnDmrSetAVTransportURI(
        ErrorCode                ec ,
        Platform::String^  device_id);
    public delegate void OnDmrSetPlayMode(
        ErrorCode                ec ,
        Platform::String^  device_id);
    public delegate void OnDmrStop(
        ErrorCode                ec ,
        Platform::String^  device_id);

    // ConnectionManager
    public delegate void OnDmrGetCurrentConnectionIDs(
        ErrorCode                ec ,
        Platform::String^  device_id ,
        Windows::Foundation::Collections::IVector<Platform::String^>^             ids);
    public delegate void OnDmrGetCurrentConnectionInfo(
        ErrorCode                ec ,
        Platform::String^  device_id ,
        DMR_ConnectionInfo^       info);
    public delegate void OnDmrGetProtocolInfo(
        ErrorCode                 ec ,
        Platform::String^         device_id ,
        Windows::Foundation::Collections::IVector<Platform::String^>^             sources ,
        Windows::Foundation::Collections::IVector<Platform::String^>^             sinks);

    // RenderingControl
    public delegate void OnDmrSetMute(
        ErrorCode                ec ,
        Platform::String^  device_id);
    public delegate void OnDmrGetMute(
        ErrorCode                ec ,
        Platform::String^  device_id ,
        bool                      mute);
    public delegate void OnDmrSetVolume(
        ErrorCode                ec ,
        Platform::String^  device_id);
    public delegate void OnDmrGetVolume(
        ErrorCode                ec ,
        Platform::String^  device_id ,
        uint32                volume);

    ref class MediaController;
    /*----------------------------------------------------------------------
    |   definitions
    +---------------------------------------------------------------------*/
    typedef NPT_Map<NPT_String, NPT_String>        PLT_StringMap;
    typedef NPT_Lock<PLT_StringMap>                PLT_LockStringMap;
    typedef NPT_Map<NPT_String, NPT_String>::Entry PLT_StringMapEntry;

    /*----------------------------------------------------------------------
    |   PLT_MediaItemIDFinder
    +---------------------------------------------------------------------*/
    class PLT_MediaItemIDFinder
    {
    public:
        // methods
        PLT_MediaItemIDFinder(const char* object_id) : m_ObjectID(object_id) {}

        bool operator()(const PLT_MediaObject* const & item) const {
            return item->m_ObjectID.Compare(m_ObjectID, true) ? false : true;
        }

    private:
        // members
        NPT_String m_ObjectID;
    };

    /*----------------------------------------------------------------------
    |   PLT_MicroMediaController
    +---------------------------------------------------------------------*/
    class PLT_MicroMediaController
        : public PLT_SyncMediaBrowser
        , public PLT_MediaController
        , public PLT_MediaControllerDelegate
    {
    public:
        PLT_MicroMediaController(PLT_CtrlPointReference& ctrlPoint, MediaController^ mc);
        virtual ~PLT_MicroMediaController();

        void ProcessCommandLoop();

        // PLT_MediaBrowserDelegate methods
        bool OnMSAdded(PLT_DeviceDataReference& device);
        void OnMSRemoved(PLT_DeviceDataReference&  device);
        void OnMSStateVariablesChanged(
            PLT_Service*                  service, 
            NPT_List<PLT_StateVariable*>* vars);

        // PLT_MediaControllerDelegate methods
        bool OnMRAdded(PLT_DeviceDataReference& device);
        void OnMRRemoved(PLT_DeviceDataReference& device);
        void OnMRStateVariablesChanged(PLT_Service*  service , 
            NPT_List<PLT_StateVariable*>*  vars );

        // AVTransport
        void OnGetCurrentTransportActionsResult(
            NPT_Result                res , 
            PLT_DeviceDataReference&  device ,
            PLT_StringList*           actions , 
            void*                     userdata );

        void OnGetDeviceCapabilitiesResult(
            NPT_Result                res , 
            PLT_DeviceDataReference&  device ,
            PLT_DeviceCapabilities*   capabilities ,
            void*                     userdata );

        void OnGetMediaInfoResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            PLT_MediaInfo*            info ,
            void*                     userdata );

        void OnGetPositionInfoResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            PLT_PositionInfo*         info ,
            void*                     userdata );

        void OnGetTransportInfoResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            PLT_TransportInfo*        info ,
            void*                     userdata );

        void OnGetTransportSettingsResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            PLT_TransportSettings*    settings ,
            void*                     userdata );

        void OnNextResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            void*                     userdata );

        void OnPauseResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            void*                     userdata );

        void OnPlayResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            void*                     userdata );

        void OnPreviousResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            void*                     userdata );

        void OnSeekResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            void*                     userdata );

        void OnSetAVTransportURIResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            void*                     userdata );

        void OnSetPlayModeResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            void*                     userdata );

        void OnStopResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            void*                     userdata );

        // ConnectionManager
        void OnGetCurrentConnectionIDsResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            PLT_StringList*           ids ,
            void*                     userdata );

        void OnGetCurrentConnectionInfoResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            PLT_ConnectionInfo*       info ,
            void*                     userdata );

        void OnGetProtocolInfoResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            PLT_StringList*           sources ,
            PLT_StringList*           sinks ,
            void*                     userdata );

        // RenderingControl
        void OnSetMuteResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            void*                     userdata );

        void OnGetMuteResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            const char*               channel ,
            bool                      mute ,
            void*                     userdata );

        void OnSetVolumeResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            void*                     userdata );

        void OnGetVolumeResult(
            NPT_Result                res ,
            PLT_DeviceDataReference&  device ,
            const char*               channel ,
            NPT_UInt32                volume ,
            void*                     userdata );

    private:
        const char* ChooseIDFromTable(PLT_StringMap& table);
        void        PopDirectoryStackToRoot(void);
        NPT_Result  DoBrowse(const char* object_id = NULL, bool metdata = false);

        void        GetCurMediaServer(PLT_DeviceDataReference& server);
        void        GetCurMediaRenderer(PLT_DeviceDataReference& renderer);

        void        FindMediaRendererByUUID(Platform::String^ uuid, PLT_DeviceDataReference& renderer);

        PLT_DeviceDataReference ChooseDevice(const NPT_Lock<PLT_DeviceMap>& deviceList);

        // Command Handlers
        void    HandleCmd_scan(const char* ip);
        void    HandleCmd_getms();
        void    HandleCmd_setms();
        void    HandleCmd_ls();
        void    HandleCmd_info();
        void    HandleCmd_cd(const char* command);
        void    HandleCmd_cdup();
        void    HandleCmd_pwd();
        void    HandleCmd_help();
        void    HandleCmd_getmr();
        void    HandleCmd_setmr();
        void    HandleCmd_open();
        void    HandleCmd_play();
        void    HandleCmd_seek(const char* command);
        void    HandleCmd_stop();
        void    HandleCmd_mute();
        void    HandleCmd_unmute();

    private:
        friend ref class MediaController;

    private:
        /* The tables of known devices on the network.  These are updated via the
        * OnMSAddedRemoved and OnMRAddedRemoved callbacks.  Note that you should first lock
        * before accessing them using the NPT_Map::Lock function.
        */
        NPT_Lock<PLT_DeviceMap> m_MediaServers;
        NPT_Lock<PLT_DeviceMap> m_MediaRenderers;

        /* The currently selected media server as well as 
        * a lock.  If you ever want to hold both the m_CurMediaRendererLock lock and the 
        * m_CurMediaServerLock lock, make sure you grab the server lock first.
        */
        PLT_DeviceDataReference m_CurMediaServer;
        NPT_Mutex               m_CurMediaServerLock;

        /* The currently selected media renderer as well as 
        * a lock.  If you ever want to hold both the m_CurMediaRendererLock lock and the 
        * m_CurMediaServerLock lock, make sure you grab the server lock first.
        */
        PLT_DeviceDataReference m_CurMediaRenderer;
        NPT_Mutex               m_CurMediaRendererLock;

        /* the most recent results from a browse request.  The results come back in a 
        * callback instead of being returned to the calling function, so this 
        * global variable is necessary in order to give the results back to the calling 
        * function.
        */
        PLT_MediaObjectListReference m_MostRecentBrowseResults;

        /* When browsing through the tree on a media server, this is the stack 
        * symbolizing the current position in the tree.  The contents of the 
        * stack are the object ID's of the nodes.  Note that the object id: "0" should
        * always be at the bottom of the stack.
        */
        NPT_Stack<NPT_String> m_CurBrowseDirectoryStack;

        /* the semaphore on which to block when waiting for a response from over
        * the network 
        */
        NPT_SharedVariable m_CallbackResponseSemaphore;

        MediaController^ m_media_controller_;
    };

    public ref class MediaController sealed
    {
    public:
        MediaController();

        ErrorCode Start();
        ErrorCode Stop();

        // AVTransport
        void DmrGetCurrentTransportActions(Platform::String^ device_id);
        void DmrGetDeviceCapabilities(Platform::String^ device_id);
        void DmrGetMediaInfo(Platform::String^ device_id);
        void DmrGetPositionInfo(Platform::String^ device_id);
        void DmrGetTransportInfo(Platform::String^ device_id);
        void DmrGetTransportSettings(Platform::String^ device_id);
        void DmrNext(Platform::String^ device_id);
        void DmrPause(Platform::String^ device_id);
        void DmrPlay(Platform::String^ device_id);
        void DmrPrevious(Platform::String^ device_id);
        void DmrSeek(Platform::String^ device_id, uint64 target);
        bool DmrCanSetNextAVTransportURI(Platform::String^ device_id);
        void DmrSetAVTransportURI(Platform::String^ device_id, Platform::String^ uri, Platform::String^ metadata);
        void DmrSetNextAVTransportURI(Platform::String^ device_id, Platform::String^next_uri, Platform::String^ next_metadata);
        void DmrSetPlayMode(Platform::String^ device_id, Platform::String^ new_play_mode);
        void DmrStop(Platform::String^ device_id);

        // ConnectionManager
        void DmrGetCurrentConnectionIDs(Platform::String^ device_id);
        void DmrGetCurrentConnectionInfo(Platform::String^ device_id);
        void DmrGetProtocolInfo(Platform::String^ device_id);

        // RenderingControl
        void DmrSetMute(Platform::String^ device_id, bool mute);
        void DmrGetMute(Platform::String^ device_id);
        void DmrSetVolume(Platform::String^ device_id, int32 volume);
        void DmrGetVolume(Platform::String^ device_id);

    public: // Events
        event OnDeviceAdd^          onDeviceAdd;
        event OnDeviceDel^          onDeviceDel;

        // AVTransport
        event OnDmrGetCurrentTransportActions^  onDmrGetCurrentTransportActions;
        event OnDmrGetDeviceCapabilities^       onDmrGetDeviceCapabilities;
        event OnDmrGetMediaInfo^                onDmrGetMediaInfo;
        event OnDmrGetPositionInfo^             onDmrGetPositionInfo;
        event OnDmrGetTransportInfo^            onDmrGetTransportInfo;
        event OnDmrGetTransportSettings^        onDmrGetTransportSettings;
        event OnDmrNext^                        onDmrNext;
        event OnDmrPause^                       onDmrPause;
        event OnDmrPlay^                        onDmrPlay;
        event OnDmrPrevious^                    onDmrPrevious;
        event OnDmrSeek^                        onDmrSeek;
        event OnDmrSetAVTransportURI^           onDmrSetAVTransportURI;
        event OnDmrSetPlayMode^                 onDmrSetPlayMode;
        event OnDmrStop^                        onDmrStop;

        // ConnectionManager
        event OnDmrGetCurrentConnectionIDs^     onDmrGetCurrentConnectionIDs;
        event OnDmrGetCurrentConnectionInfo^    onDmrGetCurrentConnectionInfo;
        event OnDmrGetProtocolInfo^             onDmrGetProtocolInfo;

        // RenderingControl
        event OnDmrSetMute^                     onDmrSetMute;
        event OnDmrGetMute^                     onDmrGetMute;
        event OnDmrSetVolume^                   onDmrSetVolume;
        event OnDmrGetVolume^                   onDmrGetVolume;

    private:
        friend class PLT_MicroMediaController;

    private:
        PLT_UPnP                 m_upnp_;
        PLT_CtrlPointReference   m_ctrlPoint_;
        PLT_MicroMediaController m_meida_controller_;
    };
}