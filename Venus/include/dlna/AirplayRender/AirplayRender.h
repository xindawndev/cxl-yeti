/************************************************************************
Airplay 协议：

/reverse
    * 协商请求
    POST /reverse HTTP/1.1
    User-Agent: iTunes/10.6.3 (Windows; Microsoft Windows 7 Ultimate Edition (Build 7600)) AppleWebKit/534.57.2
    Content-Length: 0
    Upgrade: PTTH/1.0
    Connection: Upgrade    HTTP/1.1 101 Switching Protocols
    Date: Fri Sep 14 03:30:20 2012
    Upgrade: PTTH/1.0
    Connection: Upgrade
/scrub
    * POST方式为seek请求：
    POST /scrub?position=34.263000 HTTP/1.1
    User-Agent: iTunes/10.6.3 (Windows; Microsoft Windows 7 Ultimate Edition (Build 7600)) AppleWebKit/534.57.2
    Content-Length: 0
    HTTP/1.1 200 OK
    Date: Fri Sep 14 03:30:35 2012

    * GET方式为获取播放位置：
    GET /scrub HTTP/1.1
    User-Agent: iTunes/10.6.3 (Windows; Microsoft Windows 7 Ultimate Edition (Build 7600)) AppleWebKit/534.57.2
    Content-Length: 0

    HTTP/1.1 200 OK
    Date: Fri Sep 14 03:30:26 2012
    Content-Length: 33

    duration: 229
    position: 7.113000

/volume
    设置音量：0.000000为静音，1.000000为最大
/play
    播放请求：携带播放链接
    本地文件播放为http，网络文件播放为m3u8地址
    POST /play HTTP/1.1    User-Agent: iTunes/10.6.3 (Windows; Microsoft Windows 7 Ultimate Edition (Build 7600)) AppleWebKit/534.57.2    Content-Length: 164    Content-Type: text/parameters
    Content-Location: http://192.168.99.104:3689/airplay.mp4?database-spec='dmap.persistentid:0xba8edec512c709f7'&item-spec='dmap.itemid:0x4e'
    Start-Position: 0.013460

    POST /event HTTP/1.1    Content-Type: text/x-apple-plist+xml    Content-Length: 285x-apple-session-id: 
    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
    <plist version="1.0">    <dict>    <key>category</key>    <string>video</string>    <key>state</key>    <string>playing</string>    </dict>    </plist>
/rate
    播放与暂停：0.000000为暂停，1.000000为播放
    POST /rate?value=0.000000 HTTP/1.1    User-Agent: iTunes/10.6.3 (Windows; Microsoft Windows 7 Ultimate Edition (Build 7600)) AppleWebKit/534.57.2    Content-Length: 0
    POST /event HTTP/1.1    Content-Type: text/x-apple-plist+xml    Content-Length: 284x-apple-session-id: 
    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
    <plist version="1.0">    <dict>    <key>category</key>    <string>video</string>    <key>state</key>    <string>paused</string>    </dict>    </plist>

/stop
    停止播放
    POST /stop HTTP/1.1    User-Agent: iTunes/10.6.3 (Windows; Microsoft Windows 7 Ultimate Edition (Build 7600)) AppleWebKit/534.57.2    Content-Length: 0
    POST /event HTTP/1.1    Content-Type: text/x-apple-plist+xml    Content-Length: 285x-apple-session-id: 
    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
    <plist version="1.0">    <dict>    <key>category</key>    <string>video</string>    <key>state</key>    <string>stopped</string>    </dict>    </plist>
/photo
    推送图片：在HTTP的Body发送实际图片
/playback-info
    获取播放端的状态：总时长、缓冲时长、播放位置、播放器状态（LOADING、PLAYING、PAUSED、STOP）等信息
/server-info
    获取服务器信息：主要是mac地址信息

mDNSResponser 启动方法：-r -p 7000 -n FriendlyName -t _airplay._tcp. -d local. -x deviceid=38:E7:D8:AC:4E:EB features=0x77 model=AppleTV2,1 srcvers=101.28
/************************************************************************/

#ifndef _AIRPLAY_RENDER_H_
#define _AIRPLAY_RENDER_H_

#include "ILibAsyncSocket.h"

#define AIRPLAY_SERVER_VERSION_STR "101.28"

typedef void * AirplayToken;
typedef void * AirplaySessionToken;

// password == NULL 表示不使用密码
AirplayToken    AirplayCreate(void * chain,
                           const unsigned short port,
                           const char * friendly_name,
                           const char * mac_addr,
                           const char * password);
