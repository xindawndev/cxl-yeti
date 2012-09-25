/*
* Many concepts and protocol specification in this code are taken
* from Airplayer. https://github.com/PascalW/Airplayer
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "AirPlayServer.h"

#ifdef HAS_AIRPLAY

#include <netinet/in.h>
#include <arpa/inet.h>
//#include "DllLibPlist.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "threads/SingleLock.h"
#include "filesystem/File.h"
#include "Util.h"
#include "FileItem.h"
#include "Application.h"
//#include "utils/md5.h"
#include "utils/Variant.h"
#include "guilib/GUIWindowManager.h"

#ifdef TARGET_WINDOWS
#define close closesocket
#endif

CAirPlayServer *CAirPlayServer::ServerInstance = NULL;
int CAirPlayServer::m_isPlaying = false;

DWORD WINAPI ILibPoolThread(void *args)
{
    ILibThreadPool_AddThread(args);
    return(0);
}

bool CAirPlayServer::Initialize()
{
    int x;
    DWORD ptid = 0;
    DWORD ptid2 = 0;

    MicroStackChain = ILibCreateChain();

    ILib_Pool = ILibThreadPool_Create();
    for( x = 0; x < 3; ++x ) {
        CreateThread(NULL,0, &ILibPoolThread, ILib_Pool, 0, &ptid);
    }
    return true;
}

bool CAirPlayServer::StartServer(int port, bool nonlocal)
{
    StopServer(true);

    ServerInstance = new CAirPlayServer(port, nonlocal);
    if (ServerInstance->Initialize())
    {
        ServerInstance->Create();
        return true;
    }
    else
        return false;

}

bool CAirPlayServer::SetCredentials(bool usePassword, const CStdString& password)
{
  bool ret = false;

  if (ServerInstance)
  {
    ret = ServerInstance->SetInternalCredentials(usePassword, password);
  }
  return ret;
}

bool CAirPlayServer::SetInternalCredentials(bool usePassword, const CStdString& password)
{
    m_usePassword = usePassword;
    m_password = password;
    return true;
}

void CAirPlayServer::Stop()
{
    ILibStopChain(MicroStackChain);
}

void CAirPlayServer::StopServer(bool bWait)
{
    if (ServerInstance)
    {
        ServerInstance->Stop();
        ServerInstance->StopThread(bWait);
        if (bWait)
        {
            delete ServerInstance;
            ServerInstance = NULL;
        }
    }
}

CAirPlayServer::CAirPlayServer(int port, bool nonlocal)
{
    m_port = port;
    m_nonlocal = nonlocal;
}

#define LogFun() printf("[%s] [%s] <%d>\n", __FILE__, __FUNCTION__, __LINE__)

int APW_Callback_SetAVTransportURI (  APW instance, void * session, char * uri, char * metadata)
{
    LogFun();
    CStdString userAgent="AppleCoreMedia/1.0.0.8F455 (AppleTV; U; CPU OS 4_3 like Mac OS X; de_de)", location = uri;
    CURL::Encode(userAgent);
    location += "|User-Agent=" + userAgent;

    CFileItem fileToPlay(location, false);
    fileToPlay.SetProperty("StartPercent", (0.0f));
    g_application.getApplicationMessenger().MediaPlay(fileToPlay);
    CLog::Log(LOGDEBUG, "AIRPLAY: SetAVTransportURI with uri = %s", location.c_str());
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
    if (CAirPlayServer::m_isPlaying > 0)
    {
        CLog::Log(LOGDEBUG, "AIRPLAY: Stop");
        g_application.getApplicationMessenger().MediaStop();
        CAirPlayServer::m_isPlaying--;
        APW_StateChange_TransportPlayState(instance, APW_PS_Stopped);
    }
    else
    {
        g_windowManager.PreviousWindow();
    }
    return 0;
}

int APW_Callback_Play (               APW instance, void * session, char* playSpeed)
{
    LogFun();
    if (g_application.m_pPlayer && g_application.m_pPlayer->IsPlaying() && g_application.m_pPlayer->IsPaused())
    {
        CLog::Log(LOGDEBUG, "AIRPLAY: Play");
        g_application.getApplicationMessenger().MediaPause();
        APW_StateChange_TransportPlayState(instance, APW_PS_Playing);
    }
    return 0;
}

int APW_Callback_Pause (              APW instance, void * session)
{
    LogFun();
    if (g_application.m_pPlayer && g_application.m_pPlayer->IsPlaying() && !g_application.m_pPlayer->IsPaused())
    {
        CLog::Log(LOGDEBUG, "AIRPLAY: Pause");
        g_application.getApplicationMessenger().MediaPause();
        APW_StateChange_TransportPlayState(instance, APW_PS_Paused);
    }
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
    if (g_application.m_pPlayer)
    {
        //g_application.m_pPlayer->SeekTime(position);
        CLog::Log(LOGDEBUG, "AIRPLAY: Seek with pos %ld", position);
    }
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
    int oldVolume = g_application.GetVolume();
    volume *= 100;
    if(oldVolume != (int)volume)
    {
        CLog::Log(LOGDEBUG, "AIRPLAY: SetVolume with volume %d", (int)volume);
        g_application.SetVolume(volume);
        g_application.getApplicationMessenger().ShowVolumeBar(oldVolume < volume);
    }
    return 0;
}

int APW_Callback_SetMute (            APW instance, void * session, BOOL mute)
{
    LogFun();
    CLog::Log(LOGDEBUG, "AIRPLAY: SetMute with Mute = %s", mute ? "TRUE" : "FALSE");
    if (mute) {
        if (!g_application.IsMuted()) {
            g_application.ToggleMute();
        }
    } else {
        if (g_application.IsMuted()) {
            g_application.ToggleMute();
        }
    }
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

void CAirPlayServer::Process()
{
    CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
    apw = APW_Method_Create(
        MicroStackChain, ILib_Pool, m_port, "LeoChenPlayer", iface ? iface->GetMacAddress().c_str() : "FF:FF:FF:FF:FF:F2", m_usePassword ? m_password.c_str():"");

    if (apw == NULL) { // ´´½¨Ê§°Ü
        ILibThreadPool_Destroy(ILib_Pool);
        return;
    }

    apw->Event_SetAVTransportURI    = APW_Callback_SetAVTransportURI;
    apw->Event_GetAVProtocolInfo    = APW_Callback_GetAVProtocolInfo;
    apw->Event_SetPlayMode          = APW_Callback_SetPlayMode;
    apw->Event_Stop                 = APW_Callback_Stop;
    apw->Event_Play                 = APW_Callback_Play;
    apw->Event_Pause                = APW_Callback_Pause;
    apw->Event_SeekTrack            = APW_Callback_SeekTrack;
    apw->Event_SeekTrackPosition    = APW_Callback_SeekTrackPosition;
    apw->Event_SeekMediaPosition    = APW_Callback_SeekMediaPosition;
    apw->Event_Next                 = APW_Callback_Next;
    apw->Event_Previous             = APW_Callback_Previous;
    apw->Event_SelectPreset         = APW_Callback_SelectPreset;
    apw->Event_SetVolume            = APW_Callback_SetVolume;
    apw->Event_SetMute              = APW_Callback_SetMute;
    apw->Event_SetContrast          = APW_Callback_SetContrast;
    apw->Event_SetBrightness        = APW_Callback_SetBrightness;

    ILibStartChain(MicroStackChain);
}

#endif
