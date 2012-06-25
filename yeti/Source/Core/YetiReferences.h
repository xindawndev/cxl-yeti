#ifndef _CXL_YETI_REFERENCES_H_
#define _CXL_YETI_REFERENCES_H_

#include "YetiConstants.h"
#include "YetiThreads.h"

NAMEBEG

template < typename T >
class Reference
{
public:
    Reference() : m_object_(NULL), m_counter_(NULL), m_mutex_(NULL) {}
    explicit Reference(T * object, bool thread_safe = true)
        : m_object_(object)
        , m_counter_(object ? new YETI_Cardinal(1) : NULL)
        , m_mutex_(thread_safe ? new Mutex() : NULL) {}
    Reference(const Reference<T> & ref)
        : m_object_(ref.m_object_)
        , m_counter_(ref.m_counter_)
        , m_mutex_(ref.m_mutex_)
    {
        if (m_mutex_) m_mutex_->lock();
        if (m_counter_) ++(*m_counter_);
        if (m_mutex_) m_mutex_->unlock();
    }
    Reference(T * object, YETI_Cardinal * counter, Mutex * mutex)
        : m_object_(object)
        , m_counter_(counter)
        , m_mutex_(mutex)
    {
        if (m_mutex_) m_mutex_->lock();
        if (m_counter_) ++(*m_counter_);
        if (m_mutex_) m_mutex_->unlock();
    }

    ~Reference()
    {
        _release();
    }

    Reference<T> & operator=(const Reference<T> & ref)
    {
        if (this != &ref) {
            _release();
            m_object_ = ref.m_object_;
            m_counter_ = ref.m_counter_;
            m_mutex_ = ref.m_mutex_;

            if (m_mutex_) m_mutex_->lock();
            if (m_counter_) ++(*m_counter_);
            if (m_mutex_) m_mutex_->unlock();
        }
        return *this;
    }

    Reference<T> & operator =(T * object)
    {
        _release();
        m_object_ = object;
        m_counter_ = object ? new YETI_Cardinal(1) : NULL;
        m_mutex_ = NULL;
        return *this;
    }

    T & operator*() const { return *m_object_; }
    T * operator->() const { return m_object_; }

    bool operator==(const Reference<T> & ref) const 
    {
        return m_object_ == ref.m_object_;
    }
    bool operator!=(const Reference<T> & ref) const
    {
        return m_object_ != ref.m_object_;
    }

    template < typename U >
    operator Reference<U>()
    {
        return Reference<U>(m_object_, m_counter_, m_mutex_);
    }

    T * as_pointer() const { return m_object_; }

    YETI_Cardinal get_counter() const { return *m_counter_; }

    bool is_null() const { return m_object_ == NULL; }

    void detach()
    {
        _release(true);
    }

private:
    void _release(bool detach_only = false)
    {
        bool last_reference = false;
        if (m_mutex_) m_mutex_->lock();

        if (m_counter_ && --(*m_counter_) == 0) {
            delete m_counter_;
            if (!detach_only) delete m_object_;
            last_reference = true;
        }

        m_counter_ = NULL;
        m_object_ = NULL;

        if (m_mutex_) {
            Mutex * mutex = m_mutex_;
            if (last_reference) m_mutex_ = NULL;
            mutex->unlock();
            if (last_reference) delete mutex;
        }
    }
    T *             m_object_;
    YETI_Cardinal * m_counter_;
    Mutex *         m_mutex_;
};

NAMEEND

#endif // _CXL_YETI_REFERENCES_H_