int             AirplayGetLocalInterfaceToHost(const AirplaySessionToken session_token);
void *          AirplayGetWebServerToken(const AirplayToken airplay_token);
void            AirplaySetTag(const AirplayToken airplay_token, void * user_token);
void *          AirplayGetTag(const AirplayToken airplay_token);
AirplayToken    AirplayTokenFromSessionToken(const AirplaySessionToken session_token);

// 回调函数
typedef void(* AirplayHandlerGetCurrentTransportActions) (  void * session_token, unsigned int instance_id);
typedef void(* AirplayHandlerGetDeviceCapabilities) (       void * session_token, unsigned int instance_id);
typedef void(* AirplayHandlerGetMediaInfo) (                void * session_token, unsigned int instance_id);
typedef void(* AirplayHandlerGetPositionInfo) (             void * session_token, unsigned int instance_id);
typedef void(* AirplayHandlerGetTransportInfo) (            void * session_token, unsigned int instance_id);
typedef void(* AirplayHandlerGetTransportSettings) (        void * session_token, unsigned int instance_id);
typedef void(* AirplayHandlerNext) (                        void * session_token, unsigned int instance_id);
typedef void(* AirplayHandlerPause) (                       void * session_token, unsigned int instance_id);
typedef void(* AirplayHandlerPlay) (                        void * session_token, unsigned int instance_id, char * speed);
typedef void(* AirplayHandlerPrevious) (                    void * session_token, unsigned int instance_id);
typedef void(* AirplayHandlerSeek) (                        void * session_token, unsigned int instance_id, char * unit, char * target);
typedef void(* AirplayHandlerSetAVTransportURI) (           void * session_token, unsigned int instance_id, char * current_uri,char * current_uri_metadata);
typedef void(* AirplayHandlerSetPlayMode) (                 void * session_token, unsigned int instance_id, char * new_play_mode);
typedef void(* AirplayHandlerStop) (                        void * session_token, unsigned int instance_id);
typedef void(* AirplayHandlerGetCurrentConnectionIDs) (     void * session_token);
typedef void(* AirplayHandlerGetCurrentConnectionInfo) (    void * session_token, int connection_id);
typedef void(* AirplayHandlerGetProtocolInfo) (             void * session_token);
typedef void(* AirplayHandlerGetBrightness) (               void * session_token, unsigned int instance_id);
typedef void(* AirplayHandlerGetContrast) (                 void * session_token, unsigned int instance_id);
typedef void(* AirplayHandlerGetMute) (                     void * session_token, unsigned int instance_id, char * channel);
typedef void(* AirplayHandlerGetVolume) (                   void * session_token, unsigned int instance_id, char * channel);
typedef void(* AirplayHandlerListPresets) (                 void * session_token, unsigned int instance_id);
typedef void(* AirplayHandlerSelectPreset) (                void * session_token, unsigned int instance_id, char * preset_name);
typedef void(* AirplayHandlerSetBrightness) (               void * session_token, unsigned int instance_id, unsigned short desired_brightness);
typedef void(* AirplayHandlerSetContrast) (                 void * session_token, unsigned int instance_id, unsigned short desired_contrast);
typedef void(* AirplayHandlerSetMute) (                     void * session_token, unsigned int instance_id, char * channel, int desire_mute);
typedef void(* AirplayHandlerSetVolume) (                   void * session_token, unsigned int instance_id, char * channel, unsigned short desired_volume);

extern AirplayHandlerGetCurrentTransportActions             AirplayCallbackGetCurrentTransportActions;
extern AirplayHandlerGetDeviceCapabilities                  AirplayCallbackGetDeviceCapabilities;
extern AirplayHandlerGetMediaInfo                           AirplayCallbackGetMediaInfo;
extern AirplayHandlerGetPositionInfo                        AirplayCallbackGetPositionInfo;
extern AirplayHandlerGetTransportInfo                       AirplayCallbackGetTransportInfo;
extern AirplayHandlerGetTransportSettings                   AirplayCallbackGetTransportSettings;
extern AirplayHandlerNext                                   AirplayCallbackNext;
extern AirplayHandlerPause                                  AirplayCallbackPause;
extern AirplayHandlerPlay                                   AirplayCallbackPlay;
extern AirplayHandlerPrevious                               AirplayCallbackPrevious;
extern AirplayHandlerSeek                                   AirplayCallbackSeek;
extern AirplayHandlerSetAVTransportURI                      AirplayCallbackSetAVTransportURI;
extern AirplayHandlerSetPlayMode                            AirplayCallbackSetPlayMode;
extern AirplayHandlerStop                                   AirplayCallbackStop;
extern AirplayHandlerGetCurrentConnectionIDs                AirplayCallbackGetCurrentConnectionIDs;
extern AirplayHandlerGetCurrentConnectionInfo               AirplayCallbackGetCurrentConnectionInfo;
extern AirplayHandlerGetProtocolInfo                        AirplayCallbackGetProtocolInfo;
extern AirplayHandlerGetBrightness                          AirplayCallbackGetBrightness;
extern AirplayHandlerGetContrast                            AirplayCallbackGetContrast;
extern AirplayHandlerGetMute                                AirplayCallbackGetMute;
extern AirplayHandlerGetVolume                              AirplayCallbackGetVolume;
extern AirplayHandlerListPresets                            AirplayCallbackListPresets;
extern AirplayHandlerSelectPreset                           AirplayCallbackSelectPreset;
extern AirplayHandlerSetBrightness                          AirplayCallbackSetBrightness;
extern AirplayHandlerSetContrast                            AirplayCallbackSetContrast;
extern AirplayHandlerSetMute                                AirplayCallbackSetMute;
extern AirplayHandlerSetVolume                              AirplayCallbackSetVolume;

