// Class1.cpp
#include "pch.h"
#include "PLTWinRt.h"
#include "PltUPnP.h"
#include "NptWinRtPch.h"
#include <ppltasks.h>

using namespace PLTWinRt;
using namespace Platform;
using namespace Concurrency;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace Windows::Networking::Sockets;
using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;

#include "PltLeaks.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//NPT_SET_LOCAL_LOGGER("platinum.tests.micromediacontroller")

ErrorCode switch_ec(int ec)
{
    switch (ec) {
    case NPT_FAILURE:
        return ErrorCode::EC_FAILURE;
    case NPT_SUCCESS:
        return ErrorCode::EC_SUCCESS;
    case NPT_ERROR_INVALID_PARAMETERS:
        return ErrorCode::EC_INVALID_PARAMETERS;
    case NPT_ERROR_PERMISSION_DENIED:
        return ErrorCode::EC_PERMISSION_DENIED;
    case NPT_ERROR_OUT_OF_MEMORY:
        return ErrorCode::EC_OUT_OF_MEMORY;
    case NPT_ERROR_NO_SUCH_NAME:
        return ErrorCode::EC_NO_SUCH_NAME;
    case NPT_ERROR_NO_SUCH_PROPERTY:
        return ErrorCode::EC_NO_SUCH_PROPERTY;
    case NPT_ERROR_NO_SUCH_ITEM:
        return ErrorCode::EC_NO_SUCH_ITEM;
    case NPT_ERROR_NO_SUCH_CLASS:
        return ErrorCode::EC_NO_SUCH_CLASS;
    case NPT_ERROR_OVERFLOW:
        return ErrorCode::EC_OVERFLOW;
    case NPT_ERROR_INTERNAL:
        return ErrorCode::EC_INTERNAL;
    case NPT_ERROR_INVALID_STATE:
        return ErrorCode::EC_INVALID_STATE;
    case NPT_ERROR_INVALID_FORMAT:
        return ErrorCode::EC_INVALID_FORMAT;
    case NPT_ERROR_INVALID_SYNTAX:
        return ErrorCode::EC_INVALID_SYNTAX;
    case NPT_ERROR_NOT_IMPLEMENTED:
        return ErrorCode::EC_NOT_IMPLEMENTED;
    case NPT_ERROR_NOT_SUPPORTED:
        return ErrorCode::EC_NOT_SUPPORTED;
    case NPT_ERROR_TIMEOUT:
        return ErrorCode::EC_TIMEOUT;
    case NPT_ERROR_WOULD_BLOCK:
        return ErrorCode::EC_WOULD_BLOCK;
    case NPT_ERROR_TERMINATED:
        return ErrorCode::EC_TERMINATED;
    case NPT_ERROR_OUT_OF_RANGE:
        return ErrorCode::EC_OUT_OF_RANGE;
    case NPT_ERROR_OUT_OF_RESOURCES:
        return ErrorCode::EC_OUT_OF_RESOURCES;
    case NPT_ERROR_NOT_ENOUGH_SPACE:
        return ErrorCode::EC_NOT_ENOUGH_SPACE;
    case NPT_ERROR_INTERRUPTED:
        return ErrorCode::EC_INTERRUPTED;
    case NPT_ERROR_CANCELLED:
        return ErrorCode::EC_CANCELLED;
    }

    return ErrorCode::EC_UNKNOWN;
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::PLT_MicroMediaController
+---------------------------------------------------------------------*/
PLT_MicroMediaController::PLT_MicroMediaController(PLT_CtrlPointReference& ctrlPoint, MediaController^ mc) :
    PLT_SyncMediaBrowser(ctrlPoint),
    PLT_MediaController(ctrlPoint)
{
    // create the stack that will be the directory where the
    // user is currently browsing. 
    // push the root directory onto the directory stack.
    m_CurBrowseDirectoryStack.Push("0");

    PLT_MediaController::SetDelegate(this);

    m_media_controller_ = mc;
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::PLT_MicroMediaController
+---------------------------------------------------------------------*/
PLT_MicroMediaController::~PLT_MicroMediaController()
{
}

/*
*  Remove trailing white space from a string
*/
static void strchomp(char* str)
{
    if (!str) return;
    char* e = str+NPT_StringLength(str)-1;

    while (e >= str && *e) {
        if ((*e != ' ')  &&
            (*e != '\t') &&
            (*e != '\r') &&
            (*e != '\n'))
        {
            *(e+1) = '\0';
            break;
        }
        --e;
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::ChooseIDFromTable
+---------------------------------------------------------------------*/
/* 
* Presents a list to the user, allows the user to choose one item.
*
* Parameters:
*		PLT_StringMap: A map that contains the set of items from
*                        which the user should choose.  The key should be a unique ID,
*						 and the value should be a string describing the item. 
*       returns a NPT_String with the unique ID. 
*/
const char*
    PLT_MicroMediaController::ChooseIDFromTable(PLT_StringMap& table)
{
    printf("Select one of the following:\n");

    NPT_List<PLT_StringMapEntry*> entries = table.GetEntries();
    if (entries.GetItemCount() == 0) {
        printf("None available\n"); 
    } else {
        // display the list of entries
        NPT_List<PLT_StringMapEntry*>::Iterator entry = entries.GetFirstItem();
        int count = 0;
        while (entry) {
            printf("%d)\t%s (%s)\n", ++count, (const char*)(*entry)->GetValue(), (const char*)(*entry)->GetKey());
            ++entry;
        }

        int index, watchdog = 3;
        char buffer[1024];

        // wait for input
        while (watchdog > 0) {
            fgets(buffer, 1024, stdin);
            strchomp(buffer);

            if (1 != sscanf(buffer, "%d", &index)) {
                printf("Please enter a number\n");
            } else if (index < 0 || index > count)	{
                printf("Please choose one of the above, or 0 for none\n");
                watchdog--;
                index = 0;
            } else {	
                watchdog = 0;
            }
        }

        // find the entry back
        if (index != 0) {
            entry = entries.GetFirstItem();
            while (entry && --index) {
                ++entry;
            }
            if (entry) {
                return (*entry)->GetKey();
            }
        }
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::PopDirectoryStackToRoot
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::PopDirectoryStackToRoot(void)
{
    NPT_String val;
    while (NPT_SUCCEEDED(m_CurBrowseDirectoryStack.Peek(val)) && val.Compare("0")) {
        m_CurBrowseDirectoryStack.Pop(val);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::OnMSAdded
+---------------------------------------------------------------------*/
bool 
    PLT_MicroMediaController::OnMSAdded(PLT_DeviceDataReference& device) 
{     
    // Issue special action upon discovering MediaConnect server
    PLT_Service* service;
    if (NPT_SUCCEEDED(device->FindServiceByType("urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:*", service))) {
        PLT_ActionReference action;
        PLT_SyncMediaBrowser::m_CtrlPoint->CreateAction(
            device, 
            "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1", 
            "IsAuthorized", 
            action);
        if (!action.IsNull()) PLT_SyncMediaBrowser::m_CtrlPoint->InvokeAction(action, 0);

        PLT_SyncMediaBrowser::m_CtrlPoint->CreateAction(
            device, 
            "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1", 
            "IsValidated", 
            action);
        if (!action.IsNull()) PLT_SyncMediaBrowser::m_CtrlPoint->InvokeAction(action, 0);
    }

    m_media_controller_->onDeviceAdd(NPT_Str2Str(device->GetUUID()), NPT_Str2Str(device->GetFriendlyName()), false);
    return true; 
}

void
    PLT_MicroMediaController::OnMSRemoved(PLT_DeviceDataReference& device)
{
    m_media_controller_->onDeviceDel(NPT_Str2Str(device->GetUUID()), NPT_Str2Str(device->GetFriendlyName()), false);

    NPT_String uuid = device->GetUUID();
    {
        NPT_AutoLock lock(m_MediaServers);
        m_MediaServers.Erase(uuid);
    }

    {
        NPT_AutoLock lock(m_CurMediaServerLock);

        // if it's the currently selected one, we have to get rid of it
        if (!m_CurMediaServer.IsNull() && m_CurMediaServer == device) {
            m_CurMediaServer = NULL;
        }
    }
}

void PLT_MicroMediaController::OnMSStateVariablesChanged(
    PLT_Service*                  service, 
    NPT_List<PLT_StateVariable*>* vars)
{

}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::OnMRAdded
+---------------------------------------------------------------------*/
bool
    PLT_MicroMediaController::OnMRAdded(PLT_DeviceDataReference& device)
{
    NPT_String uuid = device->GetUUID();

    // test if it's a media renderer
    PLT_Service* service;
    if (NPT_SUCCEEDED(device->FindServiceByType("urn:schemas-upnp-org:service:AVTransport:*", service))) {
        NPT_AutoLock lock(m_MediaRenderers);
        m_MediaRenderers.Put(uuid, device);
        m_media_controller_->onDeviceAdd(NPT_Str2Str(device->GetUUID()), NPT_Str2Str(device->GetFriendlyName()), true);
    }

    return true;
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::OnMRRemoved
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::OnMRRemoved(PLT_DeviceDataReference& device)
{
    m_media_controller_->onDeviceDel(NPT_Str2Str(device->GetUUID()), NPT_Str2Str(device->GetFriendlyName()), true);

    NPT_String uuid = device->GetUUID();

    {
        NPT_AutoLock lock(m_MediaRenderers);
        m_MediaRenderers.Erase(uuid);
    }

    {
        NPT_AutoLock lock(m_CurMediaRendererLock);

        // if it's the currently selected one, we have to get rid of it
        if (!m_CurMediaRenderer.IsNull() && m_CurMediaRenderer == device) {
            m_CurMediaRenderer = NULL;
        }
    }
}

void PLT_MicroMediaController::OnMRStateVariablesChanged(PLT_Service*  service , 
                               NPT_List<PLT_StateVariable*>*  vars )
{

}

// AVTransport
void PLT_MicroMediaController::OnGetCurrentTransportActionsResult(
    NPT_Result                res , 
    PLT_DeviceDataReference&  device ,
    PLT_StringList*           actions , 
    void*                     userdata )
{
    if (actions == NULL) {
        m_media_controller_->onDmrGetCurrentTransportActions(switch_ec(res), NPT_Str2Str(device->GetUUID()), nullptr);
        return;
    }

    Vector<Platform::String^>^ acts = ref new Vector<Platform::String^>();
    for (unsigned int i = 0; i < actions->GetItemCount(); ++i ) {
        acts->Append(NPT_Str2Str(*(actions->GetItem(i))));
    }
    m_media_controller_->onDmrGetCurrentTransportActions(switch_ec(res), NPT_Str2Str(device->GetUUID()), acts);
}

void PLT_MicroMediaController::OnGetDeviceCapabilitiesResult(
    NPT_Result                res , 
    PLT_DeviceDataReference&  device ,
    PLT_DeviceCapabilities*   capabilities ,
    void*                     userdata )
{
    if (capabilities == NULL) {
        m_media_controller_->onDmrGetDeviceCapabilities(switch_ec(res), NPT_Str2Str(device->GetUUID()), nullptr, nullptr, nullptr);
        return;
    }

    Vector<Platform::String^>^ pm;
    if (capabilities->play_media.GetItemCount() == 0) {
        pm = nullptr;
    } else {
        pm =  ref new Vector<Platform::String^>();
        for (unsigned int i = 0; i < capabilities->play_media.GetItemCount(); ++i ) {
            pm->Append(NPT_Str2Str(*(capabilities->play_media.GetItem(i))));
        }
    }

    Vector<Platform::String^>^ rm;
    if (capabilities->rec_media.GetItemCount() == 0) {
        rm = nullptr;
    } else {
        rm =  ref new Vector<Platform::String^>();
        for (unsigned int i = 0; i < capabilities->rec_media.GetItemCount(); ++i ) {
            rm->Append(NPT_Str2Str(*(capabilities->rec_media.GetItem(i))));
        }
    }

    Vector<Platform::String^>^ rqm;
    if (capabilities->rec_quality_modes.GetItemCount() == 0) {
        rqm = nullptr;
    } else {
        rqm =  ref new Vector<Platform::String^>();
        for (unsigned int i = 0; i < capabilities->rec_quality_modes.GetItemCount(); ++i ) {
            rqm->Append(NPT_Str2Str(*(capabilities->rec_quality_modes.GetItem(i))));
        }
    }

    m_media_controller_->onDmrGetDeviceCapabilities(
        switch_ec(res),
        NPT_Str2Str(device->GetUUID()),
        pm,
        rm, 
        rqm);
}

void PLT_MicroMediaController::OnGetMediaInfoResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    PLT_MediaInfo*            info ,
    void*                     userdata )
{
    if (info == NULL) {
        m_media_controller_->onDmrGetMediaInfo(
            switch_ec(res),
            NPT_Str2Str(device->GetUUID()),
            nullptr);
        return;
    }

    DMR_MediaInfo^ mediainfo = ref new DMR_MediaInfo;
    mediainfo->num_tracks = info->num_tracks;
    Windows::Foundation::TimeSpan durations;
    durations.Duration =  info->media_duration.ToNanos();
    mediainfo->media_duration = durations;
    mediainfo->cur_uri = NPT_Str2Str(info->cur_uri);
    mediainfo->cur_metadata = NPT_Str2Str(info->cur_metadata);
    mediainfo->next_uri = NPT_Str2Str(info->next_uri);
    mediainfo->next_metadata = NPT_Str2Str(info->next_metadata);
    mediainfo->play_medium = NPT_Str2Str(info->play_medium);
    mediainfo->rec_medium = NPT_Str2Str(info->rec_medium);
    mediainfo->write_status = NPT_Str2Str(info->write_status);
    m_media_controller_->onDmrGetMediaInfo(
        switch_ec(res),
        NPT_Str2Str(device->GetUUID()),
        mediainfo);
}

void PLT_MicroMediaController::OnGetPositionInfoResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    PLT_PositionInfo*         info ,
    void*                     userdata )
{
    if (info == NULL) {
        m_media_controller_->onDmrGetPositionInfo(
            switch_ec(res),
            NPT_Str2Str(device->GetUUID()),
            nullptr);
        return;
    }

    DMR_PositionInfo^ posinfo = ref new DMR_PositionInfo;
    posinfo->track = info->track;
    Windows::Foundation::TimeSpan track_durations;
    track_durations.Duration =  info->track_duration.ToNanos();
    posinfo->track_duration = track_durations;
    posinfo->track_metadata = NPT_Str2Str(info->track_metadata);
    posinfo->track_uri = NPT_Str2Str(info->track_uri);

    Windows::Foundation::TimeSpan rel_time;
    rel_time.Duration =  info->rel_time.ToNanos();
    posinfo->rel_time = rel_time;

    Windows::Foundation::TimeSpan abs_time;
    abs_time.Duration =  info->abs_time.ToNanos();
    posinfo->abs_time = abs_time;

    posinfo->rel_count = info->rel_count;
    posinfo->abs_count = info->abs_count;
    m_media_controller_->onDmrGetPositionInfo(
        switch_ec(res),
        NPT_Str2Str(device->GetUUID()),
        posinfo);
}

void PLT_MicroMediaController::OnGetTransportInfoResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    PLT_TransportInfo*        info ,
    void*                     userdata )
{
    if (info == NULL) {
        m_media_controller_->onDmrGetTransportInfo(
            switch_ec(res),
            NPT_Str2Str(device->GetUUID()),
            nullptr);
        return;
    }

    DMR_TransportInfo^ transinfo = ref new DMR_TransportInfo;
    transinfo->cur_transport_state = NPT_Str2Str(info->cur_transport_state);
    transinfo->cur_transport_status = NPT_Str2Str(info->cur_transport_status);
    transinfo->cur_speed = NPT_Str2Str(info->cur_speed);

    m_media_controller_->onDmrGetTransportInfo(
        switch_ec(res),
        NPT_Str2Str(device->GetUUID()),
        transinfo);
}

void PLT_MicroMediaController::OnGetTransportSettingsResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    PLT_TransportSettings*    settings ,
    void*                     userdata )
{
    if (settings == NULL) {
        m_media_controller_->onDmrGetTransportSettings(
            switch_ec(res),
            NPT_Str2Str(device->GetUUID()),
            nullptr);
        return;
    }

    DMR_TransportSettings^ setinfo = ref new DMR_TransportSettings;
    setinfo->play_mode = NPT_Str2Str(settings->play_mode);
    setinfo->rec_quality_mode = NPT_Str2Str(settings->rec_quality_mode);

    m_media_controller_->onDmrGetTransportSettings(
        switch_ec(res),
        NPT_Str2Str(device->GetUUID()),
        setinfo);
}

void PLT_MicroMediaController::OnNextResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    void*                     userdata )
{
    m_media_controller_->onDmrNext(switch_ec(res), NPT_Str2Str(device->GetUUID()));
}

void PLT_MicroMediaController::OnPauseResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    void*                     userdata )
{
    m_media_controller_->onDmrPause(switch_ec(res), NPT_Str2Str(device->GetUUID()));
}

void PLT_MicroMediaController::OnPlayResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    void*                     userdata )
{
    m_media_controller_->onDmrPlay(switch_ec(res), NPT_Str2Str(device->GetUUID()));
}

void PLT_MicroMediaController::OnPreviousResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    void*                     userdata )
{
    m_media_controller_->onDmrPrevious(switch_ec(res), NPT_Str2Str(device->GetUUID()));
}

void PLT_MicroMediaController::OnSeekResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    void*                     userdata )
{
    m_media_controller_->onDmrSeek(switch_ec(res), NPT_Str2Str(device->GetUUID()));
}

