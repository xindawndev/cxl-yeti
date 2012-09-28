#ifndef _AIRPLAY_WRAPPER_H_
#define _AIRPLAY_WRAPPER_H_

#include "ILibParsers.h"
#include "ILibThreadPool.h"
#include "CdsObject.h"

#if defined( ENABLED_AIRPLAY )

#if !defined(WIN32)
#   define BOOL    int
#   define TRUE    1
#   define FALSE   0
#endif

typedef enum {
    APW_ECS_SETAVTRANSPORTURI   = 0x0001,
    APW_ECS_STOP                = 0x0002,
    APW_ECS_PLAY                = 0x0004,
    APW_ECS_PAUSE               = 0x0008,
    APW_ECS_SEEKTRACK           = 0x0010,
    APW_ECS_SEEKTRACKTIME       = 0x0020,
    APW_ECS_SEEKMEDIATIME       = 0x0040,
    APW_ECS_NEXT                = 0x0080,
    APW_ECS_PREVIOUS            = 0x0100,
    APW_ECS_SETPLAYMODE         = 0x0200,
    APW_ECS_SELECTPRESET        = 0x0400,
    APW_ECS_SETBRIGHTNESS       = 0x0800,
    APW_ECS_SETCONTRAST         = 0x1000,
    APW_ECS_SETVOLUME           = 0x2000,
    APW_ECS_SETMUTE             = 0x4000,
    APW_ECS_GETAVPROTOCOLINFO   = 0x8000,
    APW_ECS_DEFAULT             = 0xffff
} APWEventContextSwitch;

typedef enum {
    APW_ERROR_OK                        = 0,
    APW_ERROR_BADTHIS                   = -1,
    APW_ERROR_BADINTERNALSTATE          = -2,
    APW_ERROR_NOTRUNNING                = -3,
    APW_ERROR_BADPLAYLIST               = -4,
    APW_ERROR_PLAYLISTTOOLARGE          = -5,
    APW_ERROR_TRACKINDEXOUTOFRANGE      = -6,
    APW_ERROR_TRANSITIONNOTSUPPORTED    = -7,
    APW_ERROR_VOLUMEOUTOFRANGE          = -8,
    APW_ERROR_CONTRASTOUTOFRANGE        = -9,
    APW_ERROR_BRIGHTNESSOUTOFRANGE      = -10,
    APW_ERROR_REJECTURI                 = -11,
    APW_ERROR_REJECTMETADATA            = -12,
    APW_ERROR_BADURI                    = -13,
    APW_ERROR_BADMETADATA               = -14,
    APW_ERROR_BADPLAYSTATE              = -15,
    APW_ERROR_BADRECORDSTATE            = -16,
    APW_ERROR_BADMEDIAPLAYMODE          = -17,
    APW_ERROR_BADTRANSPORTSTATUS        = -18,
    APW_ERROR_BADPLAYCOMMAND            = -19,
    APW_ERROR_BADRECORDCOMMAND          = -20,
    APW_ERROR_NOTHREADPOOL              = -21,
    APW_ERROR_INVALIDARGUMENT           = -22,
    APW_ERROR_OUTOFMEMORY               = -23,
    APW_ERROR_BADPLAYSPEED              = -24
} APW_Error;

typedef enum {
    APW_PS_NoMedia                      = 0,
    APW_PS_Stopped                      = 1,
    APW_PS_Paused                       = 2,
    APW_PS_Playing                      = 3,
    APW_PS_Transitioning                = 4
} APWPlayState;

typedef enum {
    APW_MPM_Normal                      = 0,
    APW_MPM_Repeat_One                  = 1,
    APW_MPM_Repeat_All                  = 2,
    APW_MPM_Random                      = 3,
    APW_MPM_Shuffle                     = 4,
    APW_MPM_Direct_One                  = 5,
    APW_MPM_Intro                       = 6
} APWMediaPlayMode;

typedef enum {
    APW_TS_OK                           = 0,
    APW_TS_ERROR_OCCURRED               = 1
} APWTransportStatus;

struct _APW;

typedef struct _APW * APW;
typedef int (* APWCallback_SetAVTransportURI) (  APW instance, void * session, char * uri, char * metadata);
typedef int (* APWCallback_GetAVProtocolInfo) (  APW instance, void * session, char ** protocolInfo);
typedef int (* APWCallback_SetPlayMode) (        APW instance, void * session, APWMediaPlayMode play_mode);
typedef int (* APWCallback_Stop) (               APW instance, void * session);
typedef int (* APWCallback_Play) (               APW instance, void * session, char* playSpeed);
typedef int (* APWCallback_Pause) (              APW instance, void * session);
typedef int (* APWCallback_SeekTrack) (          APW instance, void * session, unsigned int trackIndex);
typedef int (* APWCallback_SeekTrackPosition) (  APW instance, void * session, long position);
typedef int (* APWCallback_SeekMediaPosition) (  APW instance, void * session, long position);
typedef int (* APWCallback_Next) (               APW instance, void * session);
typedef int (* APWCallback_Previous) (           APW instance, void * session);
typedef int (* APWCallback_SelectPreset) (       APW instance, void * session, char* presetName);
typedef int (* APWCallback_SetVolume) (          APW instance, void * session, unsigned char volume);
typedef int (* APWCallback_SetMute) (            APW instance, void * session, BOOL mute);
typedef int (* APWCallback_SetContrast) (        APW instance, void * session, unsigned char contrast);
typedef int (* APWCallback_SetBrightness) (      APW instance, void * session, unsigned char brightness);

