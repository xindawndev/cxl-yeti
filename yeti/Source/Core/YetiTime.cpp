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

TimeStamp::TimeStamp(const TimeStamp & timestamp)
{
    m_nano_seconds_ = timestamp.m_nano_seconds_;
}

TimeStamp::TimeStamp(double seconds)
{
    m_nano_seconds_ = (YETI_Int64)(seconds * 1e9);
}

TimeStamp & TimeStamp::operator +=(const TimeStamp & t)
{
    m_nano_seconds_ += t.m_nano_seconds_;
    return *this;
}

TimeStamp & TimeStamp::operator -=(const TimeStamp & t)
{
    m_nano_seconds_ -= t.m_nano_seconds_;
    return *this;
}

static int match_string(const char * string, const char * const *list, unsigned int list_length)
{
    for (unsigned int i = 0; i < list_length; ++i) {
        if (StringEqual(string, list[i])) return i;
    }

    return -1;
}

static YETI_UInt32 elapsed_leap_years_since1900(YETI_UInt32 year)
{
    if (year < 1901) return 0;
    YETI_UInt32 years_since_1900 = year - 1 - 1900; // not including the current year
    return years_since_1900 / 4 - years_since_1900 / 100 + (years_since_1900 + 300) / 400;
}

static YETI_UInt32 elapsed_days_since1900(const DateTime & date)
{
    YETI_UInt32 day_count = YETI_TIME_ELAPSED_DAYS_AT_MONTH[date.m_month_ -1] + date.m_day_ - 1;
    if (YETI_TIME_YEAR_IS_LEAP(date.m_year_) && (date.m_month_ > 2)) ++day_count;
    YETI_UInt32 leap_year_count = elapsed_leap_years_since1900(date.m_year_);
    day_count += (date.m_year_ - 1900) * 365 + leap_year_count;

    return day_count;
}

DateTime::DateTime()
: m_year_(1970)
, m_month_(1)
, m_day_(1)
, m_hours_(0)
, m_minutes_(0)
, m_seconds_(0)
, m_nanoseconds_(0)
, m_timezone_(0)
{
}

DateTime::DateTime(const TimeStamp & timestamp, bool local /* = false */)
{
    from_timestamp(timestamp, local);
}

YETI_Result DateTime::change_timezone(YETI_Int32 timezone)
{
    if (timezone < -12 * 60 || timezone > 12 * 60) return YETI_ERROR_OUT_OF_RANGE;
    TimeStamp ts;
    YETI_Result result = to_timestamp(ts);
    if (YETI_FAILED(result)) return result;
    ts.set_nanos(ts.to_nanos() + (YETI_Int64)timezone * (YETI_Int64)60 * (YETI_Int64)1000000000);
    result = from_timestamp(ts);
    m_timezone_ = timezone;
    return result;
}

YETI_Result DateTime::from_timestamp(const TimeStamp & timestamp, bool local /* = false */)
{
    YETI_Int64 seconds = timestamp.to_seconds();
    if (seconds < 0 && (YETI_Int32)seconds != seconds) return YETI_ERROR_OUT_OF_RANGE;
    YETI_Int32 timezone = 0;
    if (local) {
        timezone = get_local_timezone();
        seconds += timezone * 60;
    }

    seconds += (YETI_Int64)YETI_SECONDS_PER_YEAR * 60 + (YETI_Int64)(17 * YETI_SECONDS_PER_DAY);
    YETI_UInt32 years_since_1900 = (YETI_UInt32)(seconds / YETI_SECONDS_PER_YEAR);
    seconds -= (YETI_Int64)years_since_1900 * YETI_SECONDS_PER_YEAR;
    bool is_leap_year = false;
    YETI_UInt32 leap_year_since_1900 = elapsed_leap_years_since1900(years_since_1900 + 1900);
    if (seconds < (leap_year_since_1900 * YETI_SECONDS_PER_DAY)) {
        seconds += YETI_SECONDS_PER_YEAR;
        seconds -= leap_year_since_1900 * YETI_SECONDS_PER_DAY;
        --years_since_1900;
        if (YETI_TIME_YEAR_IS_LEAP(years_since_1900 + 1900)) {
            seconds += YETI_SECONDS_PER_DAY;
            is_leap_year =  true;
        }
    } else {
        seconds -= leap_year_since_1900 * YETI_SECONDS_PER_DAY;
        if (YETI_TIME_YEAR_IS_LEAP(years_since_1900 + 1900)) {
            is_leap_year = true;
        }
    }

    m_year_ =years_since_1900 + 1900;
    YETI_UInt32 day_of_the_year = (YETI_UInt32)(seconds / YETI_SECONDS_PER_DAY);
    seconds -= day_of_the_year * YETI_SECONDS_PER_DAY;

    const YETI_Int32 * month_day = is_leap_year ? YETI_TIME_MONTH_DAY_LEAP : YETI_TIME_MONTH_DAY;
    YETI_UInt32 month;
    for (month = 1; month_day[month] < (YETI_Int32)day_of_the_year; ++month) {}
    m_month_ = month;
    m_day_ = day_of_the_year - month_day[month - 1];
    m_hours_ = (YETI_Int32)seconds / 3600;
    seconds  -= m_hours_ * 3600L;
    m_minutes_ = (YETI_Int32)seconds / 60;
    m_seconds_ = (YETI_Int32)seconds - m_minutes_ * 60;
    m_nanoseconds_ = (YETI_Int32)(timestamp.to_nanos() % 1000000000);
    if (local) {
        m_timezone_ = timezone;
    } else {
        m_timezone_ = 0;
    }
    
    return YETI_SUCCESS;
}

