#ifndef _CXL_YETI_LIST_H_
#define _CXL_YETI_LIST_H_

#include "YetiResults.h"
#include "YetiTypes.h"
#include "YetiConstants.h"
#include "YetiCommon.h"

const int YETI_ERROR_LIST_EMPTY                 = YETI_ERROR_BASE_LIST - 0;
const int YETI_ERROR_LIST_OPERATION_ABORTED     = YETI_ERROR_BASE_LIST - 1;
const int YETI_ERROR_LIST_OPERATION_CONTINUE    = YETI_ERROR_BASE_LIST - 2;

NAMEBEG

template < typename T >
class List
{
protected:
    class Item;

public:
    typedef T element;

    class iterator
    {
    public:
        iterator() : m_item_(NULL) {}
        explicit iterator(Item * item) : m_item_(item) {}
        iterator(const iterator & copy) : m_item_(copy.m_item_) {}

        T & operator*() const { return m_item_->m_data_; }
        T * operator->() const {return &m_item_->m_data_; }

        iterator & operator++() {
            m_item_ = m_item_->m_next_;
            return (*this);
        }

        iterator & operator++(int) {
            iterator saved_this = *this;
            m_item_ = m_item_->m_next_;
            return saved_this;
        }

        iterator operator--() {
            m_item_ = m_item_->m_prev_;
            return *this;
        }

        iterator operator--(int) {
            iterator saved_this = *this;
            m_item_ = m_item_->m_prev_;
            return saved_this;
        }

        operator bool() const {
            return m_item_ != NULL;
        }

        bool operator==(const iterator & other) const {
            return m_item_ == other.m_item_;
        }

        bool operator!=(const iterator & other) const {
            return  m_item_ != other->m_item_;
        }

        void operator=(Item * item) {
            m_item_ = item;
        }

    private:
        Item * m_item_;

        friend class List<T>;
    };

    List<T>();
    List<T>(const List<T> & list);
    ~List<T>();

    YETI_Result add(const T & data);
    YETI_Result insert(const iterator where, const T & data);
    YETI_Result remove(const T & data, bool all = false);
    YETI_Result erase(const iterator position);
    YETI_Result pop_head(T & data);
    bool        contains(const T & data) const;
    YETI_Result clear();
    YETI_Result get(YETI_Ordinal index, T & data) const;
    YETI_Result get(YETI_Ordinal index, T *& data) const;
    YETI_Result get_item_count() const { return m_item_count_; }
    iterator    get_first_item() const { return iterator(m_head_); }
    iterator    get_last_item() const { return iterator(m_tail_); }
    iterator    get_item(YETI_Ordinal index) const;

    YETI_Result add(List<T> & list);
    YETI_Result remove(const List<T> & list, bool all = false);
    YETI_Result cut(YETI_Cardinal keep, List<T> & cut);

    YETI_Result add(Item & item);
    YETI_Result detach(Item & item);
    YETI_Result insert(const iterator where, Item & item);

    template < typename X >
    YETI_Result apply(const X & function) const
    {
        Item * item = m_head_;
        while (item) {
            function(item->m_data_);
            item = item->m_next_;
        }
    }

    template < typename X, typename P >
    YETI_Result apply_until(const X & function, const P & predicate, bool * match = NULL) const
    {
        Item * item = m_head_;
        while (item) {
            YETI_Result ret_value;
            if (predicate(function(item->m_data_),ret_value)) {
                if(match) *match = true;
                return ret_value;
            }
            item = item->m_next_;
        }

        if (match) *match = false;
        return YETI_SUCCESS;
    }

    template < typename P >
    iterator find(const P & predicate, YETI_Ordinal n = 0) const
    {
        Item * item = m_head_;
        while (item) {
            if (predicate(item->m_data_)) {
                if (n == 0) {
                    return iterator(item);
                }
                --n;
            }
            item = item->m_next_;
        }

        return iterator(NULL);
    }

    // 归并排序
    // http://en.wikipedia.org/wiki/Mergesort
    template < typename X >
    YETI_Result sort(const X & function)
    {
        if (get_item_count() <= 1) return YETI_SUCCESS;

        List<T> right;
        YETI_CHECK(cut(get_item_count() >> 1, right));

        // Sort ourselves again
        sort(function);

        // sort the right side
        right.sort(function);

        // merge the two back inline
        if (function(m_tail_->m_data_, right.m_head_->m_data_) > 0) {
            merge(right, function);
        } else {
            // append right
            right.m_head_->m_prev_ = m_tail_;
            if (m_tail_) m_tail_->m_next_ = right.m_head_;
            if (!m_head_) m_head_ = right.m_head_;
            m_tail_ = right.m_tail_;
            m_item_count_ += right.m_item_count_;

            right.m_item_count_ = 0;
            right.m_head_ = right.m_tail_ = NULL;
        }

        return YETI_SUCCESS;
    }

