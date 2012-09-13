/************************************************************************
Airplay 协议：

/reverse
    协商请求
/scrub
    POST方式为seek请求
    GET方式为获取播放位置
/volume
    设置音量：0.000000为静音，1.000000为最大
/play
    播放请求：携带播放链接
    本地文件播放为http，网络文件播放为m3u8地址
/rate
    播放与暂停：0.000000为暂停，1.000000为播放
/stop
    停止播放
/photo
    推送图片：在HTTP的Body发送实际图片
/playback-info
    获取播放端的状态：总时长、缓冲时长、播放位置、播放器状态（LOADING、PLAYING、PAUSED、STOP）等信息
/server-info
    获取服务器信息：主要是mac地址信息
/************************************************************************/

#ifndef _AIRPLAY_RENDER_H_
#define _AIRPLAY_RENDER_H_

#include "ILibAsyncSocket.h"

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


#endif
