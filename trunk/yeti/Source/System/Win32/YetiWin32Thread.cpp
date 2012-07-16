#include "YetiWin32Thread.h"

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiConstants.h"
#include "YetiThreads.h"
#include "YetiDebug.h"
#include "YetiResults.h"
#include "YetiTime.h"
#include "YetiSystem.h"

//YETI_SET_LOCAL_LOGGER("neptune.threads.win32")

#if defined(_WIN32_WCE) || defined(_XBOX)
#define YETI_WIN32_USE_CREATE_THREAD
#endif

#if defined(YETI_WIN32_USE_CREATE_THREAD)
#define _beginthreadex(security, stack_size, start_proc, arg, flags, pid) \
    CreateThread(security, stack_size, (LPTHREAD_START_ROUTINE) start_proc,   \
    arg, flags, (LPDWORD)pid)
#define _endthreadex ExitThread
#endif

NAMEBEG

Win32Mutex::Win32Mutex()
{
    m_handle_ = CreateMutex(NULL, FALSE, NULL);
}

Win32Mutex::~Win32Mutex()
{
    CloseHandle(m_handle_);
}

YETI_Result Win32Mutex::lock()
{
    DWORD result = WaitForSingleObject(m_handle_, INFINITE);
    if (result == WAIT_OBJECT_0) {
        return YETI_SUCCESS;
    }

    return YETI_FAILURE;
}

YETI_Result Win32Mutex::unlock()
{
    ReleaseMutex(m_handle_);
    return YETI_SUCCESS;
}

Mutex::Mutex()
{
    m_delegate_ = new Win32Mutex();
}

Win32CriticalSection::Win32CriticalSection()
{
    InitializeCriticalSection(&m_critical_section_);
}

Win32CriticalSection::~Win32CriticalSection()
{
    DeleteCriticalSection(&m_critical_section_);
}

YETI_Result Win32CriticalSection::lock()
{
    EnterCriticalSection(&m_critical_section_);
    return YETI_SUCCESS;
}

YETI_Result Win32CriticalSection::unlock()
{
    LeaveCriticalSection(&m_critical_section_);
    return YETI_SUCCESS;
}

Win32Event::Win32Event(bool manual /* = false */, bool initial /* = false */)
{
    m_event_ = CreateEvent(NULL, (manual == true) ? TRUE : FALSE, (initial == true) ? TRUE: FALSE, NULL);
}

Win32Event::~Win32Event()
{
    CloseHandle(m_event_);
}

YETI_Result Win32Event::wait(YETI_Timeout timeout /* = YETI_TIMEOUT_INFINITE */)
{
    if (m_event_) {
        DWORD result = WaitForSingleObject(m_event_, (timeout == YETI_TIMEOUT_INFINITE) ? INFINITE : timeout);
        if (result == WAIT_TIMEOUT) {
            return YETI_ERROR_TIMEOUT;
        }
        if (result != WAIT_OBJECT_0 && result != WAIT_ABANDONED) {
            return YETI_FAILURE;
        }
    }
    return YETI_SUCCESS;
}

void Win32Event::signal()
{
    SetEvent(m_event_);
}

void Win32Event::reset()
{
    ResetEvent(m_event_);
}

class Win32SharedVariable : public SharedVariable
{
public:
    Win32SharedVariable(int value);
    ~Win32SharedVariable() {}

    void set_value(int value);
    int get_value();
    YETI_Result wait_until_equals(int value, YETI_Timeout timeout = YETI_TIMEOUT_INFINITE);
    YETI_Result wait_while_equals(int value, YETI_Timeout timeout = YETI_TIMEOUT_INFINITE);
private:
    volatile int m_value_;
    Mutex m_lock_;
    Win32Event m_event_;
};

Win32SharedVariable::Win32SharedVariable(int value)
: m_value_(value) {}

void Win32SharedVariable::set_value(int value)
{
    m_lock_.lock();
    if (value != m_value_) {
        m_value_ = value;
        m_event_.signal();
    }
    m_lock_.unlock();
}