static YETI_Result check_date(const DateTime & date)
{
    YETI_TIME_CHECK_BOUNDS(date.m_year_, YETI_DATETIME_YEAR_MIN, YETI_DATETIME_YEAR_MAX);
    YETI_TIME_CHECK_BOUNDS(date.m_month_,       1, 12);
    YETI_TIME_CHECK_BOUNDS(date.m_day_,         1, 31);
    YETI_TIME_CHECK_BOUNDS(date.m_hours_,       0, 23);
    YETI_TIME_CHECK_BOUNDS(date.m_minutes_,     0, 59);
    YETI_TIME_CHECK_BOUNDS(date.m_seconds_,     0, 59);
    YETI_TIME_CHECK_BOUNDS(date.m_nanoseconds_, 0, 999999999);
    YETI_TIME_CHECK_BOUNDS(date.m_timezone_,   -12*60, 12*60);
    return YETI_SUCCESS;
}

YETI_Result DateTime::to_timestamp(TimeStamp & timestamp) const
{
    timestamp.set_nanos(0);
    YETI_Result result = check_date(*this);
    if (YETI_FAILED(result)) return result;

    YETI_UInt32 days = elapsed_days_since1900(*this);
    YETI_Int64 seconds = (YETI_Int64)days      * (24*60*60) + 
        (YETI_Int64)m_hours_   * (60*60) +
        (YETI_Int64)m_minutes_ * (60) + 
        (YETI_Int64)m_seconds_;
    seconds -= (YETI_Int64)m_timezone_ * 60;
    seconds -= (YETI_Int64)YETI_SECONDS_PER_YEAR * 70 + (YETI_Int64)(17 * YETI_SECONDS_PER_DAY);
    timestamp.from_nanos(seconds * 1000000000 + m_nanoseconds_);

    return YETI_SUCCESS;
}

static void append_number(String & output, YETI_UInt32 number, unsigned int digit_count)
{
    YETI_Size new_length = output.get_length() + digit_count;
    output.set_length(new_length);
    char * dest = output.use_chars() + new_length;
    while (digit_count--) {
        *--dest = '0' + (number % 10);
        number /= 10;
    }
}

String DateTime::to_string(_eFormat format /* = FORMAT_ANSI */, YETI_Flags flags /* = 0 */) const
{
    String result;
    if (YETI_FAILED(check_date(*this))) return result;
    switch (format)
    {
    case FORMAT_W3C:
        append_number(result, m_year_, 4);
        result += '-';

        break;
    case FORMAT_ANSI:
        break;
    case FORMAT_RFC_1036:
        break;
    case FORMAT_RFC_1123:
        break;
    }
    return result;
}

YETI_Result DateTime::from_string(const char * date, _eFormat format /* = FORMAT_ANSI */) 
{
    return check_date(*this);
}
NAMEEND