void AirplaySetDisconnectFlag(                              AirplaySessionToken token,void * flag);

// Invocation Response Methods
void AirplayResponse_Error(                                 const AirplaySessionToken session_token, const int error_code, const char * error_msg);
void AirplayResponse_Generic(                               const AirplaySessionToken session_token);
void AirplayResponse_GetCurrentTransportActions(            const AirplaySessionToken session_token, const char * actions);
void AirplayResponse_GetDeviceCapabilities(                 const AirplaySessionToken session_token, const char * play_media, const char * rec_media, const char * rec_quality_modes);
void AirplayResponse_GetMediaInfo(                          const AirplaySessionToken session_token, const unsigned int nr_tracks, const char * media_duration, const char * current_uri, const char * current_uri_metadata, const char * next_uri, const char * next_uri_metadata, const char * play_medium, const char * record_medium, const char * write_status);
void AirplayResponse_GetPositionInfo(                       const AirplaySessionToken session_token, int duration, int current_pos);
void AirplayResponse_GetTransportInfo(                      const AirplaySessionToken session_token, const char * current_transport_state, const char * current_transport_status, const char * current_speed);
void AirplayResponse_GetTransportSettings(                  const AirplaySessionToken session_token, const char * play_mode, const char * rec_quality_mode);
void AirplayResponse_Next(                                  const AirplaySessionToken session_token);
void AirplayResponse_Pause(                                 const AirplaySessionToken session_token);
void AirplayResponse_Play(                                  const AirplaySessionToken session_token);
void AirplayResponse_Previous(                              const AirplaySessionToken session_token);
void AirplayResponse_Seek(                                  const AirplaySessionToken session_token);
void AirplayResponse_SetAVTransportURI(                     const AirplaySessionToken session_token);
void AirplayResponse_SetPlayMode(                           const AirplaySessionToken session_token);
void AirplayResponse_Stop(                                  const AirplaySessionToken session_token);
void AirplayResponse_GetCurrentConnectionIDs(               const AirplaySessionToken session_token, const char * connection_ids);
void AirplayResponse_GetCurrentConnectionInfo(              const AirplaySessionToken session_token, const int rcs_id, const int AVTransportID, const char * protocol_info, const char * peer_connection_manager, const int peer_connection_id, const char * direction, const char * status);
void AirplayResponse_GetProtocolInfo(                       const AirplaySessionToken session_token, const char * source, const char * sink);
void AirplayResponse_GetBrightness(                         const AirplaySessionToken session_token, const unsigned short current_brightness);
void AirplayResponse_GetContrast(                           const AirplaySessionToken session_token, const unsigned short current_contrast);
void AirplayResponse_GetMute(                               const AirplaySessionToken session_token, const int current_mute);
void AirplayResponse_GetVolume(                             const AirplaySessionToken session_token, const unsigned short current_volume);
void AirplayResponse_ListPresets(                           const AirplaySessionToken session_token, const char * current_preset_name_list);
void AirplayResponse_SelectPreset(                          const AirplaySessionToken session_token);
void AirplayResponse_SetBrightness(                         const AirplaySessionToken session_token);
void AirplayResponse_SetContrast(                           const AirplaySessionToken session_token);
void AirplayResponse_SetMute(                               const AirplaySessionToken session_token);
void AirplayResponse_SetVolume(                             const AirplaySessionToken session_token);

/* State Variable Eventing Methods */
void AirplaySetState_LastChange(                            AirplayToken airplay_token, int state);
void AirplaySetState_SourceProtocolInfo(                    AirplayToken airplay_token, char * val);
void AirplaySetState_SinkProtocolInfo(                      AirplayToken airplay_token, char * val);
void AirplaySetState_CurrentConnectionIDs(                  AirplayToken airplay_token, char * val);

#endif
