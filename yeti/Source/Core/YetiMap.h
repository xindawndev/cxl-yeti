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
        m_entries_.add(new Entry(key, value));
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

template < typename K, typename V >
const Map<K, V> & Map<K, V>::operator =(const Map<K, V> & copy)
{
    if (this == &copy) return copy;
    clear();
    listiterator entry = copy.m_entries_.get_first_item();
    while (entry) {
        m_entries_.add(new Entry((*entry)->get_key(), (*entry)->get_value()));
        ++entry;
    }
}

template < typename K, typename V >
YETI_Result Map<K, V>::erase(const K & key)
{
    listiterator entry = m_entries_.get_first_item();
    while (entry) {
        if ((*entry)->get_key() == key) {
            delete *entry;
            m_entries_.erase(entry);
            return YETI_SUCCESS;
        }
        ++entry;
    }

    return YETI_ERROR_NO_SUCH_ITEM;
}

template < typename K, typename V >
bool Map<K, V>::operator ==(const Map<K, V> & other) const
{
    if (m_entries_.get_item_count() != other.m_entries_.get_item_count()) return false;

    listiterator entry = m_entries_.get_first_item();
    while (entry) {
        V * value;
        if (YETI_SUCCEEDED(other.get((*entry)->m_key_, value))) {
            if (!(*value == (*entry)->m_value_)) return false;
        } else {
            return false;
        }
        ++entry;
    }

    return true;
}

template <typename K, typename V>
bool Map<K, V>::operator !=(const Map<K, V> & other) const
{
    return !(*this == other);
}

template <typename K, typename V>
V & Map<K, V>::operator [](const K & key)
{
    Entry * entry = _get_entry(key);
    if (entry == NULL) {
        entry = new Entry(key);
        m_entries_.add(entry);
    }

    return entry->m_value_;
}

template < typename K, typename V, typename HF = Hash<K> >
class HashMap
{
public:
    class Entry {
    public:
        Entry(YETI_UInt32 hash_value, const K & key, const V & value) : m_hash_value_(hash_value), m_key_(key), m_value_(value) {}
        Entry(YETI_UInt32 hash_value, const K & key)                  : m_hash_value_(hash_value), m_key_(key) {}

        const K & get_key()             const { return m_key_;          }
        const V & get_value()           const { return m_value_;        }
        YETI_UInt32 get_hash_value()    const { return m_hash_value_;   }

        bool operator ==(const Entry & other) const {
            return m_hash_value_ == other.m_hash_value_ && m_key_ == other.m_key_ && m_value_ == other.m_value_;
        }

    protected:
        void _set_value(const V & value) { m_value_ = value; }

        YETI_UInt32 m_hash_value_;
        K           m_key_;
        V           m_value_;

        friend class HashMap<K, V, HF>;
    };

    class iterator {
    public:
        iterator() : m_entry_(NULL), m_map_(NULL) {}
        iterator(Entry ** entry, const HashMap<K, V, HF> * map) : m_entry_(entry), m_map_(map) {}
        iterator(const iterator & copy) : m_entry_(copy.m_entry_), m_map_(copy.m_map_) {}

        const Entry & operator*() const { return **m_entry_; }
        iterator & operator ++() {
            if (m_map_ && m_entry_) {
                do {
                    ++m_entry_;
                    if (m_entry_ >= &m_map_->m_buckets_[1 << m_map_->m_bucket_count_log_]) {
                        m_entry_ = NULL;
                    } else {
                        if (*m_entry_) break;
                    }
                } while(m_entry_);
            }

            return *this;
        }

        iterator operator ++(int) {
            iterator saved_this = *this;
            ++(*this);
            return saved_this;
        }

        operator bool() const {
            return m_entry_ != NULL;
        }

        bool operator ==(const iterator & other) const {
            return m_map_ == other.m_map_ && m_entry_ == other.m_entry_;
        }

        bool operator != (const iterator & other) const {
            return !(*this == other);
        }

        void operator =(const iterator & other) {
            m_entry_ = other.m_entry_;
            m_map_   = other.m_map_;
        }

    private:
        friend class HashMap<K, V, HF>;

        Entry **                    m_entry_;
        const HashMap<K, V, HF> *   m_map_;
    };

    HashMap<K, V, HF>();
    HashMap<K, V, HF>(const HF & hasher);
    HashMap<K, V, HF>(const HashMap<K, V, HF> & copy);
    ~HashMap<K, V, HF>();

    YETI_Result     put(const K & key, const V & value);
    YETI_Result     get(const K & key, V *& value) const;
    bool            has_key(const K & key) const { return _get_entry(key) != NULL; }
    bool            has_value(const V & value) const;
    YETI_Result     erase(const K & key);
    YETI_Cardinal   get_entry_count() const { return m_entry_count_; }
    iterator        get_entries() const;
    YETI_Result     clear();

