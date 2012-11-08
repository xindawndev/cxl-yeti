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
#include "NptWinRtPch.h"

NPT_String Str2NPT_Str(Platform::String^ str)
{
    NPT_String ret_str = "";
    if (str->IsEmpty()) return ret_str;

    size_t sz = 2 * wcslen(str->Data()) + 1;
    char * tmp = new char[sz];
    wcstombs_s(&sz, tmp , sz, str->Data(), sz);
    ret_str = tmp;
    delete []tmp;
    return ret_str;
}

LPWSTR A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp)
{
    int ret;

    if (lpw == NULL || lpa == NULL) return NULL;

    lpw[0] = '\0';
    ret = MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars);
    if (ret == 0) {
        return NULL;
    }        
    return lpw;
}

Platform::String^ NPT_Str2Str(NPT_String str)
{
    NPT_WIN32_USE_CHAR_CONVERSION;
    Platform::String^ ret_str = ref new Platform::String(NPT_WIN32_A2W(str.GetChars()));
    return ret_str;
}

