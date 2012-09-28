#include "ILibParsers.h"
#include "AirplayRender.h"

#if defined( ENABLED_AIRPLAY )

#include "ILibWebServer.h"
#include "ILibDnssd.h"
#include "ILibMd5.h"
//#include "plist/plist.h"

#define RECEIVEBUFFER 1024

#define AIRPLAY_STATUS_OK                  200
#define AIRPLAY_STATUS_SWITCHING_PROTOCOLS 101
#define AIRPLAY_STATUS_NEED_AUTH           401
#define AIRPLAY_STATUS_NOT_FOUND           404
#define AIRPLAY_STATUS_METHOD_NOT_ALLOWED  405
#define AIRPLAY_STATUS_NOT_IMPLEMENTED     501
#define AIRPLAY_STATUS_NO_RESPONSE_NEEDED  1000

#define EVENT_NONE     -1
#define EVENT_STOPPED   1
#define EVENT_PAUSED    2
#define EVENT_PLAYING   3
#define EVENT_LOADING   4
const char *eventStrings[] = {"stopped", "paused", "playing", "loading"};

#define PLAYBACK_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
    "<plist version=\"1.0\">\r\n"\
    "<dict>\r\n"\
    "<key>duration</key>\r\n"\
    "<real>%f</real>\r\n"\
    "<key>loadedTimeRanges</key>\r\n"\
    "<array>\r\n"\
    "\t\t<dict>\r\n"\
    "\t\t\t<key>duration</key>\r\n"\
    "\t\t\t<real>%f</real>\r\n"\
    "\t\t\t<key>start</key>\r\n"\
    "\t\t\t<real>0.0</real>\r\n"\
    "\t\t</dict>\r\n"\
    "</array>\r\n"\
    "<key>playbackBufferEmpty</key>\r\n"\
    "<true/>\r\n"\
    "<key>playbackBufferFull</key>\r\n"\
    "<false/>\r\n"\
    "<key>playbackLikelyToKeepUp</key>\r\n"\
    "<true/>\r\n"\
    "<key>position</key>\r\n"\
    "<real>%f</real>\r\n"\
    "<key>rate</key>\r\n"\
    "<real>%d</real>\r\n"\
    "<key>readyToPlay</key>\r\n"\
    "<true/>\r\n"\
    "<key>seekableTimeRanges</key>\r\n"\
    "<array>\r\n"\
    "\t\t<dict>\r\n"\
    "\t\t\t<key>duration</key>\r\n"\
    "\t\t\t<real>%f</real>\r\n"\
    "\t\t\t<key>start</key>\r\n"\
    "\t\t\t<real>0.0</real>\r\n"\
    "\t\t</dict>\r\n"\
    "</array>\r\n"\
    "</dict>\r\n"\
    "</plist>\r\n"

#define PLAYBACK_INFO_NOT_READY  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
    "<plist version=\"1.0\">\r\n"\
    "<dict>\r\n"\
    "<key>readyToPlay</key>\r\n"\
    "<false/>\r\n"\
    "</dict>\r\n"\
    "</plist>\r\n"

#define SERVER_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
    "<plist version=\"1.0\">\r\n"\
    "<dict>\r\n"\
    "<key>deviceid</key>\r\n"\
    "<string>%s</string>\r\n"\
    "<key>features</key>\r\n"\
    "<integer>119</integer>\r\n"\
    "<key>model</key>\r\n"\
    "<string>AppleTV2,1</string>\r\n"\
    "<key>protovers</key>\r\n"\
    "<string>1.0</string>\r\n"\
    "<key>srcvers</key>\r\n"\
    "<string>"AIRPLAY_SERVER_VERSION_STR"</string>\r\n"\
    "</dict>\r\n"\
    "</plist>\r\n"

#define EVENT_INFO "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\r\n"\
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\r\n"\
    "<plist version=\"1.0\">\r\n"\
    "<dict>\r\n"\
    "<key>category</key>\r\n"\
    "<string>video</string>\r\n"\
    "<key>state</key>\r\n"\
    "<string>%s</string>\r\n"\
    "</dict>\r\n"\
    "</plist>\r\n"\

#define AUTH_REALM "AirPlay"
#define AUTH_REQUIRED "WWW-Authenticate: Digest realm=\""  AUTH_REALM  "\", nonce=\"%s\"\r\n"

struct AirplayDataObject
{
    ILibChain_PreSelect     pre_select;
    ILibChain_PostSelect    post_select;
    ILibChain_Destroy       destroy;

    void *                  txt_map;
    void *                  session_map;
    void *                  http_server;
    void *                  dnssd_module;
    unsigned short          server_port;
    char *                  friendly_name;
    char *                  mac_addr;
    char *                  password;
    char *                  auth_nonce; // 认证

    void *                  user_tag;
};

AirplayHandlerGetCurrentTransportActions             AirplayCallbackGetCurrentTransportActions;
AirplayHandlerGetDeviceCapabilities                  AirplayCallbackGetDeviceCapabilities;
AirplayHandlerGetMediaInfo                           AirplayCallbackGetMediaInfo;
AirplayHandlerGetPositionInfo                        AirplayCallbackGetPositionInfo;
AirplayHandlerGetTransportInfo                       AirplayCallbackGetTransportInfo;
AirplayHandlerGetTransportSettings                   AirplayCallbackGetTransportSettings;
AirplayHandlerNext                                   AirplayCallbackNext;
AirplayHandlerPause                                  AirplayCallbackPause;
AirplayHandlerPlay                                   AirplayCallbackPlay;
AirplayHandlerPrevious                               AirplayCallbackPrevious;
AirplayHandlerSeek                                   AirplayCallbackSeek;
AirplayHandlerSetAVTransportURI                      AirplayCallbackSetAVTransportURI;
AirplayHandlerSetPlayMode                            AirplayCallbackSetPlayMode;
AirplayHandlerStop                                   AirplayCallbackStop;
AirplayHandlerGetCurrentConnectionIDs                AirplayCallbackGetCurrentConnectionIDs;
AirplayHandlerGetCurrentConnectionInfo               AirplayCallbackGetCurrentConnectionInfo;
AirplayHandlerGetProtocolInfo                        AirplayCallbackGetProtocolInfo;
AirplayHandlerGetBrightness                          AirplayCallbackGetBrightness;
AirplayHandlerGetContrast                            AirplayCallbackGetContrast;
AirplayHandlerGetMute                                AirplayCallbackGetMute;
AirplayHandlerGetVolume                              AirplayCallbackGetVolume;
AirplayHandlerListPresets                            AirplayCallbackListPresets;
AirplayHandlerSelectPreset                           AirplayCallbackSelectPreset;
AirplayHandlerSetBrightness                          AirplayCallbackSetBrightness;
AirplayHandlerSetContrast                            AirplayCallbackSetContrast;
AirplayHandlerSetMute                                AirplayCallbackSetMute;
AirplayHandlerSetVolume                              AirplayCallbackSetVolume;