void PLT_MicroMediaController::OnSetAVTransportURIResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    void*                     userdata )
{
    m_media_controller_->onDmrSetAVTransportURI(switch_ec(res), NPT_Str2Str(device->GetUUID()));
}

void PLT_MicroMediaController::OnSetPlayModeResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    void*                     userdata )
{
    m_media_controller_->onDmrSetPlayMode(switch_ec(res), NPT_Str2Str(device->GetUUID()));
}

void PLT_MicroMediaController::OnStopResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    void*                     userdata )
{
    m_media_controller_->onDmrStop(switch_ec(res), NPT_Str2Str(device->GetUUID()));
}

// ConnectionManager
void PLT_MicroMediaController::OnGetCurrentConnectionIDsResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    PLT_StringList*           ids ,
    void*                     userdata )
{
    if (ids == NULL) {
        m_media_controller_->onDmrGetCurrentConnectionIDs(switch_ec(res), NPT_Str2Str(device->GetUUID()), nullptr);
        return;
    }

    Vector<Platform::String^>^ idss =  ref new Vector<Platform::String^>();
    for (unsigned int i = 0; i < ids->GetItemCount(); ++i ) {
        idss->Append(NPT_Str2Str(*(ids->GetItem(i))));
    }

    m_media_controller_->onDmrGetCurrentConnectionIDs(
        switch_ec(res),
        NPT_Str2Str(device->GetUUID()),
        idss);
}

