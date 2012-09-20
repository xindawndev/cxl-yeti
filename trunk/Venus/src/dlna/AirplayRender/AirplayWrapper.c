#include "AirplayRender.h"
#include "AirplayWrapper.h"
#include "ILibWebServer.h"

#define TESTMASK(x, y)          (((x & y) == y)? TRUE: FALSE)
#define MALLOC(x)               memset(malloc((size_t)x), 0, (size_t)x)
#define FREE(x)                 if(x != NULL) free(x)
#define String_Create(x)        (char *)strcpy((char *)malloc(strlen(x) + 1), x)
#define String_CreateSize(x)    (char *)memset(malloc((size_t)(x + 1)), 0, 1)
#define String_Destroy(x)       FREE(x)

#define EVENT_CURRENTPLAYMODE   0x00000001

#ifdef WIN32
typedef unsigned __int64 methods_param;
#else
typedef void *           methods_param;
#endif

typedef struct _contex_method_call {
    APW                     apw;
    AirplaySessionToken     session;
    APWEventContextSwitch   method;
    int                     param_count;
    methods_param           params[16];
} *contex_method_call;

BOOL add_method_param(contex_method_call contex, methods_param param)
{
    if (contex->param_count == 16) {
        return FALSE;
    }
    contex->params[contex->param_count++] = param;
    return TRUE;
}

contex_method_call create_method(APWEventContextSwitch method_id, APW instance,AirplaySessionToken session)
{
    contex_method_call result = MALLOC(sizeof(struct _contex_method_call));
    if (result != NULL) {
        result->session = session;
        result->method  = method_id;
        result->apw     = instance;
    }
    return result;
}

typedef struct _APWInternalState {
    void * chain;
    void * airplay_token;
    void * apw_monitor;

    char * udn;
    unsigned short seconds;
    unsigned short port;

    char* FriendlyName;
    char* SerialNumber;
    char* ProtocolInfo;

    char* PlayMedia;
    char* RecMedia;
    char* RecQualityModes;

    APWEventContextSwitch  EventsOnThreadBitMask;

    unsigned long last_change_mask;
    unsigned short Brightness;
    unsigned short Contrast;
    unsigned short Volume;
    BOOL Mute;
    APWMediaPlayMode CurrentPlayMode;
    APWPlayState transport_state;
    APWTransportStatus TransportStatus;
    char* TransportPlaySpeed;
    char* AVTransportURI;
    char* AVTransportURIMetaData;
    unsigned short CurrentTransportActions;
    long CurrentMediaDuration; /* in ms. */
    unsigned int NumberOfTracks;
    unsigned int CurrentTrack;
    long CurrentTrackDuration; /* in ms. */
    char* CurrentTrackMetaData;
    char* CurrentTrackURI;
    long AbsoluteTimePosition; /* in ms. */
    long RelativeTimePosition; /* in ms. */

    sem_t   resource_lock;
} * APWInternalState;

APW_Error check_this(APW instance)
{
    if (instance != NULL) {
        APWInternalState state = (APWInternalState)instance->internal_state;
        if (state != NULL) {
            return APW_ERROR_OK;
        } else {
            return APW_ERROR_BADINTERNALSTATE;
        }
    } else {
        return APW_ERROR_BADTHIS;
    }
}
APW get_apw_from_session_token(AirplaySessionToken session_token)
{
    return (APW)AirplayGetTag(((struct ILibWebServer_Session *)session_token)->User);
}

void apw_lock(APW instance)
{
    if (check_this(instance) == APW_ERROR_OK) {
        APWInternalState state = (APWInternalState)instance->internal_state;
        sem_wait(&state->resource_lock);
    }
}

void apw_unlock(APW instance)
{
    if (check_this(instance) == APW_ERROR_OK) {
        APWInternalState state = (APWInternalState)instance->internal_state;
        sem_post(&state->resource_lock);
    }
}