struct _APW
{
    void (* ILib1)(void * object,fd_set * readset, fd_set * writeset, fd_set * errorset, int * blocktime);
    void (* ILib2)(void * object, int slct, fd_set * readset, fd_set * writeset, fd_set * errorset);
    void (* ILib3)(void * object);

    void *                                       internal_state;
    void *                                       tag;
    ILibThreadPool                               thread_pool;

    APWCallback_SetAVTransportURI                Event_SetAVTransportURI;
    APWCallback_GetAVProtocolInfo                Event_GetAVProtocolInfo;
    APWCallback_SetPlayMode                      Event_SetPlayMode;
    APWCallback_Stop                             Event_Stop;
    APWCallback_Play                             Event_Play;
    APWCallback_Pause                            Event_Pause;
    APWCallback_SeekTrack                        Event_SeekTrack;
    APWCallback_SeekTrackPosition                Event_SeekTrackPosition;
    APWCallback_SeekMediaPosition                Event_SeekMediaPosition;
    APWCallback_Next                             Event_Next;
    APWCallback_Previous                         Event_Previous;
    APWCallback_SelectPreset                     Event_SelectPreset;
    APWCallback_SetVolume                        Event_SetVolume;
    APWCallback_SetMute                          Event_SetMute;
    APWCallback_SetContrast                      Event_SetContrast;
    APWCallback_SetBrightness                    Event_SetBrightness;
};

APW       APW_Method_Create(
                            void * chain,
                            ILibThreadPool thread_pool,
                            unsigned short port,
                            char* friendly_name,
                            char * mac_addr,
                            char * pwd);
BOOL      APW_Method_IsRunning(                         APW instance);
void      APW_Method_NotifyMicrostackOfIPAddressChange( APW instance);
APW_Error APW_Method_SetEventContextMask(               APW instance, APWEventContextSwitch bit_flags);
APW_Error APW_Method_SetDeviceCapabilities(             APW instance, const char * play_media, const char * rec_media, const char * rec_quality_modes);
void      APW_Method_ErrorEventResponse(                void * session, int error_code, char * error_message);
BOOL      APW_Method_AddPresetNameToList(               APW instance, const char* name);
APW_Error APW_StateChange_SinkProtocolInfo(             APW instance, char* info);
APW_Error APW_StateChange_TransportPlayState(           APW instance, APWPlayState state);
APW_Error APW_StateChange_TransportPlaySpeed(           APW instance, char* play_speed);
APW_Error APW_StateChange_TransportStatus(              APW instance, APWTransportStatus status);
APW_Error APW_StateChange_CurrentTransportActions(      APW instance, unsigned short allowed_actions);
APW_Error APW_StateChange_NumberOfTracks(               APW instance, unsigned int max_number_of_tracks);
APW_Error APW_StateChange_CurrentTrack(                 APW instance, unsigned int index);
APW_Error APW_StateChange_CurrentPlayMode(              APW instance, APWMediaPlayMode mode);
APW_Error APW_StateChange_CurrentTrackURI(              APW instance, char* track_uri);
APW_Error APW_StateChange_CurrentTrackMetaData(         APW instance, struct CdsObject* track_metadata);
APW_Error APW_StateChange_CurrentTrackDuration(         APW instance, long duration);
APW_Error APW_StateChange_Volume(                       APW instance, unsigned char volume);
APW_Error APW_StateChange_Mute(                         APW instance, BOOL mute);
APW_Error APW_StateChange_Contrast(                     APW instance, unsigned char contrast);
APW_Error APW_StateChange_Brightness(                   APW instance, unsigned char brightness);
APW_Error APW_StateChange_AVTransportURI(               APW instance, char* uri);
APW_Error APW_StateChange_AVTransportURIMetaData(       APW instance, struct CdsObject* metadata);
APW_Error APW_StateChange_CurrentMediaDuration(         APW instance, long duration);
APW_Error APW_StateChange_AbsoluteTimePosition(         APW instance, long position, int is_notify);
APW_Error APW_StateChange_RelativeTimePosition(         APW instance, long position, int is_notify);

#endif // ENABLED_AIRPLAY

#endif // _AIRPLAY_WRAPPER_H_
