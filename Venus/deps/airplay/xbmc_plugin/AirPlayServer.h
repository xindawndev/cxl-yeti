#pragma once
/*
 * Many concepts and protocol specification in this code are taken from
 * the Boxee project. http://www.boxee.tv
 *
 *      Copyright (C) 2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#ifdef HAS_AIRPLAY

#include <map>
#include <sys/socket.h>
#include "threads/Thread.h"
//#include "threads/CriticalSection.h"
//#include "utils/HttpParser.h"
#include "utils/StdString.h"

extern "C"{
#include "ILibParsers.h"
#include "ILibThreadPool.h"
#include "AirplayWrapper.h"
}

#define AIRPLAY_SERVER_VERSION_STR "101.28"

class CAirPlayServer : public CThread
{
public:
  static bool StartServer(int port, bool nonlocal);
  static void StopServer(bool bWait);
  static bool SetCredentials(bool usePassword, const CStdString& password);
  static bool IsPlaying(){ return m_isPlaying > 0;}
  static int m_isPlaying;

private:
    CAirPlayServer(int port, bool nonlocal);
    bool Initialize();
    void Stop();
    bool SetInternalCredentials(bool usePassword, const CStdString& password);

    void * MicroStackChain;
    void * ILib_Pool;
    APW    apw;

    int m_port;
    bool m_nonlocal;
    bool m_usePassword;
    CStdString m_password;

protected:
    void Process();

  static CAirPlayServer *ServerInstance;
};

#endif
