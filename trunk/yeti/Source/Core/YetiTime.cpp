#include "YetiTime.h"

#include "YetiUtil.h"

NAMEBEG

const char * const YETI_TIME_DAYS_SHORTS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char * const YETI_TIME_DAYS_LONG[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char * const YETI_TIME_MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static const YETI_Int32 YETI_TIME_MONTH_DAY[] = {-1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364 };
static const YETI_Int32 YETI_TIME_MONTH_DAY_LEAP[] = {-1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
static const YETI_Int32 YETI_TIME_ELAPSED_DAYS_AT_MONTH[13] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

const YETI_Int32 YETI_SECONDS_PER_DAY = (24L * 60L * 60L);
const YETI_Int32 YETI_SECONDS_PER_YEAR = (365L * YETI_SECONDS_PER_DAY);

#define YETI_TIME_YEAR_IS_LEAP(_y) ((((_y) % 4 == 0) && ((_y) % 100 != 0)) || ((_y) % 400 == 0))
#define YETI_TIME_CHECK_BOUNDS(_var, _low, _high) do {  \
    if (((_var ) < (_low)) || ((_var) > (_high))) {     \
        return YETI_ERROR_OUT_OF_RANGE;                 \
    }                                                   \
} while(0)

NAMEEND
