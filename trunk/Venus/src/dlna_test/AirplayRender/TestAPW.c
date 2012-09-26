#include "TestAPW.h"

#define LogFun() printf("[%s] [%s] <%d>\n", __FILE__, __FUNCTION__, __LINE__)

int APW_Callback_SetAVTransportURI (  APW instance, void * session, char * uri, char * metadata)
{
    LogFun();
    return 0;
}

int APW_Callback_GetAVProtocolInfo (  APW instance, void * session, char ** protocolInfo)
{
    LogFun();
    return 0;
}

int APW_Callback_SetPlayMode (        APW instance, void * session, APWMediaPlayMode play_mode)
{
    LogFun();
    return 0;
}

int APW_Callback_Stop (               APW instance, void * session)
{
    LogFun();
    APW_StateChange_TransportPlayState(instance, APW_PS_Stopped);
    return 0;
}

int APW_Callback_Play (               APW instance, void * session, char* playSpeed)
{
    LogFun();
    APW_StateChange_TransportPlayState(instance, APW_PS_Playing);
    return 0;
}

int APW_Callback_Pause (              APW instance, void * session)
{
    LogFun();
    APW_StateChange_TransportPlayState(instance, APW_PS_Paused);
    return 0;
}

int APW_Callback_SeekTrack (          APW instance, void * session, unsigned int trackIndex)
{
    LogFun();
    return 0;
}

int APW_Callback_SeekTrackPosition (  APW instance, void * session, long position)
{
    LogFun();
    return 0;
}

int APW_Callback_SeekMediaPosition (  APW instance, void * session, long position)
{
    LogFun();
    return 0;
}

int APW_Callback_Next (               APW instance, void * session)
{
    LogFun();
    return 0;
}

int APW_Callback_Previous (           APW instance, void * session)
{
    LogFun();
    return 0;
}

int APW_Callback_SelectPreset (       APW instance, void * session, char* presetName)
{
    LogFun();
    return 0;
}

int APW_Callback_SetVolume (          APW instance, void * session, unsigned char volume)
{
    LogFun();
    return 0;
}

int APW_Callback_SetMute (            APW instance, void * session, BOOL mute)
{
    LogFun();
    return 0;
}

int APW_Callback_SetContrast (        APW instance, void * session, unsigned char contrast)
{
    LogFun();
    return 0;
}

int APW_Callback_SetBrightness (      APW instance, void * session, unsigned char brightness)
{
    LogFun();
    return 0;
}
