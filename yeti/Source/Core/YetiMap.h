#ifndef _CXL_YETI_MAP_H_
#define _CXL_YETI_MAP_H_

#include "YetiTypes.h"
#include "YetiResults.h"
#include "YetiList.h"
#include "YetiHash.h"

NAMEBEG

template < typename K, typename V >
class Map
{
public:
    class Entry{
    public:
        Entry(const K & key, const V & value) : m_key_(key), m_value_(value) {}
        Entry(const K & key) : m_key_(key) {}

        const K & get_key() const { return m_key_; }
        const V & get_value() const { return m_value_; }

        bool operator==(const Entry & other) const {
            return m_key_ == other.m_key_ && m_value_ == other.m_value_;
        }

    protected:
        void _set_value(const V & value) { m_value_ = value; }

        K m_key_;
        V m_value_;

        friend class Map<K, V>;
    };

    Map<K, V>() {}
    Map<K, V>(const Map<K, V> & copy);

    ~Map<K, V>();

    YETI_Result   put(const K & key, const V & value);
    YETI_Result   get(const K & key, V *& value) const;
    bool          has_key(const K & key) const { return _get_entry(key) != NULL; }
    bool          has_value(const V & value) const;
    YETI_Result   erase(const K & key);
    YETI_Cardinal get_entry_count() const { return m_entries_.get_item_count(); }
    const List<Entry *> get_entries() const { return m_entries_; }
    YETI_Result   clear();

    V &                 operator[](const K & key);
    const Map<K, V> &   operator=(const Map<K, V> & copy);
    bool                operator==(const Map<K, V> & other) const;
    bool                operator!=(const Map<K, V> & other) const;

private:
    typedef typename List<Entry *>::iterator listiterator;

    Entry * _get_entry(const K & key) const;

    List<Entry *> m_entries_;
};

template < typename K, typename V >
Map<K, V>::Map(const Map<K, V> & copy)
{
    *this = copy;
}

template < typename K, typename V >
Map<K, V>::~Map()
{
    clear();
}

template < typename K, typename V >
YETI_Result Map<K, V>::clear()
{
    m_entries_.apply(ObjectDeleter<Entry>());
    m_entries_.clear();

    return YETI_SUCCESS;
}

template < typename K, typename V >
typename Map<K, V>::Entry * Map<K, V>::_get_entry(const K & key) const
{
    typename List<Entry *>::iterator entry = m_entries_.get_first_item();
    while (entry) {
        if ((*entry)->get_key() == key) return *entry;
        ++entry;
    }

    return NULL;
}

template < typename K, typename V >
YETI_Result Map<K, V>::put(const K & key, const V & value)
{
    Entry * entry = _get_entry(key);
    if (entry == NULL) {
        m_entries_.add(new entry(key, value));
    } else {
        entry->_set_value(value);
    }

    return YETI_SUCCESS;
}

template < typename K, typename V >
YETI_Result Map<K, V>::get(const K & key, V *& value) const
{
    Entry * entry = _get_entry(key);
    if (entry == NULL) {
        value = NULL;
        return YETI_ERROR_NO_SUCH_ITEM;
    } else {
        value = &entry->m_value_;
        return YETI_SUCCESS;
    }
}

template < typename K, typename V >
bool Map<K, V>::has_value(const V & value) const
{
    listiterator entry = m_entries_.get_first_item();
    while (entry) {
        if (value == (*entry)->m_value_) return true;
        ++entry;
    }

    return false;
}

NAMEEND

#endif // _CXL_YETI_MAP_H_
