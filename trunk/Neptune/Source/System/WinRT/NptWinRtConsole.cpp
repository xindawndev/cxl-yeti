/*****************************************************************
|
|   Neptune - Console Support: Win32 Implementation
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptWinRtPch.h"

#include "NptConfig.h"
#include "NptConsole.h"

/*----------------------------------------------------------------------
|   NPT_Console::Output
+---------------------------------------------------------------------*/
void
NPT_Console::Output(const char* message)
{
	NPT_WIN32_USE_CHAR_CONVERSION;
    OutputDebugStringW(NPT_WIN32_A2W(message));
}