    template < typename X >
    YETI_Result merge(List<T> & other, const X & function)
    {
        iterator left = get_first_item();
        iterator right;

        while (left && other.m_head_) {
            if (function(*left, other.m_head_->m_data_) <= 0) {
                ++left;
            } else {
                // remove head and insert it
                Item * head = other.m_head_;
                other.detach(*head);
                insert(left, *head);
            }
        }

        // add what's left of other if any
        if (other.m_head_) {
            other.m_head_->m_prev_ = m_tail_;
            if (m_tail_) m_tail_.m_next_ = other.m_head_;
            m_tail_ = other.m_tail_;
            if (!m_head_) m_head_ = other.m_head_;
            other.m_head_ = other.m_tail_ = NULL;
        }
        m_item_count_ += other.m_item_count_;
        other.m_item_count_ = 0;

        return YETI_SUCCESS;
    }

    void operator=(const List<T> & other);
    bool operator==(const List<T> & other) const;
    bool operator!=(const List<T> & other) const;

protected:
    class Item
    {
    public:
        Item(const T & data) : m_next_(0), m_prev_(0), m_data_(data) {}

        Item * m_next_;
        Item * m_prev_;
        T      m_data_;
    };

    YETI_Cardinal   m_item_count_;
    Item *          m_head_;
    Item *          m_tail_;
};

// 构造
template < typename T >
inline List<T>::List() : m_item_count_(0), m_head_(0), m_tail_(0)
{
}

// 复制构造
template < typename T >
inline List<T>::List(const List<T> & list) : m_item_count_(0), m_head_(0), m_tail_(0)
{
    *this = list;
}

// 析构
template < typename T >
inline List<T>::~List()
{
    clear();
}

// 重载=
template < typename T >
void List<T>::operator =(const List<T> & list)
{
    clear();
    // copy the new list
    Item * item = list.m_head_;
    while (item) {
        add(item->m_data_);
        item = item->m_next_;
    }
}

// 重载==
template < typename T >
bool List<T>::operator ==(const List<T> & other) const
{
    if (m_item_count_ != other.m_item_count_) return false;

    // compare all elements one by one
    Item * out_item = m_head_;
    Item * other_item = other.m_head_;
    while (out_item && other_item) {
        if (out_item->m_data_ != other_item->m_data_) return false;
        out_item = out_item->m_next_;
        other_item = other_item->m_next_;
    }

    return out_item == NULL && other_item == NULL;
}

// 重载!=
template < typename T >
bool List<T>::operator !=(const List<T> & other) const
{
    return !(*this == other);
}

template < typename T >
YETI_Result List<T>::clear()
{
    // delete all items
    Item * item = m_head_;
    while (item) {
        Item * next = item->m_next_;
        delete item;
        item = next;
    }

    m_item_count_ = 0;
    m_head_       = NULL;
    m_tail_       = NULL;

    return YETI_SUCCESS;
}

template < typename T >
YETI_Result List<T>::add(Item & item)
{
    // add element at the tail
    if (m_tail_) {
        item.m_prev_ = m_tail_;
        item.m_next_ = NULL;
        m_tail_->m_next_ = &item;
        m_tail_ = &item;
    } else {
        m_head_ = &item;
        m_tail_ = &item;
        item.m_next_ = NULL;
        item.m_prev_ = NULL;
    }

    // one more item in the list now
    ++m_item_count_;

    return YETI_SUCCESS;
}

template < typename T >
YETI_Result List<T>::add(List<T> & list)
{
    // copy the new list
    Item * item = list.m_head_;
    while (item) {
        add(item->m_data_);
        item = item->m_next_;
    }

    return YETI_SUCCESS;
}

template < typename T >
inline YETI_Result List<T>::add(const T & data)
{
    return add(*new Item(data));
}

template < typename T >
typename List<T>::iterator List<T>::get_item(YETI_Ordinal n) const
{
    iterator result;
    if (n >= m_item_count_) return result;

    result = m_head_;
    for (unsigned int i = 0; i < n; ++i) {
        ++result;
    }

    return result;
}

template < typename T >
inline YETI_Result List<T>::insert(const iterator where, const T & data)
{
    return insert(where, *new Item(data));
}