void AirplaySetDisconnectFlag(AirplaySessionToken token,void * flag)
{
    ((struct ILibWebServer_Session *)token)->Reserved10 = flag;
}

void AirplayResponse_Error(const AirplaySessionToken session_token, const int error_code, const char * error_msg)
{
    char * head;
    int head_length;
    time_t ltime;
    char * date;

    ltime = time(NULL);
    date = asctime(gmtime(&ltime));
    date[strlen(date) - 1] = '\0';

    head = (char*)malloc(59);
    head_length = sprintf(head, "HTTP/1.1 %d %s\r\nDate: %s\r\n\r\n", error_code, error_msg, date);
    ILibWebServer_Send_Raw((struct ILibWebServer_Session *)session_token, head, head_length, 0, 1);
}

void AirplayResponse_Generic(const AirplaySessionToken session_token)
{
    AirplayResponse_Error(session_token, 200, "OK");
}

void AirplayResponse_GetCurrentTransportActions(const AirplaySessionToken session_token, const char * actions)
{

}

void AirplayResponse_GetDeviceCapabilities(const AirplaySessionToken session_token, const char * play_media, const char * rec_media, const char * rec_quality_modes)
{

}

void AirplayResponse_GetMediaInfo(const AirplaySessionToken session_token, const unsigned int nr_tracks, const char * media_duration, const char * current_uri, const char * current_uri_metadata, const char * next_uri, const char * next_uri_metadata, const char * play_medium, const char * record_medium, const char * write_status)
{

}

void AirplayResponse_GetPositionInfo(const AirplaySessionToken session_token, int duration, int current_pos)
{
    time_t ltime;
    char * date = NULL;
    char * body = NULL;
    int len = 0, body_len = 0;
    char * packet = NULL;
    ltime = time(NULL);
    date = asctime(gmtime(&ltime));
    date[strlen(date) - 1] = '\0';
    packet = (char *)malloc(512);
    body = (char *)malloc(128);
    body_len = sprintf(body, "duration: %d\r\nposition: %d\r\n", duration, current_pos);
    len = sprintf(packet, "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: text/parameters\r\nContent-Length: %d\r\n\r\n%s", date, body_len, body);
    free(body);
    ILibWebServer_Send_Raw((struct ILibWebServer_Session *)session_token, packet, len, 0, 1);
}

void AirplayResponse_GetTransportInfo(const AirplaySessionToken session_token, const char * current_transport_state, const char * current_transport_status, const char * current_speed)
{

}

void AirplayResponse_GetTransportSettings(const AirplaySessionToken session_token, const char * play_mode, const char * rec_quality_mode)
{

}

void AirplayResponse_Next(const AirplaySessionToken session_token)
{

}

void AirplayResponse_Pause(const AirplaySessionToken session_token)
{
    AirplayResponse_Generic(session_token);
}

void AirplayResponse_Play(const AirplaySessionToken session_token)
{
    AirplayResponse_Generic(session_token);
}

void AirplayResponse_Previous(const AirplaySessionToken session_token)
{

}

void AirplayResponse_Seek(const AirplaySessionToken session_token)
{
    AirplayResponse_Generic(session_token);
}

void AirplayResponse_SetAVTransportURI(const AirplaySessionToken session_token)
{

}

void AirplayResponse_SetPlayMode(const AirplaySessionToken session_token)
{

}

void AirplayResponse_Stop(const AirplaySessionToken session_token)
{
    AirplayResponse_Generic(session_token);
}

void AirplayResponse_GetCurrentConnectionIDs(const AirplaySessionToken session_token, const char * connection_ids)
{

}

void AirplayResponse_GetCurrentConnectionInfo(const AirplaySessionToken session_token, const int rcs_id, const int AVTransportID, const char * protocol_info, const char * peer_connection_manager, const int peer_connection_id, const char * direction, const char * status)
{

}

void AirplayResponse_GetProtocolInfo(const AirplaySessionToken session_token, const char * source, const char * sink)
{

}

void AirplayResponse_GetBrightness(const AirplaySessionToken session_token, const unsigned short current_brightness)
{

}

void AirplayResponse_GetContrast(const AirplaySessionToken session_token, const unsigned short current_contrast)
{

}

void AirplayResponse_GetMute(const AirplaySessionToken session_token, const int current_mute)
{

}

void AirplayResponse_GetVolume(const AirplaySessionToken session_token, const unsigned short current_volume)
{

}

void AirplayResponse_ListPresets(const AirplaySessionToken session_token, const char * current_preset_name_list)
{

}

void AirplayResponse_SelectPreset(const AirplaySessionToken session_token)
{

}

void AirplayResponse_SetBrightness(const AirplaySessionToken session_token)
{

}

void AirplayResponse_SetContrast(const AirplaySessionToken session_token)
{

}

void AirplayResponse_SetMute(const AirplaySessionToken session_token)
{

}

void AirplayResponse_SetVolume(const AirplaySessionToken session_token)
{

}

char * AirplayCalcResponse(const char * username,
                           const char * password,
                           const char * realm,
                           const char * method,
                           const char * digestUri,
                           const char * nonce)
{
    //char * resp;
    //char * ha1;
    //char * ha2;
    //char buf[128] = {0};
    //char * bytes = NULL;

    //MD5_CTX md5ctx;

    //MD5Init(&md5ctx, 0);
    //sprintf(buf, "%s:%s:%s", username, realm, password);
    //MD5Update(&md5ctx, buf, strlen(buf));
    //MD5Final(&md5ctx);
    //bytes = (char *)&(md5ctx.digest);

    //ha1 = Md5::GetMD5(username + ":" + realm + ":" + password);
    //ha2 = Md5::GetMD5(method + ":" + digestUri);

    //resp = Md5::GetMD5(str_to_lower(ha1) + ":" + nonce + ":" + str_to_lower(ha2));
    //return str_to_lower(resp);

    return NULL;
}

char * AirplayGetFildFromString(const char * auth_str, const char * field)
{
    char * retstr = NULL;
    int field_index = ILibString_IndexOf(auth_str, strlen(auth_str), field, strlen(field));
    if (field_index != -1) {
        int equal_index = ILibString_IndexOf(auth_str + field_index, strlen(auth_str) - field_index, "=", 1);
    }
    return retstr;
}

