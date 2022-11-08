#pragma once
#include<vector>
#include<memory>
#include "./error.h"
template<typename T>
class Vec{
public:
    Vec() = default;
    T** begin()
    {
        return &_items[0];
    }
    T** end()
    {
        return &_items[_items.size()];
    }

    template<typename U>
    void push(U item){
        static_assert(std::is_base_of<T, U>(), "Not a subtype??");
        auto alloc = new U(std::move(item));
        
        _items.push_back(alloc);
    }
    template<typename U>
    void push(std::unique_ptr<U> item){
        static_assert(std::is_base_of<T, U>(), "Not a subtype??");

        // Manage the memory myself
        auto mem = item.get();
        item.release();
        _items.push_back(mem);
    }

    template<typename U>
    void pushf(U& item){
        static_assert(std::is_base_of<T, U>(), "Not a subtype??");

        // Manage the memory myself
        auto mem = &item;
        _items.insert(_items.begin(), mem);
    }

    bool includes(T* check){
        for(auto item : _items){
            if( item == check )return true;
        }
        return false;
    }

    size_t indexof(T* item){
        size_t i = 0;
        for(auto it : _items){
            if(it == item)break;
            i++;
        }
        if(i == _items.size())return -1;
        return i;
    }

    T& pop(){
        auto last = _items[_items.size() - 1];
        _items.pop_back();
        return *last;
    }
    std::size_t len(){
        return _items.size();
    }
    void remove(size_t idx){
        delete _items[idx];
        _items.erase(_items.begin() + idx);
    }
    T& operator[](size_t idx){
        if( idx > len()-1 )except(E_INTERNAL, "IndexError: Bad Vec Access Index");
        return *_items[idx];
    }


    void set(size_t idx, T& item){
        _items[idx] = &item;
    }
private:
    std::vector<T*> _items;
    bool delete_members = true;
};