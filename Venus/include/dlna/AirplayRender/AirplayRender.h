/************************************************************************
Airplay Э�飺

/reverse
    Э������
/scrub
    POST��ʽΪseek����
    GET��ʽΪ��ȡ����λ��
/volume
    ����������0.000000Ϊ������1.000000Ϊ���
/play
    ��������Я����������
    �����ļ�����Ϊhttp�������ļ�����Ϊm3u8��ַ
/rate
    ��������ͣ��0.000000Ϊ��ͣ��1.000000Ϊ����
/stop
    ֹͣ����
/photo
    ����ͼƬ����HTTP��Body����ʵ��ͼƬ
/playback-info
    ��ȡ���Ŷ˵�״̬����ʱ��������ʱ��������λ�á�������״̬��LOADING��PLAYING��PAUSED��STOP������Ϣ
/server-info
    ��ȡ��������Ϣ����Ҫ��mac��ַ��Ϣ
/************************************************************************/

#ifndef _AIRPLAY_RENDER_H_
#define _AIRPLAY_RENDER_H_

#include "ILibAsyncSocket.h"

typedef void * AirplayToken;
typedef void * AirplaySessionToken;

// password == NULL ��ʾ��ʹ������
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

// �ص�����


#endif