// 发送状态事件
void fire_last_change_event(APW apw)
{
    APWInternalState state = (APWInternalState)apw->internal_state;
    if (TESTMASK(state->last_change_mask, EVENT_CURRENTPLAYMODE) == TRUE) {
        AirplaySetState_LastChange(state->airplay_token, state->transport_state);
    }
}

void apw_last_change_timer_event(void * object)
{
    APWInternalState state;
    APW apw         = (APW)object;
    APW_Error ec    = check_this(apw);
    if (ec != APW_ERROR_OK) {
        return;
    }
    state           = (APWInternalState)apw->internal_state;
    if (state->last_change_mask != 0) {
        fire_last_change_event(apw);
        state->last_change_mask = 0;
    }

    ILibLifeTime_AddEx(state->apw_monitor, apw, 200, &apw_last_change_timer_event, NULL);
}

void callback_from_thread_pool(ILibThreadPool thread_pool, void * methods)
{
    APW instance = NULL;
    contex_method_call method = (contex_method_call)methods;
    if (method == NULL) {
        return;
    }

    instance = method->apw;

    switch (method->method) {
    case APW_ECS_GETAVPROTOCOLINFO:
        // 无论如何都返回没有实现
        AirplayResponse_Error((const AirplaySessionToken)method->session, 405, "Method Not Allowed");
        break;
    case APW_ECS_SETAVTRANSPORTURI:
        {
            if (instance->Event_SetAVTransportURI != NULL && method->param_count == 2) {
                int result;
                char * uri = (char *)method->params[0];
                char * metadata = (char *)method->params[1];
                result = instance->Event_SetAVTransportURI(instance, method->session, uri, metadata);
                String_Destroy(uri);
                String_Destroy(metadata);
            } else if(instance->Event_SetAVTransportURI == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_STOP:
        {
            if (instance->Event_Stop != NULL && method->param_count == 0) {
                int result = instance->Event_Stop(instance, method->session);
            } else if(instance->Event_Stop == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_PLAY:
        {
            if (instance->Event_Play != NULL && method->param_count == 1) {
                int result;
                char * play_speed = (char *)method->params[0];
                result = instance->Event_Play(instance, method->session, play_speed);
                String_Destroy(play_speed);
            } else if(instance->Event_Play == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_PAUSE:
        {
            if (instance->Event_Pause != NULL && method->param_count == 0) {
                int result = instance->Event_Pause(instance, method->session);
            } else if(instance->Event_Pause == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_SEEKTRACK:
        {
            if (instance->Event_SeekTrack != NULL && method->param_count == 1) {
                int result;
                unsigned int track = (unsigned int)(unsigned long)method->params[0];
                result = instance->Event_SeekTrack(instance, method->session, track);
            } else if(instance->Event_SeekTrack == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_SEEKTRACKTIME:
        {
            if (instance->Event_SeekTrackPosition != NULL && method->param_count == 1) {
                int result;
                long position = (long)method->params[0];
                result = instance->Event_SeekTrackPosition(instance, method->session, position);
            } else if(instance->Event_SeekTrackPosition == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_SEEKMEDIATIME:
        {
            if (instance->Event_SeekMediaPosition != NULL && method->param_count == 1) {
                int result;
                long position = (long)method->params[0];
                result = instance->Event_SeekMediaPosition(instance, method->session, position);
            } else if(instance->Event_SeekMediaPosition == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_NEXT:
        {
            if (instance->Event_Next != NULL && method->param_count == 0) {
                int result = instance->Event_Next(instance, method->session);
            } else if(instance->Event_Next == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_PREVIOUS:
        {
            if (instance->Event_Previous != NULL && method->param_count == 0) {
                int result = instance->Event_Previous(instance, method->session);
            } else if(instance->Event_Previous == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_SETPLAYMODE:
        {
            if (instance->Event_SetPlayMode != NULL && method->param_count == 1) {
                int result;
                APWMediaPlayMode mode = (APWMediaPlayMode)method->params[0];
                result = instance->Event_SetPlayMode(instance, method->session, mode);
            } else if(instance->Event_SetPlayMode == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_SELECTPRESET:
        {
            if (instance->Event_SelectPreset != NULL && method->param_count == 1) {
                int result;
                char* preset = (char*)method->params[0];
                result = instance->Event_SelectPreset(instance, method->session, preset);
                String_Destroy(preset);
            } else if(instance->Event_SelectPreset == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_SETBRIGHTNESS:
        {
            if (instance->Event_SetBrightness != NULL && method->param_count == 1) {
                int result;
                unsigned char val = (unsigned char)method->params[0];
                result = instance->Event_SetBrightness(instance, method->session, val);
            } else if(instance->Event_SetBrightness == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_SETCONTRAST:
        {
            if (instance->Event_SetContrast != NULL && method->param_count == 1) {
                int result;
                unsigned char val = (unsigned char)method->params[0];
                result = instance->Event_SetContrast(instance, method->session, val);
            } else if(instance->Event_SetContrast == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_SETVOLUME:
        {
            if (instance->Event_SetVolume != NULL && method->param_count == 1) {
                int result;
                unsigned char val = (unsigned char)method->params[0];
                result = instance->Event_SetVolume(instance, method->session, val);
            } else if(instance->Event_SetVolume == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    case APW_ECS_SETMUTE:
        {
            if (instance->Event_SetMute != NULL && method->param_count == 1) {
                int result;
                BOOL val = (BOOL)method->params[0];
                result = instance->Event_SetMute(instance, method->session, val);
            } else if(instance->Event_SetMute == NULL) {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Action Not Implemented");
            } else {
                AirplayResponse_Error((const AirplaySessionToken)method->session, 501, "Action Failed: Program Error");
            }
        }
        break;
    default:
        ILibWebServer_Release((struct ILibWebServer_Session *)method->session);
        FREE(method);
        return;
    }
    ILibWebServer_Release((struct ILibWebServer_Session *)method->session);
    FREE(method);
}

APW_Error call_method_through_thread_pool(APW instance, contex_method_call method)
{
    APWInternalState state;
    BOOL contextSwitch = FALSE;
    APW_Error err = check_this(instance);
    if (err != APW_ERROR_OK) {
        return err;
    }
    state = (APWInternalState)instance->internal_state;
    contextSwitch = TESTMASK(state->EventsOnThreadBitMask, method->method);
    switch (method->method) {
    case APW_ECS_GETAVPROTOCOLINFO:
        break;
    case APW_ECS_SETAVTRANSPORTURI:
        if (instance->Event_SetAVTransportURI != NULL && method->param_count == 2) {
            method->params[0] = (methods_param)(void *)String_Create((const char*)method->params[0]);
            method->params[1] = (methods_param)(void *)String_Create((const char*)method->params[1]);
        }
        break;
    case APW_ECS_STOP:
        break;
    case APW_ECS_PLAY:
        if (instance->Event_Play != NULL && method->param_count == 1) {
            method->params[0] = (methods_param)(void *)String_Create((const char *)method->params[0]);
        }
        break;
    case APW_ECS_PAUSE:
        break;
    case APW_ECS_SEEKTRACK:
        break;
    case APW_ECS_SEEKTRACKTIME:
        break;
    case APW_ECS_SEEKMEDIATIME:
        break;
    case APW_ECS_NEXT:
        break;
    case APW_ECS_PREVIOUS:
        break;
    case APW_ECS_SETPLAYMODE:
        break;
    case APW_ECS_SELECTPRESET:
        if (instance->Event_SelectPreset != NULL && method->param_count == 1) {
            method->params[0] = (methods_param)(void *)String_Create((const char *)method->params[0]);
        }
        break;
    case APW_ECS_SETBRIGHTNESS:
        break;
    case APW_ECS_SETCONTRAST:
        break;
    case APW_ECS_SETVOLUME:
        break;
    case APW_ECS_SETMUTE:
        break;
    default:
        return APW_ERROR_INVALIDARGUMENT;
    }

    ILibWebServer_AddRef((struct ILibWebServer_Session *)method->session);
    if (contextSwitch == TRUE) {
        ILibThreadPool_QueueUserWorkItem(instance->thread_pool, (void *)method, &callback_from_thread_pool);
    } else {
        callback_from_thread_pool(NULL, (void *)method);
    }

    return APW_ERROR_OK;
}

void APW_GetCurrentTransportActions (void * session_token, unsigned int instance_id)
{}

// 获取设备能力
void APW_GetDeviceCapabilities      (void * session_token, unsigned int instance_id)
{}

void APW_GetMediaInfo               (void * session_token, unsigned int instance_id)
{}

// 获取当前播放位置
void APW_GetPositionInfo            (void * session_token, unsigned int instance_id)
{}

void APW_GetTransportInfo           (void * session_token, unsigned int instance_id)
{}

void APW_GetTransportSettings       (void * session_token, unsigned int instance_id)
{}

// 下一个
void APW_Next                       (void * session_token, unsigned int instance_id)
{}

// 暂停
void APW_Pause                      (void * session_token, unsigned int instance_id)
{
    APW instance = get_apw_from_session_token(session_token);
    contex_method_call method = NULL;

    printf("Invodk: APW_Pause(%u);\n", instance_id);

    if (instance_id != 0) {
        AirplayResponse_Error(session_token, 718, "Invalid InstanceID");
        return;
    }
    if (check_this(instance) != APW_ERROR_OK) {
        AirplayResponse_Error(session_token, 501, "Action Failed");
        return;
    }

    method = create_method(APW_ECS_PAUSE, instance, session_token);
    call_method_through_thread_pool(instance, method);
}

// 播放
void APW_Play                       (void * session_token, unsigned int instance_id, char * speed)
{
    APW instance = get_apw_from_session_token(session_token);
    contex_method_call method = NULL;

    printf("Invoke: APW_Play(%u, %s);\n",instance_id, speed);

    if (instance_id != 0) {
        AirplayResponse_Error(session_token, 718, "Invalid InstanceID");
        return;
    }
    if (check_this(instance) != APW_ERROR_OK) {
        AirplayResponse_Error(session_token, 501, "Action Failed");
        return;
    }

    method = create_method(APW_ECS_PLAY, instance, session_token);
    add_method_param(method, (methods_param)speed);

    call_method_through_thread_pool(instance, method);
}

void APW_Previous                   (void * session_token, unsigned int instance_id)
{}

// 拖动
void APW_Seek                       (void * session_token, unsigned int instance_id, char * units, char * target)
{
    APW instance = get_apw_from_session_token(session_token);
    contex_method_call method = NULL;
    int h = 0, m = 0, s = 0, validargs = 0;

    printf("Invoke: APW_Seek(%u, %s, %s);\n", instance_id, units, target);
    if (instance_id != 0) {
        AirplayResponse_Error(session_token, 718, "Invalid InstanceID");
        return;
    }
    if (check_this(instance) != APW_ERROR_OK) {
        AirplayResponse_Error(session_token, 501, "Action Failed");
        return;
    }
    if (strcmp((const char *)units, "TRACK_NR") == 0) {
        int target_pos = atoi(target);
        method = create_method(APW_ECS_SEEKTRACK, instance, session_token);
        add_method_param(method, (methods_param)target_pos);

        call_method_through_thread_pool(instance, method);
    } else if(strcmp(units, "ABS_TIME") == 0) {
        long target_pos = atol(target);
        method = create_method(APW_ECS_SEEKMEDIATIME, instance, session_token);
        add_method_param(method, (methods_param)target_pos);

        call_method_through_thread_pool(instance, method);
    } else if(strcmp(units, "REL_TIME") == 0) {
        long target_pos = 0;
        validargs = sscanf(target, "%d:%d:%d", &h, &m, &s);
        if ( validargs != 3 ) {
            AirplayResponse_Error(session_token, 402, "Invalid Args");
            return;
        }
        target_pos = h * 3600 + m * 60 + s;

        method = create_method(APW_ECS_SEEKTRACKTIME, instance, session_token);
        add_method_param(method, (methods_param)target_pos);

        call_method_through_thread_pool(instance, method);
    } else {
        AirplayResponse_Error(session_token, 710, "Seek Mode Not Supported");
    }
}

// 设置播放串
void APW_SetAVTransportURI          (void * session_token, unsigned int instance_id, char * current_uri,char * current_uri_metadata)
{
    APW instance = get_apw_from_session_token(session_token);
    contex_method_call method = NULL;

    printf("Invoke: APW_SetAVTransportURI(%u, %s, %s);\n", instance_id, current_uri, current_uri_metadata ? current_uri_metadata: "");

    if (instance_id != 0) {
        AirplayResponse_Error(session_token, 718, "Invalid InstanceID");
        return;
    }
    if (check_this(instance) != APW_ERROR_OK) {
        AirplayResponse_Error(session_token, 501, "Action Failed");
        return;
    }

    method = create_method(APW_ECS_SETAVTRANSPORTURI, instance, session_token);
    current_uri = (current_uri ? current_uri : "");
    add_method_param(method, (methods_param)current_uri);
    current_uri_metadata = (current_uri_metadata ? current_uri_metadata : "");
    add_method_param(method, (methods_param)current_uri_metadata);

    call_method_through_thread_pool(instance, method);
}

// 设置播放模式
void APW_SetPlayMode                (void * session_token, unsigned int instance_id, char * new_play_mode)
{}

// 停止
void APW_Stop                       (void * session_token, unsigned int instance_id)
{
    APW instance = get_apw_from_session_token(session_token);
    contex_method_call method = NULL;

    printf("Invodk: APW_Stop(%u);\n", instance_id);

    if (instance_id != 0) {
        AirplayResponse_Error(session_token, 718, "Invalid InstanceID");
        return;
    }
    if (check_this(instance) != APW_ERROR_OK) {
        AirplayResponse_Error(session_token, 501, "Action Failed");
        return;
    }

    method = create_method(APW_ECS_STOP, instance, session_token);
    call_method_through_thread_pool(instance, method);
}

// 获取当前连接ID
void APW_GetCurrentConnectionIDs    (void * session_token)
{}

// 获取连接信息
void APW_GetCurrentConnectionInfo   (void * session_token, unsigned int instance_id)
{}

// 获取协议信息
void APW_GetProtocolInfo            (void * session_token)
{}

void APW_ListPresets                (void * session_token, unsigned int instance_id)
{}

void APW_SelectPreset               (void * session_token, unsigned int instance_id, char * preset_name)
{}

// 获取明亮度
void APW_GetBrightness              (void * session_token, unsigned int instance_id)
{}

// 获取对比度
void APW_GetContrast                (void * session_token, unsigned int instance_id)
{}

// 设置明亮度
void APW_SetBrightness              (void * session_token, unsigned int instance_id, unsigned short desired_brightness)
{}

// 设置对比度
void APW_SetContrast                (void * session_token, unsigned int instance_id, unsigned short desired_contrast)
{}

// 获取是否静音
void APW_GetMute                    (void * session_token, unsigned int instance_id, char * channel)
{
    APW instance = get_apw_from_session_token(session_token);
    APWInternalState state = NULL;
    contex_method_call method = NULL;

    printf("Invoke: APW_GetMute(%u, %s);\n", instance_id, channel);

    if (instance_id != 0) {
        AirplayResponse_Error(session_token, 718, "Invalid InstanceID");
        return;
    }
    if (check_this(instance) != APW_ERROR_OK) {
        AirplayResponse_Error(session_token, 501, "Action Failed");
        return;
    }

    state = (APWInternalState)instance->internal_state;
    if (strcmp(channel, "Master") != 0) {
        AirplayResponse_Error(session_token, 600, "Argument Value Invalid");
        return;
    }

    // 将当前是否静音信息发送给AirplayRender：state->Mute ? 1: 0;
}

// 获取当前音量
void APW_GetVolume                  (void * session_token, unsigned int instance_id, char * channel)
{
    APW instance = get_apw_from_session_token(session_token);
    APWInternalState state = NULL;
    contex_method_call method = NULL;

    printf("Invoke: APW_GetVolume(%u, %s);\n", instance_id, channel);

    if (instance_id != 0) {
        AirplayResponse_Error(session_token, 718, "Invalid InstanceID");
        return;
    }
    if (check_this(instance) != APW_ERROR_OK) {
        AirplayResponse_Error(session_token, 501, "Action Failed");
        return;
    }

    state = (APWInternalState)instance->internal_state;
    if (strcmp(channel, "Master") != 0) {
        AirplayResponse_Error(session_token, 600, "Argument Value Invalid");
        return;
    }

    // 将当前音量信息发送给AirplayRender：state->Volume;
}

// 设置静音
void APW_SetMute                    (void * session_token, unsigned int instance_id, char * channel, int desire_mute)
{
    APW instance = get_apw_from_session_token(session_token);
    APWInternalState state = NULL;
    contex_method_call method = NULL;

    printf("Invoke: APW_SetMute(%u, %s, %d);\n", instance_id, channel, desire_mute);

    if (instance_id != 0) {
        AirplayResponse_Error(session_token, 718, "Invalid InstanceID");
        return;
    }
    if (check_this(instance) != APW_ERROR_OK) {
        AirplayResponse_Error(session_token, 501, "Action Failed");
        return;
    }

    state = (APWInternalState)instance->internal_state;
    if (strcmp(channel, "Master") != 0) {
        AirplayResponse_Error(session_token, 600, "Argument Value Invalid");
        return;
    }

    method = create_method(APW_ECS_SETMUTE, instance, session_token);
    add_method_param(method, (methods_param)desire_mute);
    call_method_through_thread_pool(instance, method);
}

// 设置音量
void APW_SetVolume                  (void * session_token, unsigned int instance_id, char * channel, unsigned short desired_volume)
{
    APW instance = get_apw_from_session_token(session_token);
    APWInternalState state = NULL;
    contex_method_call method = NULL;

    printf("Invoke: APW_SetVolume(%u, %s, %u);\n", instance_id, channel, desired_volume);

    if (instance_id != 0) {
        AirplayResponse_Error(session_token, 718, "Invalid InstanceID");
        return;
    }
    if (check_this(instance) != APW_ERROR_OK) {
        AirplayResponse_Error(session_token, 501, "Action Failed");
        return;
    }

    state = (APWInternalState)instance->internal_state;
    if (strcmp(channel, "Master") != 0) {
        AirplayResponse_Error(session_token, 600, "Argument Value Invalid");
        return;
    }

    method = create_method(APW_ECS_SETVOLUME, instance, session_token);
    add_method_param(method, (methods_param)desired_volume);
    call_method_through_thread_pool(instance, method);
}

void apw_destroy_from_chain(APW instance)
{
    if (instance != NULL) {
        APWInternalState state = (APWInternalState)instance->internal_state;
        if (state != NULL) {
            sem_destroy(&state->resource_lock);
            FREE(instance->internal_state);
        }
    }
}

APW APW_Method_Create(void * chain, ILibThreadPool thread_pool, unsigned short port, char* friendly_name, char * mac_addr, char * pwd)
{
    APW apw = NULL;
    APWInternalState inner_state = NULL;

    apw = (APW)MALLOC(sizeof(struct _APW));
    if (apw == NULL) {
        return NULL;
    }
    inner_state = (APWInternalState)MALLOC(sizeof(struct _APWInternalState));
    if (inner_state == NULL) {
        FREE(apw);
        return NULL;
    }
    inner_state->chain = chain;
    inner_state->airplay_token = AirplayCreate(chain, port, friendly_name, mac_addr, pwd);
    AirplaySetTag(inner_state->airplay_token, (void *)apw);
    inner_state->apw_monitor = ILibCreateLifeTime(inner_state->chain);
    ILibLifeTime_AddEx(inner_state->apw_monitor, apw, 200, &apw_last_change_timer_event, NULL);
    sem_init(&inner_state->resource_lock, 0, 1);

    apw->ILib1 = NULL;
    apw->ILib2 = NULL;
    apw->ILib3 = apw_destroy_from_chain;
    apw->internal_state = (void *)inner_state;
    apw->thread_pool = thread_pool;

    AirplayCallbackGetCurrentTransportActions   = (AirplayHandlerGetCurrentTransportActions )&APW_GetCurrentTransportActions;
    AirplayCallbackGetDeviceCapabilities        = (AirplayHandlerGetDeviceCapabilities      )&APW_GetDeviceCapabilities;
    AirplayCallbackGetMediaInfo                 = (AirplayHandlerGetMediaInfo               )&APW_GetMediaInfo;
    AirplayCallbackGetPositionInfo              = (AirplayHandlerGetPositionInfo            )&APW_GetPositionInfo;
    AirplayCallbackGetTransportInfo             = (AirplayHandlerGetTransportInfo           )&APW_GetTransportInfo;
    AirplayCallbackGetTransportSettings         = (AirplayHandlerGetTransportSettings       )&APW_GetTransportSettings;
    AirplayCallbackNext                         = (AirplayHandlerNext                       )&APW_Next;
    AirplayCallbackPause                        = (AirplayHandlerPause                      )&APW_Pause;
    AirplayCallbackPlay                         = (AirplayHandlerPlay                       )&APW_Play;
    AirplayCallbackPrevious                     = (AirplayHandlerPrevious                   )&APW_Previous;
    AirplayCallbackSeek                         = (AirplayHandlerSeek                       )&APW_Seek;
    AirplayCallbackSetAVTransportURI            = (AirplayHandlerSetAVTransportURI          )&APW_SetAVTransportURI;
    AirplayCallbackSetPlayMode                  = (AirplayHandlerSetPlayMode                )&APW_SetPlayMode;
    AirplayCallbackStop                         = (AirplayHandlerStop                       )&APW_Stop;
    AirplayCallbackGetCurrentConnectionIDs      = (AirplayHandlerGetCurrentConnectionIDs    )&APW_GetCurrentConnectionIDs;
    AirplayCallbackGetCurrentConnectionInfo     = (AirplayHandlerGetCurrentConnectionInfo   )&APW_GetCurrentConnectionInfo;
    AirplayCallbackGetProtocolInfo              = (AirplayHandlerGetProtocolInfo            )&APW_GetProtocolInfo;
    AirplayCallbackListPresets                  = (AirplayHandlerListPresets                )&APW_ListPresets;
    AirplayCallbackSelectPreset                 = (AirplayHandlerSelectPreset               )&APW_SelectPreset;
    AirplayCallbackGetBrightness                = (AirplayHandlerGetBrightness              )&APW_GetBrightness;
    AirplayCallbackGetContrast                  = (AirplayHandlerGetContrast                )&APW_GetContrast;
    AirplayCallbackSetBrightness                = (AirplayHandlerSetBrightness              )&APW_SetBrightness;
    AirplayCallbackSetContrast                  = (AirplayHandlerSetContrast                )&APW_SetContrast;
    AirplayCallbackGetMute                      = (AirplayHandlerGetMute                    )&APW_GetMute;
    AirplayCallbackGetVolume                    = (AirplayHandlerGetVolume                  )&APW_GetVolume;
    AirplayCallbackSetMute                      = (AirplayHandlerSetMute                    )&APW_SetMute;
    AirplayCallbackSetVolume                    = (AirplayHandlerSetVolume                  )&APW_SetVolume;

    ILibAddToChain(chain, apw);

    return apw;
}
