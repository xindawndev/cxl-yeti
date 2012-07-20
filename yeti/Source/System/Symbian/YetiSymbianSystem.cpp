#include "e32cmn.h"
#include "e32math.h"
#include "sys/time.h"

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiSystem.h"
#include "YetiResults.h"
#include "YetiDebug.h"

NAMEBEG

static TInt64 System_RandomGeneratorSeed = 0;

YETI_Result System::get_process_id(YETI_UInt32 & id)
{
    //id = getpid();
    id = 0;
    return YETI_SUCCESS;
}

YETI_Result System::get_current_timestamp(TimeStamp & now)
{
    struct timeval now_tv;

    /* get current time from system */
    if (gettimeofday(&now_tv, NULL)) {
        now.m_seconds_     = 0;
        now.m_nano_seconds_ = 0;
        return YETI_FAILURE;
    }

    /* convert format */
    now.m_seconds_     = now_tv.tv_sec;
    now.m_nano_seconds_ = now_tv.tv_usec * 1000;

    return YETI_SUCCESS;
}

YETI_Result System::sleep(const TimeInterval & duration)
{
    TTimeIntervalMicroSeconds32  milliseconds = 1000 * duration.m_seconds_ + duration.m_nano_seconds_/1000000;
    User::After(milliseconds); /* FIXME: this doesn't behave like a normal sleep() where the processor idles. Need to use CTimer much more complicated logic. */

    return YETI_SUCCESS;
}

YETI_Result System::sleep_until(const TimeStamp & when)
{
    TimeStamp now;
    get_current_timestamp(now);
    if (when > now) {
        TimeInterval duration = when-now;
        return sleep(duration);
    } else {
        return YETI_SUCCESS;
    }
}

YETI_Result System::set_random_integer(unsigned int seed)
{
    System_RandomGeneratorSeed = seed;
    return YETI_SUCCESS;
}

YETI_UInt32 System::get_random_integer()
{
    if (!System_RandomGeneratorSeed) {
        TTime time;
        time.HomeTime();
        
        System::set_random_integer(time.Int64());
    }

    return Math::Rand(System_RandomGeneratorSeed);
}

NAMEEND