int Win32SharedVariable::get_value()
{
    return m_value_;
}

YETI_Result Win32SharedVariable::wait_until_equals(int value, YETI_Timeout timeout /* = YETI_TIMEOUT_INFINITE */)
{
    do {
        m_lock_.lock();
        if (m_value_ == value) {
            break;
        }
        m_lock_.unlock();
        {
            YETI_Result result = m_event_.wait(timeout);
            if (YETI_FAILED(result)) return result;
        }
    } while (1);
    m_lock_.unlock();
    return YETI_SUCCESS;
}

YETI_Result Win32SharedVariable::wait_while_equals(int value, YETI_Timeout timeout /* = YETI_TIMEOUT_INFINITE */)
{
    do {
        m_lock_.lock();
        if (m_value_ != value) {
            break;
        }
        m_lock_.unlock();
        {
            YETI_Result result = m_event_.wait(timeout);
            if (YETI_FAILED(result)) return result;
        }
    } while(1);
    m_lock_.unlock();
    return YETI_SUCCESS;
}

SharedVariable::SharedVariable(int value)
{
    m_delegate_ = new Win32SharedVariable(value);
}

class Win32AtomicVariable : public AtomicVariableInterface
{
public:
    Win32AtomicVariable(int value) : m_value_(value) {}
    ~Win32AtomicVariable() {}

    int increment()
    {
        return InterlockedIncrement(const_cast<LONG *>(&m_value_));
    }

    int decrement()
    {
        return InterlockedDecrement(const_cast<LONG *>(&m_value_));
    }

    void set_value(int value)
    {
        m_value_ = value;
    }

    int get_value()
    {
        return m_value_;
    }

private:
    volatile LONG m_value_;
};

AtomicVariable::AtomicVariable(int value /* = 0 */)
{
    m_delegate_ = new Win32AtomicVariable(value);
}

class Win32Thread : public ThreadInterface
{
public:
    static YETI_Result set_thread_priority(HANDLE thread, int priority);
    static YETI_Result get_thread_priority(HANDLE thread, int& priority);

    Win32Thread(Thread * delegator,
        Runnable & target,
        bool detached)
        : m_delegator_(delegator)
        , m_target_(target)
        , m_detached_(detached)
        , m_thread_handle_(0) {}

    ~Win32Thread() {
        if (!m_detached_) {
            wait();
        }
        if (m_thread_handle_) {
            CloseHandle(m_thread_handle_);
        }
    }

    YETI_Result start();
    YETI_Result wait(YETI_Timeout timeout = YETI_TIMEOUT_INFINITE );
    YETI_Result get_priority(int & priority);
    YETI_Result set_priority(int priority);

private:
    static unsigned int __stdcall _entry_point(void * argument);
    void run();
    YETI_Result interrupt() { return YETI_ERROR_NOT_IMPLEMENTED; }

    Thread * m_delegator_;
    Runnable & m_target_;
    bool m_detached_;
    HANDLE m_thread_handle_;
};

YETI_Result Win32Thread::set_thread_priority(HANDLE thread, int priority)
{
    int win32_priority;
    if (priority < YETI_THREAD_PRIORITY_LOWEST) {
        win32_priority = THREAD_PRIORITY_IDLE;
    } else if (priority < YETI_THREAD_PRIORITY_BELOW_NORMAL) {
        win32_priority = THREAD_PRIORITY_LOWEST;
    } else if (priority < YETI_THREAD_PRIORITY_NORMAL) {
        win32_priority = THREAD_PRIORITY_BELOW_NORMAL;
    } else if (priority < YETI_THREAD_PRIORITY_ABOVE_NORMAL) {
        win32_priority = THREAD_PRIORITY_NORMAL;
    } else if (priority < YETI_THREAD_PRIORITY_HIGHEST) {
        win32_priority = THREAD_PRIORITY_ABOVE_NORMAL;
    } else if (priority < YETI_THREAD_PRIORITY_TIME_CRITICAL) {
        win32_priority = THREAD_PRIORITY_HIGHEST;
    } else {
        win32_priority = THREAD_PRIORITY_TIME_CRITICAL;
    }

    BOOL result = ::SetThreadPriority(thread, win32_priority);
    if (!result) {
        // YETI_LOG_WARNING_1("SetThreadPriority failed (%x)", GetLastError());
        return YETI_FAILURE;
    }

    return YETI_SUCCESS;
}

