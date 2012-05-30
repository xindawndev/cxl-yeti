#ifndef _CXL_YETI_THREADS_H_
#define _CXL_YETI_THREADS_H_

#include "YetiTypes.h"
#include "YetiConstants.h"
#include "YetiInterfaces.h"

const int YETI_ERROR_CALLBACK_HANDLER_SHUTDOWN = YETI_ERROR_BASE_THREADS - 0;
const int YETI_ERROR_CALLBACK_NOTHING_PENDING  = YETI_ERROR_BASE_THREADS - 1;

NAMEBEG

const int YETI_THREAD_PRIORITY_MIN           = -15;
const int YETI_THREAD_PRIORITY_IDLE          = -15;
const int YETI_THREAD_PRIORITY_LOWEST        = -2;
const int YETI_THREAD_PRIORITY_BELOW_NORMAL  = -1;
const int YETI_THREAD_PRIORITY_NORMAL        = 0;
const int YETI_THREAD_PRIORITY_ABOVE_NORMAL  = 1;
const int YETI_THREAD_PRIORITY_HIGHEST       = 2;
const int YETI_THREAD_PRIORITY_TIME_CRITICAL = 15;
const int YETI_THREAD_PRIORITY_MAX           = 15;

class MutexInterface
{
public:
    virtual             ~MutexInterface() {}
    virtual YETI_Result lock()   = 0;
    virtual YETI_Result unlock() = 0;
};

class Mutex : public MutexInterface
{
public:
    Mutex();
    ~Mutex() { delete m_delegate_; }

    YETI_Result lock()      { return m_delegate_->lock();   }
    YETI_Result unlock()    { return m_delegate_->unlock(); }

private:
    MutexInterface * m_delegate_;
};

class AutoLock
{
public:
    AutoLock(Mutex & mutex) : m_mutex_(mutex) {
        m_mutex_.lock();
    }
    ~AutoLock() {
        m_mutex_.unlock();
    }

private:
    Mutex & m_mutex_;
};

template < typename T >
class Lock :public T, public Mutex
{
};

class SingletonLock
{
public:
    static Mutex & get_instance() {
        return instance_;
    }

private:
    static Mutex instance_;
};

class SharedVariableInterface
{
public:
    virtual             ~SharedVariableInterface() {}
    virtual void        set_value(int value) = 0;
    virtual int         get_value()          = 0;
    virtual YETI_Result wait_until_equals(int value, YETI_Timeout timeout = YETI_TIMEOUT_INFINITE) = 0;
    virtual YETI_Result wait_while_equals(int value, YETI_Timeout timeout = YETI_TIMEOUT_INFINITE) = 0;
};

class SharedVariable : public SharedVariableInterface
{
public:
    SharedVariable(int value = 0);
    ~SharedVariable() { delete m_delegate_; }

    void set_value(int value) {
        m_delegate_->set_value(value);
    }

    int get_value(/* = 0 */) {
        return m_delegate_->get_value();
    }

    YETI_Result wait_until_equals(int value, YETI_Timeout timeout = YETI_TIMEOUT_INFINITE) {
        return m_delegate_->wait_until_equals(value, timeout);
    }

    YETI_Result wait_while_equals(int value, YETI_Timeout timeout = YETI_TIMEOUT_INFINITE) {
        return m_delegate_->wait_while_equals(value, timeout);
    }

private:
    SharedVariableInterface * m_delegate_;
};

class AtomicVariableInterface
{
public:
    virtual         ~AtomicVariableInterface() {}
    virtual int     increment() = 0;
    virtual int     decrement() = 0;
    virtual int     get_value() = 0;
    virtual void    set_value(int value) = 0;
};

class AtomicVariable : public AtomicVariableInterface
{
public:
    AtomicVariable(int value = 0);
    ~AtomicVariable()   { delete m_delegate_; }

    int increment()             { return m_delegate_->increment(); }
    int decrement()             { return m_delegate_->decrement(); }
    void set_value(int value)   { m_delegate_->set_value(value);   }
    int get_value()             { return m_delegate_->get_value(); }

private:
    AtomicVariableInterface * m_delegate_;
};

class Runnable
{
public:
    ~Runnable() {}
    virtual void run() = 0;
};

class ThreadInterface : public Runnable, public Interruptible
{
public:
    virtual             ~ThreadInterface() {}
    virtual YETI_Result start() = 0;
    virtual YETI_Result wait(YETI_Timeout timeout = YETI_TIMEOUT_INFINITE) = 0;
    virtual YETI_Result set_priority(int /* priority */) { return YETI_SUCCESS; }
    virtual YETI_Result get_priority(int & priority) = 0;
};

class Thread : public ThreadInterface
{
public:
    typedef unsigned long threadid;

    static threadid get_current_thread_id();
    static YETI_Result set_current_thread_priority(int priority);
    static YETI_Result get_current_thread_priority(int & priority);

    explicit Thread(bool detached = false);
    explicit Thread(Runnable & target, bool detached = false);
    ~Thread() { delete m_delegate_; }

    YETI_Result start() {
        return m_delegate_->start();
    }

    YETI_Result wait(YETI_Timeout timeout = YETI_TIMEOUT_INFINITE ) {
        return m_delegate_->wait(timeout);
    }

    YETI_Result set_priority(int priority) {
        return m_delegate_->set_priority(priority);
    }

    YETI_Result get_priority(int & priority) {
        return m_delegate_->get_priority(priority);
    }

    virtual void run() {}

    virtual YETI_Result interrupt() { return m_delegate_->interrupt(); }

private:
    ThreadInterface * m_delegate_;
};

class ThreadCallbackReceiver
{
public:
    virtual ~ThreadCallbackReceiver() {}
    virtual void on_callback(void * args) = 0;
};

class ThreadCallbackSlot
{
public:
    class NotificationHelper {
    public:
        virtual ~NotificationHelper() {}
        virtual void notify(void) = 0;
    };

    ThreadCallbackSlot();

    YETI_Result receive_callback(ThreadCallbackReceiver & receiver, YETI_Timeout timeout = 0);
    YETI_Result send_callback(void * args);
    YETI_Result set_notification_helper(NotificationHelper * helper);
    YETI_Result shutdown();

protected:
    volatile void *         m_callback_args_;
    volatile bool           m_shutdown_;
    SharedVariable          m_pending_;
    SharedVariable          m_ack_;
    Mutex                   m_read_lock_;
    Mutex                   m_write_lock_;
    NotificationHelper *    m_notification_helper_;
};

NAMEEND

#endif // _CXL_YETI_THREADS_H_