int AirplayCheckAuthorization(void * object, char * auth_str, char * method, char * uri)
{
    struct AirplayDataObject *s = (struct AirplayDataObject *)object;
    int auth_valid              = 1;
    char * username             = NULL;

    if (auth_str == NULL) {
        return 0;
    }

    username = AirplayGetFildFromString(auth_str, "username");

    if (username == NULL) {
        auth_valid = 0;
    }

    if (auth_valid)
        if (strcmp(AirplayGetFildFromString(auth_str, "realm"), AUTH_REALM) != 0)
            auth_valid = 0;

    if (auth_valid)
        if (strcmp(AirplayGetFildFromString(auth_str, "nonce"), s->auth_nonce) != 0)
            auth_valid = 0;

    if (auth_valid)
        if (strcmp(AirplayGetFildFromString(auth_str, "uri"), uri) != 0)
            auth_valid = 0;

    if (auth_valid) {
        char *  realm = AUTH_REALM;
        char * our_resp = AirplayCalcResponse(username, s->password, realm, method, uri, s->auth_nonce);
        char * their_resp = AirplayGetFildFromString(auth_str, "response");

        if (strcmp(ILibString_ToLower(our_resp, strlen(our_resp)), ILibString_ToLower(their_resp, strlen(their_resp))) != 0) {// 需要大小写不敏感比较
            auth_valid = 0;
            printf("AirAuth: response mismatch - our: %s theirs: %s\n", our_resp, their_resp);
        } else {
            printf("AirAuth: successfull authentication from AirPlay client\n");
        }
    }

    return auth_valid;
}