YETI_Result Win32Thread::get_thread_priority(HANDLE thread, int& priority)
{
    int win32_priority = ::GetThreadPriority(thread);
    if (win32_priority == THREAD_PRIORITY_ERROR_RETURN) {
        //YETI_LOG_WARNING_1("GetThreadPriority (%x)", GetLastError());
        return YETI_FAILURE;
    }

    priority = win32_priority;
    return YETI_SUCCESS;
}

unsigned int __stdcall Win32Thread::_entry_point(void * argument)
{
    Win32Thread * thread = reinterpret_cast<Win32Thread *>(argument);

    //YETI_LOG_FINER("thread in ==============");
    
    TimeStamp now;
    System::get_current_timestamp(now);
    System::set_random_integer((YETI_UInt32)(now.to_nanos() + ::GetCurrentThreadId()));

    thread->run();

    if (thread->m_detached_) {
        delete thread->m_delegator_;
    }

    return 0;
}

YETI_Result Win32Thread::start()
{
    if (m_thread_handle_ > 0) {
        //YETI_LOG_WARNING("thread already started");
        return YETI_ERROR_INVALID_STATE;
    }

    //YETI_LOG_FINER("creating thread");

#if defined(_WIN32_WCE)
    DWORD thread_id;
#else
    unsigned int thread_id;
#endif

    bool detached = m_detached_;
    HANDLE thread_handle = (HANDLE)
        _beginthreadex(NULL, YETI_CONFIG_THREAD_STACK_SIZE,
        _entry_point,
        reinterpret_cast<void *>(this),
        0,
        &thread_id);

    if (thread_handle == 0) {
        return YETI_FAILURE;
    }

    if (detached) {
        CloseHandle(thread_handle);
    } else {
        m_thread_handle_ = thread_handle;
    }

    return YETI_SUCCESS;
}

void Win32Thread::run()
{
    m_target_.run();
}

YETI_Result Win32Thread::set_priority(int priority)
{
    if (m_thread_handle_ == 0) return YETI_ERROR_INVALID_STATE;
    return Win32Thread::set_thread_priority(m_thread_handle_, priority);
}

YETI_Result Win32Thread::get_priority(int & priority)
{
    if (m_thread_handle_ == 0) return YETI_ERROR_INVALID_STATE;
    return Win32Thread::get_thread_priority(m_thread_handle_, priority);
}

YETI_Result Win32Thread::wait(YETI_Timeout timeout)
{
    if (m_thread_handle_ == 0 || m_detached_) {
        return YETI_FAILURE;
    }

    DWORD result = WaitForSingleObject(m_thread_handle_, (timeout == YETI_TIMEOUT_INFINITE) ? INFINITE : timeout);
    if (result != WAIT_OBJECT_0) {
        return YETI_FAILURE;
    }
    return YETI_SUCCESS;
}

Thread::threadid Thread::get_current_thread_id()
{
    return ::GetCurrentThreadId();
}

YETI_Result Thread::set_current_thread_priority(int priority)
{
    return Win32Thread::set_thread_priority(::GetCurrentThread(), priority);
}

Thread::Thread(bool detached /* = false */)
{
    m_delegate_ = new Win32Thread(this, *this, detached);
}

Thread::Thread(Runnable & target, bool detached /* = false */)
{
    m_delegate_ = new Win32Thread(this, target, detached);
}

NAMEEND
