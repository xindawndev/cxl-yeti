#if defined(_XBOX)
#include <xtl.h>
#else
#include <windows.h>
#endif

#if !defined(_WIN32_WCE)
#include <sys/timeb.h>
#endif

#include "YetiSystem.h"
#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiDebug.h"
#include "YetiUtil.h"
#include "YetiResults.h"

NAMEBEG

YETI_Result System::get_process_id(YETI_UInt32 & id)
{
    id = ::GetCurrentProcessId();
    return YETI_SUCCESS;
}

YETI_Result System::get_machine_name(String & name)
{
    return GetEnvironment("COMPUTERNAME", name);
}

#if defined(_WIN32_WCE)
YETI_Result System::get_current_timestamp(TimeStamp & now)
{
    SYSTEMTIME stime;
    FILETIME ftime;
    __int64 time64;
    ::GetSystemTime(&stime);
    ::SystemTimeToFileTime(&stime, &ftime);

    /* convert to 64-bits 100-nanoseconds value */
    time64 = (((unsigned __int64)ftime.dwHighDateTime) << 32) | ((unsigned __int64)ftime.dwLowDateTime);
    time64 -= 116444736000000000; /* convert from the Windows epoch (Jan. 1, 1601) to the 
                                   * Unix epoch (Jan. 1, 1970) */
    
    now.m_seconds_ = (YETI_Int32)(time64 / 10000000);
    now.m_nano_seconds_ = 100 * (YETI_Int32)(time64-((unsigned __int64)now.m_Seconds * 10000000));

    return YETI_SUCCESS;
}
#else
YETI_Result System::get_current_timestamp(TimeStamp & now)
{
    struct _timeb time_stamp;
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
    _ftime_s(&time_stamp);
#else
    _ftime(&time_stamp);
#endif
    now.set_nanos(((YETI_UInt64)time_stamp.time) * 1000000000UL +
        ((YETI_UInt64)time_stamp.millitm) * 1000000);

    return YETI_SUCCESS;
}
#endif

YETI_Result System::sleep(const TimeInterval & duration)
{
    ::Sleep((YETI_UInt32)duration.to_millis());
    return YETI_SUCCESS;
}

YETI_Result System::sleep_until(const TimeStamp & when)
{
    TimeStamp now;
    get_current_timestamp(now);
    if (when > now) {
        TimeInterval duration = when - now;
        return sleep(duration);
    }

    return YETI_SUCCESS;
}

YETI_Result System::set_random_integer(unsigned int seed)
{
    srand(seed);
    return YETI_SUCCESS;
}

YETI_UInt32 System::get_random_integer()
{
    static bool seeded = false;
    if (seeded == false) {
        TimeStamp now;
        get_current_timestamp(now);
        srand((YETI_UInt32)now.to_nanos());
        seeded = true;
    }

    return rand();
}

NAMEEND
