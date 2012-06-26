#include "YetiTime.h"

#include "YetiUtil.h"

NAMEBEG

const char * const YETI_TIME_DAYS_SHORT[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
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
        append_number(result, m_month_, 2);
        result += '-';
        append_number(result, m_day_, 2);
        result += 'T';
        append_number(result, m_hours_, 2);
        result += ':';
        append_number(result, m_minutes_, 2);
        result += ':';
        append_number(result, m_seconds_, 2);
        if (flags & FLAG_EMIT_FRACTION) {
            result += '.';
            if (flags & FLAG_EXTENDED_PRECISION) {
                append_number(result, m_nanoseconds_, 9);
            } else {
                append_number(result, m_nanoseconds_ / 1000000, 3);
            }
        }
        if (m_timezone_) {
            YETI_UInt32 tz;
            if (m_timezone_ > 0) {
                result += '+';
                tz = m_timezone_;
            } else {
                result += '-';
                tz = -m_timezone_;
            }
            append_number(result, tz / 60, 2);
            result += ':';
            append_number(result, tz % 60, 2);
        } else {
            result += 'Z';
        }
        break;
    case FORMAT_ANSI: {
        YETI_UInt32 days = elapsed_days_since1900(*this);
        result.set_length(24);
        FormatString(result.use_chars(), result.get_length() + 1,
            "%.3s %.3s%3d %.2d:%.2d:%.2d %d",
            YETI_TIME_DAYS_SHORT[(days + 1) % 7],
            YETI_TIME_MONTHS[m_month_ - 1],
            m_day_,
            m_hours_,
            m_minutes_,
            m_seconds_,
            m_year_);
                      }
        break;
    case FORMAT_RFC_1036:
    case FORMAT_RFC_1123: {
           YETI_UInt32 days = elapsed_days_since1900(*this);
           if (format == FORMAT_RFC_1036) {
               result += YETI_TIME_DAYS_LONG[(days + 1) % 7];
               result += ", ";
               append_number(result, m_day_, 2);
               result += '-';
               result += YETI_TIME_MONTHS[m_minutes_ - 1];
               result += '-';
               append_number(result, m_year_ % 100, 2);
           } else {
               result += YETI_TIME_DAYS_SHORT[(days + 1) % 7];
               result += ", ";
               append_number(result, m_day_, 2);
               result += ' ';
               result += YETI_TIME_MONTHS[m_month_ - 1];
               result += ' ';
               append_number(result, m_year_, 4);
           }
           result += ' ';
           append_number(result, m_hours_, 2);
           result += ':';
           append_number(result, m_minutes_, 2);
           result += ':';
           append_number(result, m_seconds_, 2);
           if (m_timezone_) {
               if (m_timezone_ > 0) {
                   result += "+";
                   append_number(result, m_timezone_ / 60, 2);
                   append_number(result, m_timezone_ % 60, 2);
               } else {
                   result += '-';
                   append_number(result, -m_timezone_ / 60, 2);
                   append_number(result, -m_timezone_ % 60, 2);
               }
           } else {
               result += " GMT";
           }
                          }
        break;
    }
    return result;
}