void PLT_MicroMediaController::OnGetCurrentConnectionInfoResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    PLT_ConnectionInfo*       info ,
    void*                     userdata )
{
    if (info == NULL) {
        m_media_controller_->onDmrGetCurrentConnectionInfo(
            switch_ec(res),
            NPT_Str2Str(device->GetUUID()),
            nullptr);
        return;
    }

    DMR_ConnectionInfo^ coninfo = ref new DMR_ConnectionInfo;
    coninfo->rcs_id = info->rcs_id;
    coninfo->avtransport_id = info->avtransport_id;
    coninfo->protocol_info = NPT_Str2Str(info->protocol_info);
    coninfo->peer_connection_mgr = NPT_Str2Str(info->peer_connection_mgr);
    coninfo->peer_connection_id = info->peer_connection_id;
    coninfo->direction = NPT_Str2Str(info->direction);
    coninfo->status = NPT_Str2Str(info->status);

    m_media_controller_->onDmrGetCurrentConnectionInfo(
        switch_ec(res),
        NPT_Str2Str(device->GetUUID()),
        coninfo);
}

void PLT_MicroMediaController::OnGetProtocolInfoResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    PLT_StringList*           sources ,
    PLT_StringList*           sinks ,
    void*                     userdata )
{
    Vector<Platform::String^>^ src;
    if (sources == 0) {
        src = nullptr;
    } else {
        src =  ref new Vector<Platform::String^>();
        for (unsigned int i = 0; i <sources->GetItemCount(); ++i ) {
            src->Append(NPT_Str2Str(*(sources->GetItem(i))));
        }
    }

    Vector<Platform::String^>^ snks;
    if (sinks == 0) {
        snks = nullptr;
    } else {
        snks =  ref new Vector<Platform::String^>();
        for (unsigned int i = 0; i <sinks->GetItemCount(); ++i ) {
            snks->Append(NPT_Str2Str(*(sinks->GetItem(i))));
        }
    }

    m_media_controller_->onDmrGetProtocolInfo(
        switch_ec(res),
        NPT_Str2Str(device->GetUUID()),
        src, snks);
}