template < typename T >
YETI_Result List<T>::insert(const iterator where, Item & item)
{
    // insert the item in the list
    Item * pos = where.m_item_;
    if (pos) {
        // insert at position
        item.m_next_ = pos;
        item.m_prev_ = pos->m_prev_;
        pos->m_prev_ = &item;
        if (item.m_prev_) {
            item.m_prev_->m_next_ = &item;
        } else {
            // this is the new head
            m_head_ = &item;
        }

        // one more item in the list now
        ++m_item_count_;
    } else {
        // insert at tail
        return add(item);
    }

    return YETI_SUCCESS;
}

template < typename T >
YETI_Result List<T>::erase(iterator position)
{
    if (!position) return YETI_ERROR_NO_SUCH_ITEM;
    detach(*position.m_item_);
    delete position.m_item_;

    return YETI_SUCCESS;
}

template < typename T >
YETI_Result List<T>::remove(const T & list, bool all /* = false */)
{
    Item * item = m_head_;
    YETI_Cardinal matches = 0;

    while (item) {
        Item * next = item->m_next_;
        if (item->m_data_ == data) {
            // we found a match
            ++matches;
            // detach item
            detach(*item);
            // destroy the item
            delete item;
            if (!all) return YETI_SUCCESS;
        }
        item = next;
    }

    return matches ? YETI_SUCCESS : YETI_ERROR_NO_SUCH_ITEM;
}

template < typename T >
YETI_Result List<T>::remove(const List<T> & list, bool all /* = false */)
{
    Item * item = list.m_head_;
    while (item) {
        remove(item->m_data_, all);
        item = item->m_next_;
    }

    return YETI_SUCCESS;
}

template < typename T >
YETI_Result List<T>::detach(Item & item)
{
    // remove item
    if (item.m_prev_) {
        // item is not the head
        if (item.m_next_) {
            // item is not the tail
            item.m_next_->m_prev_ = item.m_prev_;
            item.m_prev_->m_next_ = item.m_next_;
        } else {
            // item is the tail
            m_tail_ = item.m_prev_;
            m_tail_->m_next_ = NULL;
        }
    } else {
        // item is the head
        m_head_ = item.m_next_;
        if (m_head_) {
            // item is not the tail
            m_head_->m_prev_ = NULL;
        } else {
            // item is also the tail
            m_tail_ = NULL;
        }
    }

    --m_item_count_;

    return YETI_SUCCESS;
}

template < typename T >
YETI_Result List<T>::get(YETI_Ordinal index, T & data) const
{
    T * data_pointer;
    YETI_CHECK(get(index, data_pointer));
    data = *data_pointer;

    return YETI_SUCCESS;
}

template < typename T >
YETI_Result List<T>::get(YETI_Ordinal index, T *& data) const
{
    Item * item = m_head_;
    if (index < m_item_count_) {
        while (index--) item = item->m_next_;
        data = &item->m_data_;
        return YETI_SUCCESS;
    } else {
        data = NULL;
        return YETI_ERROR_NO_SUCH_ITEM;
    }
}

template < typename T >
YETI_Result List<T>::pop_head(T & data)
{
    // check that we have an element
    if (m_head_ == NULL) return YETI_ERROR_LIST_EMPTY;

    // copy the head item's data
    data = m_head_->m_data_;

    // discard the head item
    Item * head = m_head_;
    m_head_ = m_head_->m_next_;
    if (m_head_) {
        m_head_->m_prev_ = NULL;
    } else {
        m_tail_ = NULL;
    }
    delete head;

    // update the count
    --m_item_count_;

    return YETI_SUCCESS;
}

template < typename T >
bool List<T>::contains(const T & data) const
{
    Item * item = m_head_;
    while (item) {
        if (item->m_data_ == data) return true;
        item = item->m_next_;
    }

    return false;
}

template < typename T >
YETI_Result List<T>::cut(YETI_Cardinal keep, List<T> & cut)
{
    cut.clear();

    // shortcut
    if (keep >= get_item_count()) return YETI_SUCCESS;

    // update new counts first
    cut.m_item_count_ = m_item_count_ - keep;
    m_item_count_ = keep;

    // look for the cut-point item
    Item * item =m_head_;
    while (keep--) item = item->m_next_;

    // the cut list goes from the cut-point item to the tail
    cut.m_head_ = item;
    cut.m_tail_ = m_tail_;

    // update the portion of the list we keep
    if (item == m_head_) m_head_ = NULL;
    m_tail_ = item->m_prev_;

    // update the cut list
    if (item->m_prev_) item->m_prev_->m_next_ = NULL;
    item->m_prev_ = NULL;

    return YETI_SUCCESS;
}

NAMEEND

#endif // _CXL_YETI_LIST_H_