YETI_Result DateTime::from_string(const char * date, _eFormat format /* = FORMAT_ANSI */) 
{
    if (date == NULL || date[0] == '\0') return YETI_ERROR_INVALID_PARAMETERS;
    String workspace(date);
    char * input = workspace.use_chars();
    YETI_Size input_size = workspace.get_length();
    switch (format) {
        case FORMAT_W3C: {
            if (input_size < 17 && input_size != 10) return YETI_ERROR_INVALID_SYNTAX;
            if (input[4] != '-' ||
                input[7] != '-') {
                    return YETI_ERROR_INVALID_SYNTAX;
            }

            input[4] = input[7] = '\0';
            bool no_seconds = true;
            if (input_size > 10) {
                if (input[10] != 'T' ||
                    input[13] != ':') {
                        return YETI_ERROR_INVALID_SYNTAX;
                }
                input[10] = input[13] = '\0';
                if (input[16] == ':') {
                    input[16] = '\0';
                    no_seconds = false;
                    if (input_size < 20) return YETI_ERROR_INVALID_SYNTAX;
                } else {
                    m_seconds_ = 0;
                }
            }
            if (YETI_FAILED(parse_integer(input, m_year_, false)) ||
                YETI_FAILED(parse_integer(input + 5, m_month_, false)) ||
                YETI_FAILED(parse_integer(input + 8, m_day_, false))) {
                return YETI_ERROR_INVALID_SYNTAX;
            }
            if (input_size > 10) {
                if (input[input_size - 1] == 'Z') {
                    m_timezone_ = 0;
                } else if (input[input_size - 6] == '+' || input[input_size - 6] == '-') {
                    if (input[input_size - 3] != ':') return YETI_ERROR_INVALID_SYNTAX;
                    input[input_size - 3] = '\0';
                    unsigned int hh, mm;
                    if (YETI_FAILED(parse_integer(input + input_size - 5, hh, false)) ||
                        YETI_FAILED(parse_integer(input + input_size - 2, mm, false))) {
                        return YETI_ERROR_INVALID_SYNTAX;
                    }
                    if (hh > 59 || mm > 59) return YETI_ERROR_INVALID_SYNTAX;
                    m_timezone_ = hh * 60 + mm;
                    if (input[input_size - 6] == '-') m_timezone_ = -m_timezone_;
                    input[input_size - 6] = '\0';
                }
                if (YETI_FAILED(parse_integer(input + 11, m_hours_, false)) || 
                    YETI_FAILED(parse_integer(input + 14, m_minutes_, false))) {
                    return YETI_ERROR_INVALID_SYNTAX;
                }
                if (!no_seconds && input[19] == '.') {
                    char fraction[10];
                    fraction[9] = '\0';
                    unsigned int fraction_size = StringLength(input + 20);
                    if (fraction_size == 0) return YETI_ERROR_INVALID_SYNTAX;
                    for (unsigned int i = 0; i < 9; ++i) {
                        if (i < fraction_size) {
                            fraction[i] = input[20 + i];
                        } else {
                            fraction[i] = '\0';
                        }
                    }
                    if (YETI_FAILED(parse_integer(fraction, m_nanoseconds_, false))) {
                        return YETI_ERROR_INVALID_SYNTAX;
                    }
                    input[19] = '\0';
                } else {
                    m_nanoseconds_ = 0;
                }
                if (!no_seconds) {
                    if (YETI_FAILED(parse_integer(input + 17, m_seconds_, false))) {
                        return YETI_ERROR_INVALID_SYNTAX;
                    }
                }
            }
                         }
            break;
        case FORMAT_RFC_1036:
        case FORMAT_RFC_1123: {
            if (input_size < 26) return YETI_ERROR_INVALID_SYNTAX;
            const char * wday = input;
            while (*input && *input != ',') {
                ++input;
                --input_size;
            }
            if (*input == '\0' || *wday == ',') return YETI_ERROR_INVALID_SYNTAX;
            *input++ = '\0';
            --input_size;
            char * timezone = input + input_size - 1;
            unsigned int timezone_size = 0;
            while (input_size && *timezone != ' ') {
                --timezone;
                ++timezone_size;
                --input_size;
            }
            if (input_size == 0) return YETI_ERROR_INVALID_SYNTAX;
            *timezone++ = '\0';
            if (input_size < 20) return YETI_ERROR_INVALID_SYNTAX;
            unsigned int yl = input_size - 18;
            if (yl != 2 && yl != 4) return YETI_ERROR_INVALID_SYNTAX;
            char sep;
            int wday_index;
            if (format == FORMAT_RFC_1036) {
                sep = '-';
                wday_index = match_string(wday, YETI_TIME_DAYS_LONG, 7);
            } else {
                sep = ' ';
                wday_index = match_string(wday, YETI_TIME_DAYS_SHORT, 7);
            }
            if (input[0] != ' ' ||
                input[3] != sep ||
                input[7] != sep ||
                input[8 + yl] != ' ' ||
                input[11 + yl] != ':' ||
                input[14 + yl] != ':') {
                return YETI_ERROR_INVALID_SYNTAX;
            }
            input[3] = input[7] = input[8 + yl] = input[11 + yl] = input[14 + yl] = '\0';
            m_month_ = 1 + match_string(input + 4, YETI_TIME_MONTHS, 12);
            if (YETI_FAILED(parse_integer(input + 1, m_day_, false)) ||
                YETI_FAILED(parse_integer(input + 8, m_year_, false)) ||
                YETI_FAILED(parse_integer(input + 9 + yl, m_hours_, false)) ||
                YETI_FAILED(parse_integer(input + 12 + yl, m_minutes_, false)) ||
                YETI_FAILED(parse_integer(input + 15 + yl, m_seconds_, false))) {
                return YETI_ERROR_INVALID_SYNTAX;
            }
            if (yl == 2) m_year_ += 1900;
            if (StringEqual(timezone, "GMT") ||
                StringEqual(timezone, "UT") ||
                StringEqual(timezone, "Z")) {
                m_timezone_ = 0;
            } else if (StringEqual(timezone, "EDT")) {
                m_timezone_ = -4 * 60;
            } else if (StringEqual(timezone, "EST") ||
                StringEqual(timezone, "CDT")) {
                    m_timezone_ = -5 * 60;
            } else if (StringEqual(timezone, "CST") ||
                StringEqual(timezone, "MDT")) {
                    m_timezone_ = -6 * 60;
            } else if (StringEqual(timezone, "MST") ||
                StringEqual(timezone, "PDT")) {
                    m_timezone_ = -7 * 60;
            } else if (StringEqual(timezone, "PST")) {
                m_timezone_ = -8 * 60;
            } else if (timezone_size == 1) {
                if (timezone[0] >= 'A' && timezone[0] <= 'I') {
                    m_timezone_ = -60 * (1 + timezone[0] - 'A');
                } else if (timezone[0] >= 'K' && timezone[0] <= 'M') {
                    m_timezone_ = -60 * (timezone[0] - 'A');            
                } else if (timezone[0] >= 'N' && timezone[0] <= 'Y') {
                    m_timezone_ = 60 * (1 + timezone[0] - 'N');
                } else {
                    return YETI_ERROR_INVALID_SYNTAX;
                }
            } else if (timezone_size == 5) {
                int sign;
                if (timezone[0] == '-') {
                    sign = -1;
                } else if (timezone[0] == '+') {
                    sign = 1;
                } else {
                    return YETI_ERROR_INVALID_SYNTAX;
                }
                YETI_UInt32 tz;
                if (YETI_FAILED(parse_integer(timezone + 1, tz, false))) {
                    return YETI_ERROR_INVALID_SYNTAX;
                }
                unsigned int hh = (tz / 100);
                unsigned int mm = (tz % 100);
                if (hh > 59 || mm > 59) return YETI_ERROR_INVALID_SYNTAX;
                m_timezone_ = sign * (hh * 60 + mm);
            } else {
                return YETI_ERROR_INVALID_SYNTAX;
            }
            YETI_UInt32 days = elapsed_days_since1900(*this);
            if ((int)((days + 1) % 7) != wday_index) {
                return YETI_ERROR_INVALID_PARAMETERS;
            }
            m_nanoseconds_ = 0;
                              }
            break;
        case FORMAT_ANSI: {
            if (input_size != 24) return YETI_ERROR_INVALID_SYNTAX;
            if (input[3] != ' ' ||
                input[7] != ' ' ||
                input[10] != ' ' ||
                input[13] != ':' ||
                input[16] != ':' ||
                input[19] != ' ') {
                return YETI_ERROR_INVALID_SYNTAX;
            }
            input[3] = input[7] = input[10] = input[13] = input[16] = input[19] = '\0';
            if (input[8] == ' ') input[8] = '0';
            m_month_ = 1 + match_string(input + 4, YETI_TIME_MONTHS, 12);
            if (YETI_FAILED(parse_integer(input + 8, m_day_, false)) ||
                YETI_FAILED(parse_integer(input + 11, m_hours_, false)) ||
                YETI_FAILED(parse_integer(input + 14, m_minutes_, false)) ||
                YETI_FAILED(parse_integer(input + 17, m_seconds_, false)) ||
                YETI_FAILED(parse_integer(input + 20, m_year_, false))) {
                return YETI_ERROR_INVALID_SYNTAX;
            }
            YETI_UInt32 days = elapsed_days_since1900(*this);
            if ((int)((days + 1) % 7) != match_string(input, YETI_TIME_DAYS_SHORT, 7)) {
                return YETI_ERROR_INVALID_PARAMETERS;
            }
            m_timezone_ = 0;
            m_nanoseconds_ = 0;
                          }
            break;
        default:
            return YETI_ERROR_INVALID_PARAMETERS;
    }
    return check_date(*this);
}
NAMEEND
