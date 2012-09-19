#ifndef _TEST_APW_H_
#define _TEST_APW_H_

#include "AirplayWrapper.h"

int APW_Callback_SetAVTransportURI (  APW instance, void * session, char * uri, char * metadata);
int APW_Callback_GetAVProtocolInfo (  APW instance, void * session, char ** protocolInfo);
int APW_Callback_SetPlayMode (        APW instance, void * session, APWMediaPlayMode play_mode);
int APW_Callback_Stop (               APW instance, void * session);
int APW_Callback_Play (               APW instance, void * session, char* playSpeed);
int APW_Callback_Pause (              APW instance, void * session);
int APW_Callback_SeekTrack (          APW instance, void * session, unsigned int trackIndex);
int APW_Callback_SeekTrackPosition (  APW instance, void * session, long position);
int APW_Callback_SeekMediaPosition (  APW instance, void * session, long position);
int APW_Callback_Next (               APW instance, void * session);
int APW_Callback_Previous (           APW instance, void * session);
int APW_Callback_SelectPreset (       APW instance, void * session, char* presetName);
int APW_Callback_SetVolume (          APW instance, void * session, unsigned char volume);
int APW_Callback_SetMute (            APW instance, void * session, BOOL mute);
int APW_Callback_SetContrast (        APW instance, void * session, unsigned char contrast);
int APW_Callback_SetBrightness (      APW instance, void * session, unsigned char brightness);

#endif
