#ifndef _CXL_YETI_WIN32_THREAD_H_
#define _CXL_YETI_WIN32_THREAD_H_

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiThreads.h"
#include "YetiDebug.h"

#if defined(_XBOX)
#include <xtl.h>
#else
#include <windows.h>
#if !defined(_WIN32_WCE)
#include <process.h>
#endif
#endif

NAMEBEG

class Win32Mutex: public MutexInterface
{
public:
    Win32Mutex();
    virtual ~Win32Mutex();

    virtual YETI_Result lock();
    virtual YETI_Result unlock();

private:
    HANDLE m_handle_;
};

class Win32Event
{
public:
    Win32Event(bool manual = false, bool initial = false);
    virtual ~Win32Event();

    virtual YETI_Result wait(YETI_Timeout timeout = YETI_TIMEOUT_INFINITE);
    virtual void signal();
    virtual void reset();

private:
    HANDLE m_event_;
};

class Win32CriticalSection
{
public:
    Win32CriticalSection();
    ~Win32CriticalSection();

    YETI_Result lock();
    YETI_Result unlock();

private:
    CRITICAL_SECTION m_critical_section_;
};

NAMEEND

#endif // _CXL_YETI_WIN32_THREAD_H_