// RenderingControl
void PLT_MicroMediaController::OnSetMuteResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    void*                     userdata )
{
    m_media_controller_->onDmrSetMute(switch_ec(res), NPT_Str2Str(device->GetUUID()));
}

void PLT_MicroMediaController::OnGetMuteResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    const char*               channel ,
    bool                      mute ,
    void*                     userdata )
{
    m_media_controller_->onDmrGetMute(switch_ec(res), NPT_Str2Str(device->GetUUID()), mute);
}

void PLT_MicroMediaController::OnSetVolumeResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    void*                     userdata )
{
    m_media_controller_->onDmrSetVolume(switch_ec(res), NPT_Str2Str(device->GetUUID()));
}

void PLT_MicroMediaController::OnGetVolumeResult(
    NPT_Result                res ,
    PLT_DeviceDataReference&  device ,
    const char*               channel ,
    NPT_UInt32                volume ,
    void*                     userdata )
{
    m_media_controller_->onDmrGetVolume(switch_ec(res), NPT_Str2Str(device->GetUUID()), volume);
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::ChooseIDGetCurMediaServerFromTable
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::GetCurMediaServer(PLT_DeviceDataReference& server)
{
    NPT_AutoLock lock(m_CurMediaServerLock);

    if (m_CurMediaServer.IsNull()) {
        printf("No server selected, select one with setms\n");
    } else {
        server = m_CurMediaServer;
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::GetCurMediaRenderer
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::GetCurMediaRenderer(PLT_DeviceDataReference& renderer)
{
    NPT_AutoLock lock(m_CurMediaRendererLock);

    if (m_CurMediaRenderer.IsNull()) {
        printf("No renderer selected, select one with setmr\n");
    } else {
        renderer = m_CurMediaRenderer;
    }
}

void PLT_MicroMediaController::FindMediaRendererByUUID(Platform::String^ uuid, PLT_DeviceDataReference& renderer)
{
    NPT_AutoLock lock(m_MediaRenderers);
    if (m_MediaRenderers.HasKey(Str2NPT_Str(uuid))) {
        renderer = m_MediaRenderers[Str2NPT_Str(uuid)];
    } else {
        // Not found
        renderer = NULL;
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::DoBrowse
+---------------------------------------------------------------------*/
NPT_Result
    PLT_MicroMediaController::DoBrowse(const char* object_id, /* = NULL */
    bool        metadata  /* = false */)
{
    NPT_Result res = NPT_FAILURE;
    PLT_DeviceDataReference device;
    GetCurMediaServer(device);
    if (!device.IsNull()) {
        NPT_String cur_object_id;
        m_CurBrowseDirectoryStack.Peek(cur_object_id);

        // send off the browse packet and block
        res = BrowseSync(
            device, 
            object_id?object_id:(const char*)cur_object_id, 
            m_MostRecentBrowseResults, 
            metadata);		
    }

    return res;
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_getms
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_getms()
{
    PLT_DeviceDataReference device;
    GetCurMediaServer(device);
    if (!device.IsNull()) {
        printf("Current media server: %s\n", (const char*)device->GetFriendlyName());
    } else {
        // this output is taken care of by the GetCurMediaServer call
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_getmr
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_getmr()
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        printf("Current media renderer: %s\n", (const char*)device->GetFriendlyName());
    } else {
        // this output is taken care of by the GetCurMediaRenderer call
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::ChooseDevice
+---------------------------------------------------------------------*/
PLT_DeviceDataReference
    PLT_MicroMediaController::ChooseDevice(const NPT_Lock<PLT_DeviceMap>& deviceList)
{
    PLT_StringMap            namesTable;
    PLT_DeviceDataReference* result = NULL;
    NPT_String               chosenUUID;
    NPT_AutoLock             lock(m_MediaServers);

    // create a map with the device UDN -> device Name 
    const NPT_List<PLT_DeviceMapEntry*>& entries = deviceList.GetEntries();
    NPT_List<PLT_DeviceMapEntry*>::Iterator entry = entries.GetFirstItem();
    while (entry) {
        PLT_DeviceDataReference device = (*entry)->GetValue();
        NPT_String              name   = device->GetFriendlyName();
        namesTable.Put((*entry)->GetKey(), name);

        ++entry;
    }

    // ask user to choose
    chosenUUID = ChooseIDFromTable(namesTable);
    if (chosenUUID.GetLength()) {
        deviceList.Get(chosenUUID, result);
    }

    return result?*result:PLT_DeviceDataReference(); // return empty reference if not device was selected
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_setms
+---------------------------------------------------------------------*/
void 
    PLT_MicroMediaController::HandleCmd_setms()
{
    NPT_AutoLock lock(m_CurMediaServerLock);

    PopDirectoryStackToRoot();
    m_CurMediaServer = ChooseDevice(GetMediaServersMap());
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_setmr
+---------------------------------------------------------------------*/
void 
    PLT_MicroMediaController::HandleCmd_setmr()
{
    NPT_AutoLock lock(m_CurMediaRendererLock);

    m_CurMediaRenderer = ChooseDevice(m_MediaRenderers);
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_ls
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_ls()
{
    DoBrowse();

    if (!m_MostRecentBrowseResults.IsNull()) {
        printf("There were %d results\n", m_MostRecentBrowseResults->GetItemCount());

        NPT_List<PLT_MediaObject*>::Iterator item = m_MostRecentBrowseResults->GetFirstItem();
        while (item) {
            if ((*item)->IsContainer()) {
                printf("Container: %s (%s)\n", (*item)->m_Title.GetChars(), (*item)->m_ObjectID.GetChars());
            } else {
                printf("Item: %s (%s)\n", (*item)->m_Title.GetChars(), (*item)->m_ObjectID.GetChars());
            }
            ++item;
        }

        m_MostRecentBrowseResults = NULL;
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_info
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_info()
{
    NPT_String              object_id;
    PLT_StringMap           tracks;
    PLT_DeviceDataReference device;

    // issue a browse
    DoBrowse();

    if (!m_MostRecentBrowseResults.IsNull()) {
        // create a map item id -> item title
        NPT_List<PLT_MediaObject*>::Iterator item = m_MostRecentBrowseResults->GetFirstItem();
        while (item) {
            if (!(*item)->IsContainer()) {
                tracks.Put((*item)->m_ObjectID, (*item)->m_Title);
            }
            ++item;
        }

        // let the user choose which one
        object_id = ChooseIDFromTable(tracks);

        if (object_id.GetLength()) {
            // issue a browse with metadata
            DoBrowse(object_id, true);

            // look back for the PLT_MediaItem in the results
            PLT_MediaObject* track = NULL;
            if (!m_MostRecentBrowseResults.IsNull() &&
                NPT_SUCCEEDED(NPT_ContainerFind(*m_MostRecentBrowseResults, PLT_MediaItemIDFinder(object_id), track))) {

                    // display info
                    printf("Title: %s \n", track->m_Title.GetChars());
                    printf("OjbectID: %s\n", track->m_ObjectID.GetChars());
                    printf("Class: %s\n", track->m_ObjectClass.type.GetChars());
                    printf("Creator: %s\n", track->m_Creator.GetChars());
                    printf("Date: %s\n", track->m_Date.GetChars());
                    for (NPT_List<PLT_AlbumArtInfo>::Iterator iter = track->m_ExtraInfo.album_arts.GetFirstItem();
                        iter;
                        iter++) {
                            printf("Art Uri: %s\n", (*iter).uri.GetChars());
                            printf("Art Uri DLNA Profile: %s\n", (*iter).dlna_profile.GetChars());
                    }
                    for (NPT_Cardinal i=0;i<track->m_Resources.GetItemCount(); i++) {
                        printf("\tResource[%d].uri: %s\n", i, track->m_Resources[i].m_Uri.GetChars());
                        printf("\tResource[%d].profile: %s\n", i, track->m_Resources[i].m_ProtocolInfo.ToString().GetChars());
                        printf("\tResource[%d].duration: %d\n", i, track->m_Resources[i].m_Duration);
                        printf("\tResource[%d].size: %d\n", i, (int)track->m_Resources[i].m_Size);
                        printf("\n");
                    }
                    printf("Didl: %s\n", (const char*)track->m_Didl);
            } else {
                printf("Couldn't find the track\n");
            }
        }

        m_MostRecentBrowseResults = NULL;
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_cd
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_cd(const char* command)
{
    NPT_String    newobject_id;
    PLT_StringMap containers;

    // if command has parameter, push it to stack and return
    NPT_String id = command;
    NPT_List<NPT_String> args = id.Split(" ");
    if (args.GetItemCount() >= 2) {
        args.Erase(args.GetFirstItem());
        id = NPT_String::Join(args, " ");
        m_CurBrowseDirectoryStack.Push(id);
        return;
    }

    // list current directory to let user choose
    DoBrowse();

    if (!m_MostRecentBrowseResults.IsNull()) {
        NPT_List<PLT_MediaObject*>::Iterator item = m_MostRecentBrowseResults->GetFirstItem();
        while (item) {
            if ((*item)->IsContainer()) {
                containers.Put((*item)->m_ObjectID, (*item)->m_Title);
            }
            ++item;
        }

        newobject_id = ChooseIDFromTable(containers);
        if (newobject_id.GetLength()) {
            m_CurBrowseDirectoryStack.Push(newobject_id);
        }

        m_MostRecentBrowseResults = NULL;
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_cdup
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_cdup()
{
    // we don't want to pop the root off now....
    NPT_String val;
    m_CurBrowseDirectoryStack.Peek(val);
    if (val.Compare("0")) {
        m_CurBrowseDirectoryStack.Pop(val);
    } else {
        printf("Already at root\n");
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_pwd
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_pwd()
{
    NPT_Stack<NPT_String> tempStack;
    NPT_String val;

    while (NPT_SUCCEEDED(m_CurBrowseDirectoryStack.Peek(val))) {
        m_CurBrowseDirectoryStack.Pop(val);
        tempStack.Push(val);
    }

    while (NPT_SUCCEEDED(tempStack.Peek(val))) {
        tempStack.Pop(val);
        printf("%s/", (const char*)val);
        m_CurBrowseDirectoryStack.Push(val);
    }
    printf("\n");
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_open
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_open()
{
    NPT_String              object_id;
    PLT_StringMap           tracks;
    PLT_DeviceDataReference device;

    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        // get the protocol info to try to see in advance if a track would play on the device

        // issue a browse
        DoBrowse();

        if (!m_MostRecentBrowseResults.IsNull()) {
            // create a map item id -> item title
            NPT_List<PLT_MediaObject*>::Iterator item = m_MostRecentBrowseResults->GetFirstItem();
            while (item) {
                if (!(*item)->IsContainer()) {
                    tracks.Put((*item)->m_ObjectID, (*item)->m_Title);
                }
                ++item;
            }

            // let the user choose which one
            object_id = ChooseIDFromTable(tracks);
            if (object_id.GetLength()) {
                // look back for the PLT_MediaItem in the results
                PLT_MediaObject* track = NULL;
                if (NPT_SUCCEEDED(NPT_ContainerFind(*m_MostRecentBrowseResults, PLT_MediaItemIDFinder(object_id), track))) {
                    if (track->m_Resources.GetItemCount() > 0) {
                        // look for best resource to use by matching each resource to a sink advertised by renderer
                        NPT_Cardinal resource_index = 0;
                        if (NPT_FAILED(FindBestResource(device, *track, resource_index))) {
                            printf("No matching resource\n");
                            return;
                        }

                        // invoke the setUri
                        printf("Issuing SetAVTransportURI with url=%s & didl=%s", 
                            (const char*)track->m_Resources[resource_index].m_Uri, 
                            (const char*)track->m_Didl);
                        SetAVTransportURI(device, 0, track->m_Resources[resource_index].m_Uri, track->m_Didl, NULL);
                    } else {
                        printf("Couldn't find the proper resource\n");
                    }

                } else {
                    printf("Couldn't find the track\n");
                }
            }

            m_MostRecentBrowseResults = NULL;
        }
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_play
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_play()
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        Play(device, 0, "1", NULL);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_seek
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_seek(const char* command)
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        // remove first part of command ("seek")
        NPT_String target = command;
        NPT_List<NPT_String> args = target.Split(" ");
        if (args.GetItemCount() < 2) return;

        args.Erase(args.GetFirstItem());
        target = NPT_String::Join(args, " ");

        Seek(device, 0, (target.Find(":")!=-1)?"REL_TIME":"X_DLNA_REL_BYTE", target, NULL);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_stop
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_stop()
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        Stop(device, 0, NULL);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_mute
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_mute()
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        SetMute(device, 0, "Master", true, NULL);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_unmute
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_unmute()
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        SetMute(device, 0, "Master", false, NULL);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_help
+---------------------------------------------------------------------*/
void
    PLT_MicroMediaController::HandleCmd_help()
{
    printf("\n\nNone of the commands take arguments.  The commands with a * \n");
    printf("signify ones that will prompt the user for more information once\n");
    printf("the command is called\n\n");
    printf("The available commands are:\n\n");
    printf(" quit    -   shutdown the Control Point\n");
    printf(" exit    -   same as quit\n");

    printf(" setms   - * select a media server to become the active media server\n");
    printf(" getms   -   print the friendly name of the active media server\n");
    printf(" ls      -   list the contents of the current directory on the active \n");
    printf("             media server\n");
    printf(" info    -   display media info\n");
    printf(" cd      - * traverse down one level in the content tree on the active\n");
    printf("             media server\n");
    printf(" cd ..   -   traverse up one level in the content tree on the active\n");
    printf("             media server\n");
    printf(" pwd     -   print the path from the root to your current position in the \n");
    printf("             content tree on the active media server\n");
    printf(" setmr   - * select a media renderer to become the active media renderer\n");
    printf(" getmr   -   print the friendly name of the active media renderer\n");
    printf(" open    -   set the uri on the active media renderer\n");
    printf(" play    -   play the active uri on the active media renderer\n");
    printf(" stop    -   stop the active uri on the active media renderer\n");
    printf(" seek    -   issue a seek command\n");
    printf(" mute    -   mute the active media renderer\n");
    printf(" unmute  -   unmute the active media renderer\n");

    printf(" help    -   print this help message\n\n");
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::ProcessCommandLoop
+---------------------------------------------------------------------*/
void 
    PLT_MicroMediaController::ProcessCommandLoop()
{
    char command[2048];
    bool abort = false;

    command[0] = '\0';
    while (!abort) {
        printf("command> ");
        fflush(stdout);
        fgets(command, 2048, stdin);
        strchomp(command);

        if (0 == strcmp(command, "quit") || 0 == strcmp(command, "exit")) {
            abort = true;
        } else if (0 == strcmp(command, "setms")) {
            HandleCmd_setms();
        } else if (0 == strcmp(command, "getms")) {
            HandleCmd_getms();
        } else if (0 == strncmp(command, "ls", 2)) {
            HandleCmd_ls();
        } else if (0 == strcmp(command, "info")) {
            HandleCmd_info();
        } else if (0 == strcmp(command, "cd")) {
            HandleCmd_cd(command);
        } else if (0 == strcmp(command, "cd ..")) {
            HandleCmd_cdup();
        } else if (0 == strcmp(command, "pwd")) {
            HandleCmd_pwd();
        } else if (0 == strcmp(command, "setmr")) {
            HandleCmd_setmr();
        } else if (0 == strcmp(command, "getmr")) {
            HandleCmd_getmr();
        } else if (0 == strcmp(command, "open")) {
            HandleCmd_open();
        } else if (0 == strcmp(command, "play")) {
            HandleCmd_play();
        } else if (0 == strcmp(command, "stop")) {
            HandleCmd_stop();
        } else if (0 == strncmp(command, "seek", 4)) {
            HandleCmd_seek(command);
        } else if (0 == strcmp(command, "mute")) {
            HandleCmd_mute();
        } else if (0 == strcmp(command, "unmute")) {
            HandleCmd_mute();
        } else if (0 == strcmp(command, "help")) {
            HandleCmd_help();
        } else if (0 == strcmp(command, "")) {
            // just prompt again
        } else {
            printf("Unrecognized command: %s\n", command);
            HandleCmd_help();
        }
    }
}

MediaController::MediaController()
    : m_ctrlPoint_(new PLT_CtrlPoint())
    , m_meida_controller_(m_ctrlPoint_, this)
{
    NPT_LogManager::GetDefault().Configure("plist:.level=INFO;.handlers=ConsoleHandler;.ConsoleHandler.colors=off;.ConsoleHandler.filter=63");
    m_upnp_.AddCtrlPoint(m_ctrlPoint_);
}

ErrorCode MediaController::Start()
{
    return switch_ec(m_upnp_.Start());
}

ErrorCode MediaController::Stop()
{
    return switch_ec(m_upnp_.Stop());
}

// AVTransport
void MediaController::DmrGetCurrentTransportActions(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.GetCurrentTransportActions(device, 0, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrGetCurrentTransportActions(switch_ec(ec), device_id, nullptr);
        }
    } else {
        onDmrGetCurrentTransportActions(ErrorCode::EC_NO_SUCH_ITEM, device_id, nullptr);
    }
}

void MediaController::DmrGetDeviceCapabilities(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.GetDeviceCapabilities(device, 0, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrGetDeviceCapabilities(switch_ec(ec), device_id, nullptr, nullptr, nullptr);
        }
    } else {
        onDmrGetDeviceCapabilities(ErrorCode::EC_NO_SUCH_ITEM, device_id, nullptr, nullptr, nullptr);
    }
}

void MediaController::DmrGetMediaInfo(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.GetMediaInfo(device, 0, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrGetMediaInfo(switch_ec(ec), device_id, nullptr);
        }
    } else {
        onDmrGetMediaInfo(ErrorCode::EC_NO_SUCH_ITEM, device_id, nullptr);
    }
}

void MediaController::DmrGetPositionInfo(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.GetPositionInfo(device, 0, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrGetPositionInfo(switch_ec(ec), device_id, nullptr);
        }
    } else {
        onDmrGetPositionInfo(ErrorCode::EC_NO_SUCH_ITEM, device_id, nullptr);
    }
}

void MediaController::DmrGetTransportInfo(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.GetTransportInfo(device, 0, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrGetTransportInfo(switch_ec(ec), device_id, nullptr);
        }
    } else {
        onDmrGetTransportInfo(ErrorCode::EC_NO_SUCH_ITEM, device_id, nullptr);
    }
}

void MediaController::DmrGetTransportSettings(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.GetTransportSettings(device, 0, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrGetTransportSettings(switch_ec(ec), device_id, nullptr);
        }
    } else {
        onDmrGetTransportSettings(ErrorCode::EC_NO_SUCH_ITEM, device_id, nullptr);
    }
}

void MediaController::DmrNext(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.Next(device, 0, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrNext(switch_ec(ec), device_id);
        }
    } else {
        onDmrNext(ErrorCode::EC_NO_SUCH_ITEM, device_id);
    }
}

void MediaController::DmrPause(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.Pause(device, 0, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrPause(switch_ec(ec), device_id);
        }
    } else {
        onDmrPause(ErrorCode::EC_NO_SUCH_ITEM, device_id);
    }
}

void MediaController::DmrPlay(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.Play(device, 0, "1", NULL);
        if (ec != NPT_SUCCESS) {
            onDmrPlay(switch_ec(ec), device_id);
        }
    } else {
        onDmrPlay(ErrorCode::EC_NO_SUCH_ITEM, device_id);
    }
}

void MediaController::DmrPrevious(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.Previous(device, 0, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrPrevious(switch_ec(ec), device_id);
        }
    } else {
        onDmrPrevious(ErrorCode::EC_NO_SUCH_ITEM, device_id);
    }
}

void MediaController::DmrSeek(Platform::String^ device_id, uint64 target)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_String pos;
        pos = NPT_String::Format("%.2d:%.2d:%.2d", (int)(target) / 3600, ((int)(target) % 3600)/60, ((int)(target) % 3600)%60);
        NPT_Result ec = m_meida_controller_.Seek(device, 0, "REL_TIME", pos, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrSeek(switch_ec(ec), device_id);
        }
    } else {
        onDmrSeek(ErrorCode::EC_NO_SUCH_ITEM, device_id);
    }
}

bool MediaController::DmrCanSetNextAVTransportURI(Platform::String^ device_id)
{
    return false;
}

void MediaController::DmrSetAVTransportURI(Platform::String^ device_id, Platform::String^ uri, Platform::String^ metadata)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.SetAVTransportURI(device, 0, Str2NPT_Str(uri), Str2NPT_Str(metadata), NULL);
        if (ec != NPT_SUCCESS) {
            onDmrSetAVTransportURI(switch_ec(ec), device_id);
        }
    } else {
        onDmrSetAVTransportURI(ErrorCode::EC_NO_SUCH_ITEM, device_id);
    }
}

void MediaController::DmrSetNextAVTransportURI(Platform::String^ device_id, Platform::String^next_uri, Platform::String^ next_metadata)
{
}

void MediaController::DmrSetPlayMode(Platform::String^ device_id, Platform::String^ new_play_mode)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.SetPlayMode(device, 0, Str2NPT_Str(new_play_mode), NULL);
        if (ec != NPT_SUCCESS) {
            onDmrSetPlayMode(switch_ec(ec), device_id);
        }
    } else {
        onDmrSetPlayMode(ErrorCode::EC_NO_SUCH_ITEM, device_id);
    }
}

void MediaController::DmrStop(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.Stop(device, 0, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrStop(switch_ec(ec), device_id);
        }
    } else {
        onDmrStop(ErrorCode::EC_NO_SUCH_ITEM, device_id);
    }
}

// ConnectionManager
void MediaController::DmrGetCurrentConnectionIDs(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.GetCurrentConnectionIDs(device, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrGetCurrentConnectionIDs(switch_ec(ec), device_id, nullptr);
        }
    } else {
        onDmrGetCurrentConnectionIDs(ErrorCode::EC_NO_SUCH_ITEM, device_id, nullptr);
    }
}

void MediaController::DmrGetCurrentConnectionInfo(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.GetCurrentConnectionInfo(device, 0, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrGetCurrentConnectionInfo(switch_ec(ec), device_id, nullptr);
        }
    } else {
        onDmrGetCurrentConnectionInfo(ErrorCode::EC_NO_SUCH_ITEM, device_id, nullptr);
    }
}

void MediaController::DmrGetProtocolInfo(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.GetProtocolInfo(device, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrGetProtocolInfo(switch_ec(ec), device_id, nullptr, nullptr);
        }
    } else {
        onDmrGetProtocolInfo(ErrorCode::EC_NO_SUCH_ITEM, device_id, nullptr, nullptr);
    }
}

// RenderingControl
void MediaController::DmrSetMute(Platform::String^ device_id, bool mute)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.SetMute(device, 0, "Master", mute, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrSetMute(switch_ec(ec), device_id);
        }
    } else {
        onDmrSetMute(ErrorCode::EC_NO_SUCH_ITEM, device_id);
    }
}

