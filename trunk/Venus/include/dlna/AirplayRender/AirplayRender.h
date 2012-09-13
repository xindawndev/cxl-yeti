#ifndef _AIRPLAY_RENDER_H_
#define _AIRPLAY_RENDER_H_

#include "ILibAsyncSocket.h"

typedef void * AirplayToken;
typedef void * AirplaySessionToken;

// password == NULL ��ʾ��ʹ������
AirplayToken AirplayCreate(void * chain,
                           const unsigned short port,
                           const char * friendly_name,
                           const char * mac_addr,
                           const char * password);

#endif