// Code Backup [9/20/2012 rainleafchen]
//static const  char * status_msg = "OK";
//
//void AirplayProcessHTTPPacket(struct ILibWebServer_Session * session, struct packetheader * header, char * bodyBuffer, int offset, int bodyBufferLength)
//{
//    int status;
//    int need_auth;
//    int start_qs;
//    int content_lenth;
//    char * method;
//    char * uri;
//    char * content_type;
//    char * authorization;
//    char * session_id;
//    char body[512] = {0};
//    time_t ltime;
//    char * date;
//    struct packetheader * resp_header;
//    struct AirplayDataObject * data_obj;
//
//    status          = AIRPLAY_STATUS_OK;
//    need_auth       = 0;
//    method          = header->Directive;
//    uri             = header->DirectiveObj;
//    content_lenth   = atoi(ILibGetHeaderLine(header, "content-length", 14) ? ILibGetHeaderLine(header, "content-length", 14) : "0");
//    content_type    = ILibGetHeaderLine(header, "content-type", 12);
//    authorization   = ILibGetHeaderLine(header, "authorization", 13);
//    session_id      = ILibGetHeaderLine(header, "x-apple-session-id", 18);
//    data_obj        = (struct AirplayDataObject *)session->User;
//    start_qs        = ILibString_IndexOf(header->DirectiveObj, header->DirectiveObjLength, "?", 1);
//
//    if (start_qs == -1) {
//        start_qs = header->DirectiveObjLength;
//    }
//    if (data_obj != NULL && data_obj->password != NULL) {
//        need_auth = 1;
//    }
//
//    resp_header = ILibCreateEmptyPacket();
//    ILibSetVersion(resp_header, "1.1", 3);
//
//    if (start_qs == 8 && memcmp(header->DirectiveObj, "/reverse", 8) == 0) {
//        printf("AIRPLAY Render: got request %s\n", header->DirectiveObj);
//        status = AIRPLAY_STATUS_SWITCHING_PROTOCOLS;
//        ILibAddHeaderLine(resp_header, "Upgrade", 7, "PTTH/1.0", 8);
//        ILibAddHeaderLine(resp_header, "Connection", 10, "Upgrade", 7);
//    } else if (start_qs == 5 && memcmp(header->DirectiveObj, "/rate", 5) == 0) {
//        char * found = strstr(header->DirectiveObj, "value=");
//        int rate = found ? (int)(atof(found + strlen("value=")) + 0.5f) : 0;
//
//        printf("AIRPLAY Render: got request %s with rate %i\n", header->DirectiveObj, rate);
//
//        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
//            status = AIRPLAY_STATUS_NEED_AUTH;
//        } else if (rate == 0) { // 暂停命令
//            if (AirplayCallbackPause == NULL) {
//                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//            } else {
//                AirplayCallbackPause(session, 0);
//            }
//        } else { // 播放命令
//            if (AirplayCallbackPlay == NULL) {
//                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//            } else {
//                AirplayCallbackPlay(session, 0, "1");
//            }
//        }
//    } else if (start_qs == 7 && memcmp(header->DirectiveObj, "/volume", 7) == 0) {
//        const char* found = strstr(header->DirectiveObj, "volume=");
//        double volume = found ? (double)(atof(found + strlen("volume="))) : 0;
//
//        printf("AIRPLAY Render: got request %s with volume %f\n", header->DirectiveObj, volume);
//
//        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
//            status = AIRPLAY_STATUS_NEED_AUTH;
//        } else if (volume >= 0 && volume <= 1) {
//            volume *= 100;
//            volume = volume < 0 ? 0 : (volume > 100 ? 100 : volume);
//            if (volume == 0) { // 静音
//                if (AirplayCallbackSetMute == NULL) {
//                    status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//                } else {
//                    AirplayCallbackSetMute(session, 0, "Master", 1);
//                }
//            } else { // 设置音量
//                if (AirplayCallbackSetVolume == NULL || AirplayCallbackSetMute == NULL) {
//                    status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//                } else {
//                    AirplayCallbackSetMute(session, 0, "Master", 0);
//                    AirplayCallbackSetVolume(session, 0, "Master", (unsigned short)volume);
//                }
//            }
//        }
//    } else if (start_qs == 5 && memcmp(header->DirectiveObj, "/play", 5) == 0) {
//        char * location = NULL;
//        int position = 0;
//        int last_event  = EVENT_NONE;
//
//        printf("AIRPLAY Render: got request %s\n", header->DirectiveObj);
//
//        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
//            status = AIRPLAY_STATUS_NEED_AUTH;
//        } else if (content_type != NULL && memcmp(content_type, "application/x-apple-binary-plist", 32) == 0) {
//            // process plist, iphone request
//        } else {
//            // iTuns request
//            int location_pos, start_pos;
//            location_pos = ILibString_IndexOf(bodyBuffer, bodyBufferLength, "Content-Location", 16);
//            if (location_pos != -1) {
//                location_pos += strlen("Content-Location: ");
//                location = bodyBuffer + location_pos;
//                start_pos = ILibString_IndexOf(bodyBuffer, bodyBufferLength, "Start-Position", 14);
//                bodyBuffer[start_pos - 1] = '\0';
//
//                printf("AIRPLAY Render: play uri = %s\n", location);
//
//                if (start_pos != -1) {
//                    start_pos += strlen("Start-Position: ");
//                    bodyBuffer[bodyBufferLength - 1] = '\0';
//                    position = (int)((float)atof(bodyBuffer + start_pos) * 1000.0);
//
//                    printf("AIRPLAY Render: play position = %d\n", position);
//                } else {
//                    // 获取不到position
//                }
//            } else {
//                // 获取不到URL
//                status = AIRPLAY_STATUS_NOT_FOUND;
//            }
//        }
//        if (status != AIRPLAY_STATUS_NEED_AUTH) {
//            if (AirplayCallbackSetAVTransportURI == NULL) {
//                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//            } else {
//                AirplayCallbackSetAVTransportURI(session, 0, location, "");
//            }
//            if (AirplayCallbackPlay == NULL) {
//                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//            } else {
//                AirplayCallbackPlay(session, 0, "1");
//            }
//            if (AirplayCallbackSeek == NULL) {
//                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//            } else {
//                char posbuf[128] = {0};
//                sprintf(posbuf, "%d", position);
//                AirplayCallbackSeek(session, 0, "ABS_TIME", posbuf);
//            }
//        }
//    } else if (start_qs == 6 && memcmp(header->DirectiveObj, "/scrub", 6) == 0) {
//        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
//            status = AIRPLAY_STATUS_NEED_AUTH;
//        } else if (memcmp(method, "GET", 3) == 0) { // 获取播放位置
//            printf("AIRPLAY Render: got GET request %s\n", header->DirectiveObj);
//            if (AirplayCallbackGetPositionInfo == NULL) {
//                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//            } else {
//                AirplayCallbackGetPositionInfo(session, 0);
//            }
//        } else { // POST: Seek 请求
//            const char* found = strstr(header->DirectiveObj, "position=");
//            if (found) {
//                int position = (int) (atof(found + strlen("position=")) * 1000.0);
//                printf("AIRPLAY Render: got POST request %s with pos %d\n", header->DirectiveObj, position);
//                if (AirplayCallbackSeek == NULL) {
//                    status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//                } else {
//                    char posbuf[128] = {0};
//                    sprintf(posbuf, "%d", position);
//                    AirplayCallbackSeek(session, 0, "ABS_TIME", posbuf);
//                }
//            }
//        }
//    } else if (start_qs == 5 && memcmp(header->DirectiveObj, "/stop", 5) == 0) {
//        printf("AIRPLAY Render: got request %s\n", header->DirectiveObj);
//        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
//            status = AIRPLAY_STATUS_NEED_AUTH;
//        } else {
//            if (AirplayCallbackStop == NULL) {
//                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//            } else {
//                AirplayCallbackStop(session, 0);
//            }
//        }
//    } else if (start_qs == 6 && memcmp(header->DirectiveObj, "/photo", 6) == 0) {
//        printf("AIRPLAY Render: got request %s\n", header->DirectiveObj);
//        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
//            status = AIRPLAY_STATUS_NEED_AUTH;
//        } else if (content_lenth > 0) {
//            // 将图片写入缓存，然后输出显示
//            // Body 部分为图片数据
//        }
//    } else if (start_qs == 14 && memcmp(header->DirectiveObj, "/playback-info", 14) == 0) {
//        float position      = 0.0f;
//        float duration      = 0.0f;
//        float cacheDuration = 0.0f;
//        int playing         = 0;
//
//        printf("AIRPLAY Render: got request %s\n", header->DirectiveObj);
//
//        // 获取当前播放状态
//        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
//            status = AIRPLAY_STATUS_NEED_AUTH;
//            //} else if (g_application.m_pPlayer) {
//            //    if (g_application.m_pPlayer->GetTotalTime()) {
//            //        position = ((float) g_application.m_pPlayer->GetTime()) / 1000;
//            //        duration = (float) g_application.m_pPlayer->GetTotalTime();
//            //        playing = g_application.m_pPlayer ? !g_application.m_pPlayer->IsPaused() : false;
//            //        cacheDuration = (float) g_application.m_pPlayer->GetTotalTime() * g_application.GetCachePercentage()/100.0f;
//            //    }
//
//            //    resp_body.Format(PLAYBACK_INFO, duration, cacheDuration, position, (playing ? 1 : 0), duration);
//            //    resp_header = "Content-Type: text/x-apple-plist+xml\r\n";
//
//            //    if (g_application.m_pPlayer->IsCaching()) {
//            //        _compose_reverse_event(reverse_header, reverse_body, session_id, EVENT_LOADING);
//            //    } else if (playing) {
//            //        _compose_reverse_event(reverse_header, reverse_body, session_id, EVENT_PLAYING);
//            //    } else {
//            //        _compose_reverse_event(reverse_header, reverse_body, session_id, EVENT_PAUSED);
//            //    }
//        } else {
//            sprintf(body, PLAYBACK_INFO_NOT_READY, duration, cacheDuration, position, (playing ? 1 : 0), duration);
//            ILibAddHeaderLine(resp_header, "Content-Type", 12, "text/x-apple-plist+xml", 22);
//        }
//    } else if (start_qs == 12 && memcmp(header->DirectiveObj, "/server-info", 12) == 0) {
//        printf("AIRPLAY Render: got request %s\n", header->DirectiveObj);
//        sprintf(body, SERVER_INFO, data_obj->mac_addr);
//        ILibAddHeaderLine(resp_header, "Content-Type", 12, "text/x-apple-plist+xml", 22);
//    } else if (start_qs == 19 && memcmp(header->DirectiveObj, "/slideshow-features", 19) == 0) {
//    } else if (start_qs == 10 && memcmp(header->DirectiveObj, "/authorize", 10) == 0) {
//    } else if (start_qs == 11 && memcmp(header->DirectiveObj, "/setProperty", 11) == 0) {
//        status = AIRPLAY_STATUS_NOT_FOUND;
//    } else if (start_qs == 11 && memcmp(header->DirectiveObj, "/getProperty", 11) == 0) {
//        status = AIRPLAY_STATUS_NOT_FOUND;
//    } else if (start_qs == 3 && memcmp(header->DirectiveObj, "200", 3) == 0) {
//        status = AIRPLAY_STATUS_NO_RESPONSE_NEEDED;
//    } else {
//        printf("AIRPLAY Render: unhandled request [%s]\n", header->StatusCode);
//        status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//    }
//
//    if (status == AIRPLAY_STATUS_NEED_AUTH) {
//        //_compose_auth_request_answer(resp_header, resp_body);
//    }
//
//    status_msg = "OK";
//
//    switch (status) {
//case AIRPLAY_STATUS_NOT_IMPLEMENTED:
//    status_msg = "Not Implemented";
//    break;
//case AIRPLAY_STATUS_SWITCHING_PROTOCOLS:
//    status_msg = "Switching Protocols";
//    ILibAddEntry(data_obj->session_map, session_id, strlen(session_id), ( void * )session );
//    break;
//case AIRPLAY_STATUS_NEED_AUTH:
//    status_msg = "Unauthorized";
//    break;
//case AIRPLAY_STATUS_NOT_FOUND:
//    status_msg = "Not Found";
//    break;
//case AIRPLAY_STATUS_METHOD_NOT_ALLOWED:
//    status_msg = "Method Not Allowed";
//    break;
//    }
//
//    // 加入错误码，错误信息。
//    ILibSetStatusCode(resp_header, status, (char *)status_msg, strlen(status_msg));
//
//    ltime = time(NULL);
//    date = asctime(gmtime(&ltime));
//    date[strlen(date) - 1] = '\0';
//
//    // 加入头部日期
//    ILibAddHeaderLine(resp_header, "Date", 4, date, strlen(date));
//
//    //std::string resp;
//    //char buf[512] = {0};
//    //const time_t ltime = time(NULL);
//    //char * date = asctime(gmtime(&ltime));
//    //date[strlen(date) - 1] = '\0';
//    //sprintf(buf, "HTTP/1.1 %d %s\nDate: %s\r\n", status, status_msg.c_str(), date);
//    //resp = buf;
//
//    //if (resp_header.size() > 0) {
//    //    resp += resp_header;
//    //}
//    //if (resp_body.size() > 0) {
//    //    sprintf(buf, "%sContent-Length: %d\r\n", resp.c_str(), resp_body.size());
//    //    resp = buf;
//    //}
//    //resp += "\r\n";
//
//    //if (resp_body.size() > 0) {
//    //    resp + reverse_body;
//    //}
//
//    //if (status != AIRPLAY_STATUS_NO_RESPONSE_NEEDED) {
//    //    send(m_socket_, resp.c_str(), resp.size(), 0);
//    //}
//
//    //if (reverse_header.size() > 0 && reverse_sockets.find(session_id) != reverse_sockets.end())
//    //{
//    //    //search the reverse socket to this sessionid
//    //    sprintf(buf, "POST /event HTTP/1.1\r\n");
//    //    resp = buf;
//    //    reverse_socket = reverse_sockets[session_id]; //that is our reverse socket
//    //    resp += reverse_header;
//    //}
//    //resp += "\r\n";
//
//    //if (reverse_body.size() > 0) {
//    //    resp += reverse_body;
//    //}
//
//    //if (reverse_socket != INVALID_SOCKET) {
//    //    send(reverse_socket, resp.c_str(), resp.size(), 0);//send the event status on the eventSocket
//    //}
//    ILibWebServer_StreamHeader(session, resp_header);
//    ILibWebServer_StreamBody(session, body, body ? strlen(body) : 0, ILibAsyncSocket_MemoryOwnership_STATIC, 1);
//    //ILibDestructPacket(resp_header);
//}
// Code Backup End [9/20/2012 rainleafchen]

