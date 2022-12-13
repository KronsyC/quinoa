#pragma once

#include<vector>
#include<memory>
#include "./error.h"

template<typename T>
class Vec {
public:
    Vec() = default;

    auto begin() {
        return _items.begin();
    }

    auto end() {
        return _items.end();
    }

    Vec(Vec &) = delete;


    Vec &operator=(Vec &&tg) = default;

    void push(std::unique_ptr <T> item) {
        // static_assert(std::is_base_of<T, U>(), "Not a subtype??");

        // Manage the memory myself
        auto mem = item.release();
        VecItem i;
        i.ptr = mem;
        i.owned = true;
        _items.push_back(i);
    }

    void push(std::shared_ptr <T> item) {
        // static_assert(std::is_base_of<T, U>(), "Not a subtype??");

        // Manage the memory myself
        auto mem = item.get();
        VecItem i;
        i.ptr = mem;
        i.owned = false;
        _items.push_back(i);
    }

    template<typename U>
    void push(U item) {
        static_assert(std::is_base_of<T, U>() || std::is_same<T, U>(), "Incompatible Types");
        auto alloc = new U(std::move(item));
        VecItem i;
        i.ptr = alloc;
        i.owned = true;
        _items.push_back(i);
    }

    template<typename U>
    void pushf(U &item) {
        static_assert(std::is_base_of<T, U>(), "Not a subtype??");

        // Manage the memory myself
        VecItem i;
        i.ptr = static_cast<T *>(&item);
        i.owned = true;
        _items.insert(_items.begin(), i);
    }

    bool includes(T *check) {
        for (auto item: _items) {
            if (item.ptr == check)return true;
        }
        return false;
    }

    size_t indexof(T *item) {
        size_t i = 0;
        for (auto it: _items) {
            if (it.ptr == item)break;
            i++;
        }
        if (i == _items.size())return -1;
        return i;
    }

    T &pop() {
        auto last = _items[_items.size() - 1];
        _items.pop_back();
        return *last.ptr;
    }

    std::size_t len() {
        return _items.size();
    }

    void remove(size_t idx) {
        auto item = _items[idx];
        if (item.owned)delete item.ptr;
        _items.erase(_items.begin() + idx);
    }

    T &operator[](size_t idx) {
        if (idx > len() - 1)except(E_INTERNAL, "IndexError: Bad Vec Access Index");
        return *_items[idx].ptr;
    }

    std::vector<T *> release() {
        std::vector< T * > ret;
        for (auto i: _items) {
            ret.push_back(i.ptr);
        }
        _items.clear();
        return ret;
    }

    void set(size_t idx, T &item) {
        _items[idx].ptr = &item;
    }

    struct VecItem {
        bool owned = true;
        T *ptr;

        T *operator->() const {
            return ptr;
        }

        operator T *() const {
            return ptr;
        }

        T &operator*() const {
            return *ptr;
        }
    };

    void clear() {
        for (auto i: _items) {
            if (i.owned)delete i.ptr;
        }
        _items.clear();
    }

    Vec(Vec &&from) {
        this->_items = from._items;
        from._items.clear();
    }

    ~Vec() {
        // for(auto item : _items){
        //     if(item.owned)
        //         delete item.ptr;
        // }
        _items.clear();
    }

private:
    std::vector<VecItem> _items;


};