    template < typename X >
    YETI_Result apply(const X & function) const
    {
        for (unsigned int i = 0; i < (m_bucket_count_log_ << 1); ++i) {
            if (m_buckets_[i]) {
                function(m_buckets_[i]);
            }
        }

        return YETI_SUCCESS;
    }

    V &                         operator [](const K & key);
    const HashMap<K, V, HF> &   operator =(const HashMap<K, V, HF> & copy);
    bool                        operator ==(const HashMap<K, V, HF> & other) const;
    bool                        operator !=(const HashMap<K, V, HF> & other) const;

private:
    Entry *     _get_entry(const K & key, YETI_UInt32 * position = NULL) const;
    YETI_Result _add_entry(Entry * entry);
    void        _allocate_buckets(unsigned int count_log);
    void        _adjust_buckets(YETI_Cardinal entry_count, bool allow_shrink = false);

    HF              m_hasher_;
    Entry **        m_buckets_;
    YETI_Cardinal   m_bucket_count_log_;
    YETI_Cardinal   m_entry_count_;
};

template <typename K, typename V, typename HF>
HashMap<K, V, HF>::HashMap()
: m_buckets_(NULL)
, m_entry_count_(0)
{
    _allocate_buckets(4);
}

template <typename K, typename V, typename HF>
HashMap<K, V, HF>::HashMap(const HF & hasher)
: m_hasher_(hasher)
, m_buckets_(NULL)
, m_entry_count_(0)
{
    _allocate_buckets(4);
}

template <typename K, typename V, typename HF>
HashMap<K, V, HF>::HashMap(const HashMap<K, V, HF> & copy)
: m_buckets_(NULL)
, m_bucket_count_log_(0)
, m_entry_count_(0)
{
    *this = copy;
}

template <typename K, typename V, typename HF>
HashMap<K, V, HF>::~HashMap()
{
    for (int i = 0; i < (1 << m_bucket_count_log_); ++i) {
        delete m_buckets_[i];
    }

    delete[] m_buckets_;
}

template <typename K, typename V, typename HF>
void HashMap<K, V, HF>::_allocate_buckets(unsigned int count_log)
{
    m_buckets_ = new Entry*[1 << count_log];
    m_bucket_count_log_ = count_log;
    for (int i = 0; i < (1 << count_log); ++i) {
        m_buckets_[i] = NULL;
    }
}

template <typename K, typename V, typename HF>
void HashMap<K, V, HF>::_adjust_buckets(YETI_Cardinal entry_count, bool allow_shrink /* = false */)
{
    Entry ** buckets = NULL;
    unsigned init  bucket_count = 1 << m_bucket_count_log_;
    if (2 *  entry_count >= bucket_count) {
        buckets = m_buckets_;
        _allocate_buckets(m_bucket_count_log_ + 1);
    } else if (allow_shrink && (5 * entry_count < bucket_count) && m_bucket_count_log_ > 4) {
        buckets = m_buckets_;
        _allocate_buckets(m_entry_count_ - 1);
    }

    if (buckets) {
        m_entry_count_ = 0;
        for (unsigned int i = 0; i < bucket_count; ++i) {
            if (buckets[i]) _add_entry(buckets[i]);
        }

        delete[] buckets;
    }
}

template <typename K, typename V, typename HF>
YETI_Result HashMap<K, V, HF>::clear()
{
    if (m_buckets_) {
        for (int i = 0; i < (1 << m_bucket_count_log_); ++i) {
            delete m_buckets_[i];
        }
        delete[] m_buckets_;
    }

    m_entry_count_ = 0;
    _allocate_buckets(4);

    return YETI_SUCCESS;
}

template <typename K, typename V, typename HF>
typename HashMap<K, V, HF>::iterator HashMap<K, V, HF>::get_entries() const
{
    for (int i = 0; i < (1 << m_bucket_count_log_); ++i) {
        if (m_buckets_[i]) {
            return iterator(&m_buckets_[i], this);
        }
    }

    return iterator(NULL, this);
}

template <typename K, typename V, typename HF>
typename HashMap<K, V, HF>::Entry * HashMap<K, V, HF>::_get_entry(const K & key, YETI_UInt32 * position /* = NULL */) const
{
    YETI_UInt32 hash_value  = m_hasher_(key);
    YETI_UInt32 mask        = (1 << m_bucket_count_log_) - 1;
    YETI_UInt32 cursor      = hash_value & mask;

    while (m_buckets_[cursor]) {
        Entry * entry = m_buckets_[cursor];
        if (entry->m_hash_value_ == hash_value &&
            entry->m_key_        == key) {
                if (position) *position = cursor;
                return entry;
        }
        cursor = (cursor + 1) & mask;
    }

    return NULL;
}