static const  char * status_msg = "OK";

void AirplayProcessHTTPPacket(struct ILibWebServer_Session * session, struct packetheader * header, char * bodyBuffer, int offset, int bodyBufferLength)
{
    int status;
    int need_auth;
    int start_qs;
    int content_lenth;
    char * method;
    char * uri;
    char * content_type;
    char * authorization;
    char * session_id;
    char body[512] = {0};
    time_t ltime;
    char * date;
    char * packet;
    struct AirplayDataObject * data_obj;

    status          = AIRPLAY_STATUS_OK;
    need_auth       = 0;
    method          = header->Directive;
    uri             = header->DirectiveObj;
    content_lenth   = atoi(ILibGetHeaderLine(header, "content-length", 14) ? ILibGetHeaderLine(header, "content-length", 14) : "0");
    content_type    = ILibGetHeaderLine(header, "content-type", 12);
    authorization   = ILibGetHeaderLine(header, "authorization", 13);
    session_id      = ILibGetHeaderLine(header, "x-apple-session-id", 18);
    data_obj        = (struct AirplayDataObject *)session->User;
    start_qs        = ILibString_IndexOf(header->DirectiveObj, header->DirectiveObjLength, "?", 1);


    {
        char * header_buf = NULL;
        int len = ILibGetRawPacket(header, &header_buf);
        header_buf[len] = '\0';
        printf("Request :\n%s", header_buf ? header_buf : "");
        if (bodyBuffer) bodyBuffer[bodyBufferLength] = '\0';
        printf("%s", bodyBuffer ? bodyBuffer: "");
        free(header_buf);
    }

    if (start_qs == -1) {
        start_qs = header->DirectiveObjLength;
    }
    if (data_obj != NULL && data_obj->password != NULL) {
        need_auth = 1;
    }

    ltime = time(NULL);
    date = asctime(gmtime(&ltime));
    date[strlen(date) - 1] = '\0';

    if (start_qs == 8 && memcmp(header->DirectiveObj, "/reverse", 8) == 0) {
        printf("AIRPLAY Render: got request %s\n", header->DirectiveObj);
        status = AIRPLAY_STATUS_SWITCHING_PROTOCOLS;
    } else if (start_qs == 5 && memcmp(header->DirectiveObj, "/rate", 5) == 0) {
        char * found = strstr(header->DirectiveObj, "value=");
        int rate = found ? (int)(atof(found + strlen("value=")) + 0.5f) : 0;

        printf("AIRPLAY Render: got request %s with rate %i\n", header->DirectiveObj, rate);

        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
            status = AIRPLAY_STATUS_NEED_AUTH;
        } else if (rate == 0) { // 暂停命令
            if (AirplayCallbackPause == NULL) {
                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
            } else {
                AirplayCallbackPause(session, 0);
            }
        } else { // 播放命令
            if (AirplayCallbackPlay == NULL) {
                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
            } else {
                AirplayCallbackPlay(session, 0, "1");
            }
        }
    } else if (start_qs == 7 && memcmp(header->DirectiveObj, "/volume", 7) == 0) {
        const char* found = strstr(header->DirectiveObj, "volume=");
        double volume = found ? (double)(atof(found + strlen("volume="))) : 0;

        printf("AIRPLAY Render: got request %s with volume %f\n", header->DirectiveObj, volume);

        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
            status = AIRPLAY_STATUS_NEED_AUTH;
        } else if (volume >= 0 && volume <= 1) {
            volume *= 100;
            volume = volume < 0 ? 0 : (volume > 100 ? 100 : volume);
            if (volume == 0) { // 静音
                if (AirplayCallbackSetMute == NULL) {
                    status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
                } else {
                    AirplayCallbackSetMute(session, 0, "Master", 1);
                }
            } else { // 设置音量
                if (AirplayCallbackSetVolume == NULL || AirplayCallbackSetMute == NULL) {
                    status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
                } else {
                    AirplayCallbackSetMute(session, 0, "Master", 0);
                    AirplayCallbackSetVolume(session, 0, "Master", (unsigned short)volume);
                }
            }
        }
    } else if (start_qs == 5 && memcmp(header->DirectiveObj, "/play", 5) == 0) {
        char * location = NULL;
        int position = 0;
        int last_event  = EVENT_NONE;

        printf("AIRPLAY Render: got request %s\n", header->DirectiveObj);

        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
            status = AIRPLAY_STATUS_NEED_AUTH;
        } else if (content_type != NULL && memcmp(content_type, "application/x-apple-binary-plist", 32) == 0) {
            // process plist, iphone request
//            plist_t dict = NULL;
//            plist_from_bin(bodyBuffer, bodyBufferLength, &dict);
//
//            if (plist_dict_get_size(dict)) {
//                plist_t tmpNode = plist_dict_get_item(dict, "Start-Position");
//                if (tmpNode) {
//                    double tmpDouble = 0;
//                    plist_get_real_val(tmpNode, &tmpDouble);
//                    position = (int)tmpDouble * 1000;
//                }
//
//                tmpNode = plist_dict_get_item(dict, "Content-Location");
//                if (tmpNode) {
//                    char *tmpStr = NULL;
//                    plist_get_string_val(tmpNode, &tmpStr);
//                    location = String_Create(tmpStr);
//#ifdef _WIN32
//                    plist_free_string_val(tmpStr);
//#else
//                    free(tmpStr);
//#endif
//                }
//
//                if (dict) {
//                    plist_free(dict);
//                }
//
//                if (AirplayCallbackSetAVTransportURI == NULL) {
//                    status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//                } else {
//                    AirplayCallbackSetAVTransportURI(session, 0, location, "");
//                }
//                if (AirplayCallbackPlay == NULL) {
//                    status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//                } else {
//                    AirplayCallbackPlay(session, 0, "1");
//                }
//                if (AirplayCallbackSeek == NULL) {
//                    status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
//                } else {
//                    char posbuf[128] = {0};
//                    sprintf(posbuf, "%d", position);
//                    AirplayCallbackSeek(session, 0, "ABS_TIME", posbuf);
//                }
//                freesafe(position);
//            } else {
//                perror("Error parsing plist");
//            }
        } else {
            // iTuns request
            int location_pos, start_pos;
            location_pos = ILibString_IndexOf(bodyBuffer, bodyBufferLength, "Content-Location", 16);
            if (location_pos != -1) {
                location_pos += strlen("Content-Location: ");
                location = bodyBuffer + location_pos;
                start_pos = ILibString_IndexOf(bodyBuffer, bodyBufferLength, "Start-Position", 14);
                bodyBuffer[start_pos - 1] = '\0';

                printf("AIRPLAY Render: play uri = %s\n", location);

                if (start_pos != -1) {
                    start_pos += strlen("Start-Position: ");
                    bodyBuffer[bodyBufferLength - 1] = '\0';
                    position = (int)((float)atof(bodyBuffer + start_pos) * 1000.0);

                    printf("AIRPLAY Render: play position = %d\n", position);
                } else {
                    // 获取不到position
                }
            } else {
                // 获取不到URL
                status = AIRPLAY_STATUS_NOT_FOUND;
            }
        }
        if (status != AIRPLAY_STATUS_NEED_AUTH) {
            if (AirplayCallbackSetAVTransportURI == NULL) {
                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
            } else {
                AirplayCallbackSetAVTransportURI(session, 0, location, "");
            }
            if (AirplayCallbackPlay == NULL) {
                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
            } else {
                AirplayCallbackPlay(session, 0, "1");
            }
            if (AirplayCallbackSeek == NULL) {
                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
            } else {
                char posbuf[128] = {0};
                sprintf(posbuf, "%d", position);
                AirplayCallbackSeek(session, 0, "ABS_TIME", posbuf);
            }
        }
    } else if (start_qs == 6 && memcmp(header->DirectiveObj, "/scrub", 6) == 0) {
        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
            status = AIRPLAY_STATUS_NEED_AUTH;
        } else if (memcmp(method, "GET", 3) == 0) { // 获取播放位置
            printf("AIRPLAY Render: got GET request %s\n", header->DirectiveObj);
            if (AirplayCallbackGetPositionInfo == NULL) {
                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
            } else {
                AirplayCallbackGetPositionInfo(session, 0);
            }
        } else { // POST: Seek 请求
            const char* found = strstr(header->DirectiveObj, "position=");
            if (found) {
                int position = (int) (atof(found + strlen("position=")) * 1000.0);
                printf("AIRPLAY Render: got POST request %s with pos %d\n", header->DirectiveObj, position);
                if (AirplayCallbackSeek == NULL) {
                    status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
                } else {
                    char posbuf[128] = {0};
                    sprintf(posbuf, "%d", position);
                    AirplayCallbackSeek(session, 0, "ABS_TIME", posbuf);
                }
            }
        }
    } else if (start_qs == 5 && memcmp(header->DirectiveObj, "/stop", 5) == 0) {
        printf("AIRPLAY Render: got request %s\n", header->DirectiveObj);
        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
            status = AIRPLAY_STATUS_NEED_AUTH;
        } else {
            if (AirplayCallbackStop == NULL) {
                status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
            } else {
                AirplayCallbackStop(session, 0);
            }
        }
    } else if (start_qs == 6 && memcmp(header->DirectiveObj, "/photo", 6) == 0) {
        printf("AIRPLAY Render: got request %s\n", header->DirectiveObj);
        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
            status = AIRPLAY_STATUS_NEED_AUTH;
        } else if (content_lenth > 0) {
            // 将图片写入缓存，然后输出显示
            // Body 部分为图片数据
        }
    } else if (start_qs == 14 && memcmp(header->DirectiveObj, "/playback-info", 14) == 0) {
        float position      = 0.0f;
        float duration      = 0.0f;
        float cacheDuration = 0.0f;
        int playing         = 0;

        printf("AIRPLAY Render: got request %s\n", header->DirectiveObj);

        // 获取当前播放状态
        if (data_obj->password != NULL && AirplayCheckAuthorization(data_obj, authorization, method, uri)) {
            status = AIRPLAY_STATUS_NEED_AUTH;
        //} else if () {
            //} else if (g_application.m_pPlayer) {
            //    if (g_application.m_pPlayer->GetTotalTime()) {
            //        position = ((float) g_application.m_pPlayer->GetTime()) / 1000;
            //        duration = (float) g_application.m_pPlayer->GetTotalTime();
            //        playing = g_application.m_pPlayer ? !g_application.m_pPlayer->IsPaused() : false;
            //        cacheDuration = (float) g_application.m_pPlayer->GetTotalTime() * g_application.GetCachePercentage()/100.0f;
            //    }

            //    resp_body.Format(PLAYBACK_INFO, duration, cacheDuration, position, (playing ? 1 : 0), duration);
            //    resp_header = "Content-Type: text/x-apple-plist+xml\r\n";

            //    if (g_application.m_pPlayer->IsCaching()) {
            //        _compose_reverse_event(reverse_header, reverse_body, session_id, EVENT_LOADING);
            //    } else if (playing) {
            //        _compose_reverse_event(reverse_header, reverse_body, session_id, EVENT_PLAYING);
            //    } else {
            //        _compose_reverse_event(reverse_header, reverse_body, session_id, EVENT_PAUSED);
            //    }
        } else {
            int lenlen = 0;
            int body_len = 0;
            packet = (char *)malloc(512);
            memset(packet, 0, 512);
            body_len =  sprintf(body, PLAYBACK_INFO_NOT_READY, duration, cacheDuration, position, (playing ? 1 : 0), duration);
            lenlen = sprintf(packet, "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: text/x-apple-plist+xml\r\nContent-Length: %d\r\n\r\n%s", date, body_len, body);
            ILibWebServer_Send_Raw(session, packet, lenlen, 0, 1);
            return;
        }
    } else if (start_qs == 12 && memcmp(header->DirectiveObj, "/server-info", 12) == 0) {
        
        int body_length = 0, len = 0;
        char length_buf[128] = {0};
        printf("AIRPLAY Render: got request %s\n", header->DirectiveObj);
        body_length = sprintf(body, SERVER_INFO, data_obj->mac_addr);

        packet = (char *)malloc(1024);
        memset(packet, 0, 1024);
        len = sprintf(packet, "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: text/x-apple-plist+xml\r\nContent-Length: %d\r\n\r\n%s", date, body_length, body);
        ILibWebServer_Send_Raw(session, packet, len, 0, 1);
        return;
    } else if (start_qs == 19 && memcmp(header->DirectiveObj, "/slideshow-features", 19) == 0) {
        // Ignore for now.
        return;
    } else if (start_qs == 10 && memcmp(header->DirectiveObj, "/authorize", 10) == 0) {
        // DRM, ignore for now.
        return;
    } else if (start_qs == 12 && memcmp(header->DirectiveObj, "/setProperty", 12) == 0) {
        status = AIRPLAY_STATUS_NOT_FOUND;
    } else if (start_qs == 12 && memcmp(header->DirectiveObj, "/getProperty", 12) == 0) {
        status = AIRPLAY_STATUS_NOT_FOUND;
    } else if (header->DirectiveObj == NULL && header->StatusCode == 200) {
        return;
    } else {
        printf("AIRPLAY Render: unhandled request [%d]\n", header->StatusCode);
        status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
    }

    if (status == AIRPLAY_STATUS_NEED_AUTH) {
        //_compose_auth_request_answer(resp_header, resp_body);
    }

    packet = (char *)malloc(512);
    memset(packet, 0, 512);

    switch (status) {
case AIRPLAY_STATUS_NOT_IMPLEMENTED:
    {
        int len = 0;
        len = sprintf(packet, "HTTP/1.1 %d Not Implemented\r\nDate: %s\r\n\r\n", AIRPLAY_STATUS_NOT_IMPLEMENTED, date);
        ILibWebServer_Send_Raw(session, packet, len, 0, 1);
        return;
    }
    break;
case AIRPLAY_STATUS_SWITCHING_PROTOCOLS:
    {
        int len = 0;
        ILibAddEntry(data_obj->session_map, session_id, strlen(session_id), ( void * )session );
        len = sprintf(packet, "HTTP/1.1 %d Switching Protocols\r\nDate: %s\r\nUpgrade: PTTH/1.0\r\nConnection: Upgrade\r\n\r\n", AIRPLAY_STATUS_SWITCHING_PROTOCOLS, date);
        ILibWebServer_Send_Raw(session, packet, len, 0, 1);
    }
    break;
case AIRPLAY_STATUS_NEED_AUTH:
    {
        int len = 0;
        len = sprintf(packet, "HTTP/1.1 %d Unauthorized\r\nDate: %s\r\n\r\n", AIRPLAY_STATUS_NEED_AUTH, date);
        ILibWebServer_Send_Raw(session, packet, len, 0, 1);
    }
    break;
case AIRPLAY_STATUS_NOT_FOUND:
    {
        int len = 0;
        len = sprintf(packet, "HTTP/1.1 %d Not Found\r\nDate: %s\r\n\r\n", AIRPLAY_STATUS_NOT_FOUND, date);
        ILibWebServer_Send_Raw(session, packet, len, 0, 1);
    }
    break;
case AIRPLAY_STATUS_METHOD_NOT_ALLOWED:
    {
        int len = 0;
        len = sprintf(packet, "HTTP/1.1 %d Method Not Allowed\r\nDate: %s\r\n\r\n", AIRPLAY_STATUS_METHOD_NOT_ALLOWED, date);
        ILibWebServer_Send_Raw(session, packet, len, 0, 1);
    }
    break;
    }
}

