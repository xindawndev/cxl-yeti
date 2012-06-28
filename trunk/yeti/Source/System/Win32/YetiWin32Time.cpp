#include <windows.h>

#include "YetiTime.h"
#include "YetiResults.h"

NAMEBEG

YETI_Int32 DateTime::get_local_timezone()
{
    TIME_ZONE_INFORMATION tz_info;
    DWORD result = ::GetTimeZoneInformation(&tz_info);
    if (result == TIME_ZONE_ID_INVALID) return 0;
    return -tz_info.Bias;
}

NAMEEND