void MediaController::DmrGetMute(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.GetMute(device, 0, "Master", NULL);
        if (ec != NPT_SUCCESS) {
            onDmrGetMute(switch_ec(ec), device_id, false);
        }
    } else {
        onDmrGetMute(ErrorCode::EC_NO_SUCH_ITEM, device_id, false);
    }
}

void MediaController::DmrSetVolume(Platform::String^ device_id, int32 volume)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.SetVolume(device, 0, "Master", volume, NULL);
        if (ec != NPT_SUCCESS) {
            onDmrSetVolume(switch_ec(ec), device_id);
        }
    } else {
        onDmrSetVolume(ErrorCode::EC_NO_SUCH_ITEM, device_id);
    }
}

void MediaController::DmrGetVolume(Platform::String^ device_id)
{
    PLT_DeviceDataReference device;
    m_meida_controller_.FindMediaRendererByUUID(device_id, device);
    if (!device.IsNull()) {
        NPT_Result ec = m_meida_controller_.GetVolume(device, 0, "Master", NULL);
        if (ec != NPT_SUCCESS) {
            onDmrGetVolume(switch_ec(ec), device_id, 0);
        }
    } else {
        onDmrGetVolume(ErrorCode::EC_NO_SUCH_ITEM, device_id, 0);
    }
}