void AirplaySessionReceiveSink(
struct ILibWebServer_Session * sender,
    int InterruptFlag,
struct packetheader * header,
    char * bodyBuffer,
    int * beginPointer,
    int endPointer,
    int done)
{
    char * txt;
    if (header != NULL && sender->User3 == NULL && done == 0) {
        sender->User3 = (void *)~0;
        txt = ILibGetHeaderLine(header, "Expect", 6);
        if (txt != NULL) {
            if (strcasecmp(txt, "100-Continue") == 0) {
                ILibWebServer_Send_Raw(sender, "HTTP/1.1 100 Continue\r\n\r\n", 25, ILibAsyncSocket_MemoryOwnership_STATIC, 0);
            } else {
                ILibWebServer_Send_Raw(sender, "HTTP/1.1 417 Expectation Failed\r\n\r\n", 35, ILibAsyncSocket_MemoryOwnership_STATIC, 1);
                ILibWebServer_DisconnectSession(sender);
                return;
            }
        }
    }

    if (header != NULL && done != 0 && InterruptFlag == 0) {
        AirplayProcessHTTPPacket(sender, header, bodyBuffer, beginPointer == NULL ? 0: *beginPointer, endPointer);
        if (beginPointer != NULL) { *beginPointer = endPointer; }
    }
}

