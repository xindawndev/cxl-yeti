﻿#pragma once

#include "Platinum.h"
#include "PltMediaServer.h"
#include "PltSyncMediaBrowser.h"
#include "PltMediaController.h"
#include "NptMap.h"
#include "NptStack.h"
#include <collection.h>

namespace PLTWinRt {
    public delegate void OnDeviceAdd(Platform::String^ device_id, Platform::String^ divice_name);
    public delegate void OnDeviceDel(Platform::String^ device_id, Platform::String^ divice_name);
    public delegate void OnGetDevCap(Platform::String^ device_id, int ec, Platform::String^ play_media, Platform::String^ rec_media, Platform::String^ rec_qua_meida);
    public delegate void OnPlay(Platform::String^ device_id, int ec);
    public delegate void OnSeek(Platform::String^ device_id, int ec);
    public delegate void OnStop(Platform::String^ device_id, int ec);
    public delegate void OnPause(Platform::String^ device_id, int ec);
    public delegate void OnNext(Platform::String^ device_id, int ec);
    public delegate void OnPrev(Platform::String^ device_id, int ec);
    public delegate void OnSetUri(Platform::String^ device_id, int ec);
    public delegate void OnSetVol(Platform::String^ device_id, int ec);
    public delegate void OnSetMute(Platform::String^ device_id, int ec);
    public delegate void OnSetPlayMode(Platform::String^ device_id, int ec);
    public delegate void OnGetMediaInfo(Platform::String^ device_id, int ec);
    public delegate void OnGetPosition(Platform::String^ device_id, int ec);
    public delegate void OnGetTransportInfo(Platform::String^ device_id, int ec);

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
        PLT_MicroMediaController(PLT_CtrlPointReference& ctrlPoint);
        virtual ~PLT_MicroMediaController();

        void ProcessCommandLoop();

        // PLT_MediaBrowserDelegate methods
        bool OnMSAdded(PLT_DeviceDataReference& device);

        // PLT_MediaControllerDelegate methods
        bool OnMRAdded(PLT_DeviceDataReference& device);
        void OnMRRemoved(PLT_DeviceDataReference& device);
        void OnMRStateVariablesChanged(PLT_Service* /* service */, 
            NPT_List<PLT_StateVariable*>* /* vars */) {};

    private:
        const char* ChooseIDFromTable(PLT_StringMap& table);
        void        PopDirectoryStackToRoot(void);
        NPT_Result  DoBrowse(const char* object_id = NULL, bool metdata = false);

        void        GetCurMediaServer(PLT_DeviceDataReference& server);
        void        GetCurMediaRenderer(PLT_DeviceDataReference& renderer);

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
    };

    public ref class MediaController sealed
    {
    public:
        MediaController();
        void Run();
        void Stop();
        //void GetDeviceCaps(Platform::String^ device_id);

    public: // Events
        event OnDeviceAdd^          onDeviceAdd;
        event OnDeviceDel^          onDeviceDel;
        event OnGetDevCap^          onGetDevCap;
        event OnPlay^               onPlay;
        event OnSeek^               onSeek;
        event OnStop^               onStop;
        event OnPause^              onPause;
        event OnNext^               onNext;
        event OnPrev^               onPrev;
        event OnSetUri^             onSetUri;
        event OnSetVol^             onSetVol;
        event OnSetMute^            onSetMute;
        event OnSetPlayMode^        onSetPlayMode;
        event OnGetMediaInfo^       onGetMediaInfo;
        event OnGetPosition^        onGetPosition;
        event OnGetTransportInfo^   onGetTransportInfo;

    private:
        PLT_UPnP                 m_upnp_;
        PLT_CtrlPointReference   m_ctrlPoint_;
        PLT_MicroMediaController m_meida_controller_;
    };
}