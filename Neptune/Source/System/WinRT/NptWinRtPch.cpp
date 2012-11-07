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
