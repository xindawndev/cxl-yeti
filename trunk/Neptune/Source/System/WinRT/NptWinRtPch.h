/*****************************************************************
|
|      Neptune - WinRT precompiled headers
|
|      (c) 2002-2012 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <windows.h>
#include <ppltasks.h>

#include "Neptune.h"

#define NPT_WIN32_USE_CHAR_CONVERSION int _convert = 0; LPCWSTR _lpw = NULL; LPCSTR _lpa = NULL

#define NPT_WIN32_A2W(lpa) (\
    ((_lpa = lpa) == NULL) ? NULL : (\
    _convert = (int)(strlen(_lpa)+1),\
    (INT_MAX/2<_convert)? NULL :  \
    A2WHelper((LPWSTR) alloca(_convert*sizeof(WCHAR)), _lpa, _convert, CP_UTF8)))

LPWSTR A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp);

NPT_String Str2NPT_Str(Platform::String^ str);
Platform::String^ NPT_Str2Str(NPT_String str);