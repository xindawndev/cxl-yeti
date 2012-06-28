#ifndef _CXL_YETI_SYSTEM_H_
#define _CXL_YETI_SYSTEM_H_

#include "YetiTypes.h"
#include "YetiTime.h"

NAMEBEG

class System {
public:
    static YETI_Result get_process_id(YETI_UInt32 & id);
    static YETI_Result get_machine_name(String & name);
    static YETI_Result get_current_timestamp(TimeStamp & now);
    static YETI_Result sleep(const TimeInterval & duration);
    static YETI_Result sleep_until(const TimeStamp & when);
    static YETI_Result set_random_integer(unsigned int seed);
    static YETI_UInt32 get_random_integer();

protected: 
    System() {}
};

YETI_Result get_system_machine_name(String & name);

NAMEEND

#endif // _CXL_YETI_SYSTEM_H_