template <typename K, typename V, typename HF>
YETI_Result HashMap<K, V, HF>::_add_entry(Entry * entry)
{
    _adjust_buckets(m_entry_count_ + 1);

    YETI_UInt32 hash_value  = entry->m_hash_value_;
    YETI_UInt32 mask        = (1 << m_bucket_count_log_) - 1;
    YETI_UInt32 cursor      = hash_value & mask;
    while (m_buckets_[cursor]) {
        cursor = (cursor + 1) & mask;
    }

    m_buckets_[cursor] = entry;
    ++m_entry_count_;
    
    return YETI_SUCCESS;
}

template <typename K, typename V, typename HF>
YETI_Result HashMap<K, V, HF>::put(const K & key, const V & value)
{
    Entry * entry = _get_entry(key);
    if (entry == NULL) {
        return _add_entry(new Entry(m_hasher_(key), key, value));
    } else {
        entry->_set_value(value);
    }

    return YETI_SUCCESS;
}

template <typename K, typename V, typename HF>
YETI_Result HashMap<K, V, HF>::get(const K & key, V *& value) const
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

template <typename K, typename V, typename HF>
bool HashMap<K, V, HF>::has_value(const V & value) const
{
    for (int i = 0; i < (1 << m_bucket_count_log_); ++i) {
        if (m_buckets_[i] && m_buckets_[i]->m_value_ == value) {
            return true;
        }
    }

    return false;
}

template <typename K, typename V, typename HF>
YETI_Result HashMap<K, V, HF>::erase(const K & key)
{
    YETI_UInt32 position;
    Entry * entry = _get_entry(key, &position);
    if (entry == NULL) {
        return YETI_ERROR_NO_SUCH_ITEM;
    }

    m_buckets_[position] = NULL;

    YETI_UInt32 mask = (1 << m_bucket_count_log_) - 1;
    for (YETI_UInt32 cursor = (position + 1) & mask; m_buckets_[cursor]; cursor = (cursor + 1) & mask) {
        YETI_UInt32 target = m_buckets_[cursor]->m_hash_value_ & mask;
        if ((position <= cursor) ?
            ((position < target) && (target <= cursor)) :
        ((position < target || (target <= cursor)))) {
            continue;
        }

        m_buckets_[position] = m_buckets_[cursor];
        m_buckets_[cursor] = NULL;
        position = cursor;
    }

    delete entry;
    --m_entry_count_;
    _adjust_buckets(m_entry_count_, true);

    return YETI_SUCCESS;
}

template <typename K, typename V, typename HF>
const HashMap<K, V, HF> & HashMap<K, V, HF>::operator =(const HashMap<K, V, HF> & copy)
{
    if (this == &copy) return copy;
    clear();
    _adjust_buckets(copy.m_entry_count_);

    for (int i = 0; i < (1 << copy.m_bucket_count_log_); ++i) {
        if (copy.m_buckets_[i]) {
            _add_entry(new Entry(m_hasher_(copy.m_buckets_[i]->get_key()), copy.m_buckets_[i]->get_key(), copy.m_buckets_[i]->get_value()));
        }
    }

    return *this;
}

template <typename K, typename V, typename HF>
bool HashMap<K, V, HF>::operator ==(const HashMap<K, V, HF> & other) const
{
    if (m_entry_count_ != other.m_entry_count_) return false;

    for (int i = 0; i < (1 << m_bucket_count_log_); ++i) {
        Entry * entry = m_buckets_[i];
        if (entry == NULL) continue;
        Entry * other_entry = other._get_entry(entry->m_key_);
        if (other_entry == NULL || !(other_entry->m_value_ == entry->m_value_)) {
            return false;
        }
    }

    return true;
}

template <typename K, typename V, typename HF>
bool HashMap<K, V, HF>::operator !=(const HashMap<K, V, HF> & other) const
{
    return !(*this == other);
}

template <typename K, typename V, typename HF>
V & HashMap<K, V, HF>::operator [](const K & key)
{
    Entry * entry = _get_entry(key);
    if (entry == NULL) {
        entry = new Entry(m_hasher_(key), key);
        _add_entry(entry);
    }

    return entry->m_value_;
}

template < class T >
class MapEntryValueDeleter {
public:
    void operator()(T * entry) const {
        delete entry->get_value();
    }
};

NAMEEND

#endif // _CXL_YETI_MAP_H_