void AirplaySessionSink(struct ILibWebServer_Session * SessionToken, void * user)
{
    SessionToken->OnReceive = &AirplaySessionReceiveSink;
    SessionToken->User      = user;
}

void AirplayStart(struct AirplayDataObject * object)
{
}

void AirplayPreSelect(void * object,void * readset, void * writeset, void * errorset, int * blocktime)
{
    struct AirplayDataObject *s = (struct AirplayDataObject *)object;
    AirplayStart(s);
    s->pre_select = NULL;
}

void AirplayDestroy(void * object)
{
    struct AirplayDataObject *s = (struct AirplayDataObject *)object;

    freesafe(s->friendly_name);
    freesafe(s->mac_addr);
    freesafe(s->password);
    freesafe(s->auth_nonce);
    // map 内部数据交给dnssd销毁
    ILibDestroyHashTree(s->txt_map);
    ILibDestroyHashTree(s->session_map);
}

void AirplayOnDnssdStart(int error_code, void * user)
{
    (void)user;
    printf("Start dns_sd ret = %d\n", error_code);
}

#define CreateString(x)        (char *)strcpy((char *)malloc(strlen(x) + 1), x)

AirplayToken AirplayCreate(void * chain,
                           const unsigned short port,
                           const char * friendly_name,
                           const char * mac_addr,
                           const char * password)
{
    char * key = NULL;
    char * val = NULL;
    struct AirplayDataObject * ret_val = NULL;

    if (ILibDaemonIsRunning() == 0) {
        return NULL; // Daemon没有启动
    }

    ret_val = (struct AirplayDataObject *)malloc(sizeof(struct AirplayDataObject));
    memset(ret_val, 0, sizeof(struct AirplayDataObject));
    ret_val->pre_select     = &AirplayPreSelect;
    ret_val->post_select    = NULL;
    ret_val->destroy        = &AirplayDestroy;
    ret_val->friendly_name  = CreateString(friendly_name);
    ret_val->mac_addr       = CreateString(mac_addr);
    ret_val->user_tag       = NULL;
    ret_val->auth_nonce     = NULL;
    if (password != NULL) {
        ret_val->password   = CreateString(password);
    }

    ret_val->txt_map        = ILibInitHashTree();
    ret_val->session_map    = ILibInitHashTree();

    val = CreateString(mac_addr);
    ILibAddEntry(ret_val->txt_map, "deviceid", strlen("deviceid"), ( void * )val );
    val = CreateString("0x77");
    ILibAddEntry(ret_val->txt_map, "features", strlen("features"), ( void * )val );
    val = CreateString("AppleTV2,1");
    ILibAddEntry(ret_val->txt_map, "model", strlen("model"), ( void * )val );
    val = CreateString(AIRPLAY_SERVER_VERSION_STR);
    ILibAddEntry(ret_val->txt_map, "srcvers", strlen("srcvers"), ( void * )val );

    ret_val->http_server    = ILibWebServer_Create(chain, 5, port, &AirplaySessionSink, ret_val);
    ret_val->dnssd_module   = ILibCreateDnssdModule(chain, "_airplay._tcp", ret_val->friendly_name, port, ret_val->txt_map, AirplayOnDnssdStart, ret_val);
    ILibAddToChain(chain, (void *)ret_val);

    return (void *)ret_val;
}

