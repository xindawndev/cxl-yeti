#ifndef _CXL_YETI_SINGLETON_H_
#define _CXL_YETI_SINGLETON_H_

NAMEBEG

template < typename T >
class Singleton
{
private:
    Singleton(const Singleton< T > &);
    Singleton & operator=(const Singleton< T > &);

protected:
    static T * m_singleton_;

public:
    Singleton(void)
    {
        YETI_ASSERT( !m_singleton_ );
#if defined( _MSC_VER ) && _MSC_VER < 1200	 
        int offset = (int)(T*)1 - (int)(Singleton <T>*)(T*)1;
        m_singleton_ = (T*)((int)this + offset);
#else
        m_singleton_ = static_cast< T * >(this);
#endif
    }

    ~Singleton( void ) {
        YETI_ASSERT( m_singleton_ );
        m_singleton_ = 0;
    }

    static T & get_singleton(void) {
        assert( m_singleton_ );
        return ( *m_singleton_ );
    }

    static T * get_singleton_ptr(void) {
        return m_singleton_;
    }
};

NAMEEND

#endif // _CXL_YETI_SINGLETON_H_
