#ifndef _CXL_YETI_TIME_H_
#define _CXL_YETI_TIME_H_

#include "YetiTypes.h"
#include "YetiString.h"

#define YETI_DATETIME_YEAR_MIN 1901
#define YETI_DATETIME_YEAR_MAX 2262

NAMEBEG

class TimeStamp
{
public:
    TimeStamp(const TimeStamp & timestamp);
    TimeStamp() : m_nano_seconds_(0) {}
    TimeStamp(YETI_Int64 nanoseconds) : m_nano_seconds_(nanoseconds) {}
    TimeStamp(double seconds);

    TimeStamp & operator+=(const TimeStamp & time_stamp);
    TimeStamp & operator-=(const TimeStamp & time_stamp);

    bool operator==(const TimeStamp & t) const { return m_nano_seconds_ == t.m_nano_seconds_; }
    bool operator!=(const TimeStamp & t) const { return m_nano_seconds_ != t.m_nano_seconds_; }
    bool operator> (const TimeStamp & t) const { return m_nano_seconds_ > t.m_nano_seconds_; }
    bool operator< (const TimeStamp & t) const { return m_nano_seconds_ < t.m_nano_seconds_; }
    bool operator>=(const TimeStamp & t) const { return m_nano_seconds_ >= t.m_nano_seconds_; }
    bool operator<=(const TimeStamp & t) const { return m_nano_seconds_ <= t.m_nano_seconds_; }

    void set_nanos(YETI_Int64 nanoseconds) { m_nano_seconds_ = nanoseconds; }
    void set_micros(YETI_Int64 micros) { m_nano_seconds_ = micros / 1000; }
    void set_millis(YETI_Int64 millis) { m_nano_seconds_ = millis / 1000000; }
    void set_seconds(YETI_Int64 seconds) { m_nano_seconds_ = seconds / 1000000000; }

    operator double() const { return (double)m_nano_seconds_ / 1E9; }
    void from_nanos(YETI_Int64 nanoseconds) { m_nano_seconds_ = nanoseconds; }
    YETI_Int64 to_nanos() const { return m_nano_seconds_; }
    YETI_Int64 to_micros() const { return m_nano_seconds_ / 1000; }
    YETI_Int64 to_millis() const { return m_nano_seconds_ / 1000000; }
    YETI_Int64 to_seconds() const { return m_nano_seconds_ / 1000000000; }

private:
    YETI_Int64 m_nano_seconds_;
};

inline TimeStamp operator+(const TimeStamp & t1, const TimeStamp & t2)
{
    TimeStamp t = t1;
    return t += t2;
}

inline TimeStamp operator-(const TimeStamp & t1, const TimeStamp & t2)
{
    TimeStamp t = t1;
    return t -= t2;
}

typedef TimeStamp TimeInterval;

class DateTime
{
public:
    enum _eFormat {
        FORMAT_ANSI,
        FORMAT_W3C,
        FORMAT_RFC_1123,  // RFC 822 updated by RFC 1123
        FORMAT_RFC_1036   // RFC 850 updated by RFC 1036
    };
    enum _eFormat_flags {
        FLAG_EMIT_FRACTION      = 1,
        FLAG_EXTENDED_PRECISION = 2
    };

    YETI_Int32 get_local_timezone();

    DateTime();
    DateTime(const TimeStamp & timestamp, bool local = false);

    YETI_Result change_timezone(YETI_Int32 timezone);
    YETI_Result from_timestamp(const TimeStamp & timestamp, bool local = false);
    YETI_Result to_timestamp(TimeStamp & timestamp) const;
    YETI_Result from_string(const char * date, _eFormat format = FORMAT_ANSI);
    String      to_string(_eFormat format = FORMAT_ANSI, YETI_Flags flags = 0) const;

    YETI_Int32 m_year_;          // year
    YETI_Int32 m_month_;         // month of the year (1-12)
    YETI_Int32 m_day_;           // day of the month (1-31)
    YETI_Int32 m_hours_;         // hours (0-23)
    YETI_Int32 m_minutes_;       // minutes (0-59)
    YETI_Int32 m_seconds_;       // seconds (0-59)
    YETI_Int32 m_nanoseconds_;   // nanoseconds (0-999999999)
    YETI_Int32 m_timezone_;      // minutes offset from GMT
};

NAMEEND

#endif // _CXL_YETI_TIME_H_