int AirplayGetLocalInterfaceToHost(const AirplaySessionToken session_token)
{
    return (ILibWebServer_GetLocalInterface((struct ILibWebServer_Session *)session_token));
}

void * AirplayGetWebServerToken(const AirplayToken airplay_token)
{
    return ((struct AirplayDataObject *)(airplay_token))->http_server;
}

void AirplaySetTag(const AirplayToken airplay_token, void * user_token)
{
    ((struct AirplayDataObject *)(airplay_token))->user_tag = user_token;
}

void * AirplayGetTag(const AirplayToken airplay_token)
{
    return ((struct AirplayDataObject *)(airplay_token))->user_tag;
}

AirplayToken AirplayTokenFromSessionToken(const AirplaySessionToken session_token)
{
    return((struct ILibWebServer_Session *)(session_token))->User;
}

void AirplaySetState_LastChange(AirplayToken airplay_token, int state)
{
    char * key                          = NULL;
    void * val                          = NULL;
    struct AirplayDataObject * object   = NULL;
    void * iter                         = NULL;

    switch (state) {
case 1: // stopped
case 2: // paused
case 3: // playing
case 4: // loading
    break;
default:
    // no media
    return;
    }

    object = (struct AirplayDataObject *)airplay_token;
    if (object == NULL) return;
    iter = ILibHashTree_GetEnumerator(object->session_map);
    if (!iter) return;
    while ( !ILibHashTree_MoveNext( iter ) ) {
        char snd_buf[1024]  = {0};
        int snd_len         = 0;
        char snd_body[512]  = {0};
        int len             = 0;
        ILibHashTree_GetValue( iter, &key, &len, ((void **)(&val)));
        len = sprintf(snd_body, EVENT_INFO, eventStrings[state - 1]);
        printf("AIRPLAY Render: sending event: %s\n", eventStrings[state - 1]);

        snd_len =sprintf(snd_buf, "POST /event HTTP/1.1\r\nContent-Type: text/x-apple-plist+xml\r\nContent-Length: %d\r\nx-apple-session-id: %s\r\n\r\n%s", len, key, snd_body);
        ILibWebServer_Send_Raw((struct ILibWebServer_Session *)val, snd_buf, snd_len, 1, 1);
    }
    ILibHashTree_DestroyEnumerator(iter);
}

void AirplaySetState_SourceProtocolInfo(AirplayToken airplay_token, char * val)
{

}

void AirplaySetState_SinkProtocolInfo(AirplayToken airplay_token, char * val)
{

}

void AirplaySetState_CurrentConnectionIDs(AirplayToken airplay_token, char * val)
{

}

#endif // ENABLED_AIRPLAY