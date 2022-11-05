#pragma once
#include<vector>
#include<memory>
#include "./error.h"
template<typename T>
class Vec{
public:
    Vec() = default;


    T* begin()
    {
        return _items[0];
    }
    T* end()
    {
        return _items[_items.size() - 1];
    }

    template<typename U>
    void push(U item){
        static_assert(std::is_base_of<T, U>(), "Not a subtype??");
        auto alloc = new U(item);
        
        _items.push_back(alloc);
    }

    T& pop(){
        auto last = _items[_items.size() - 1];
        _items.pop_back();
        return *last;
    }
    std::size_t len(){
        return _items.size();
    }
    T& operator[](size_t idx){
        if( idx > len()-1 )except(E_INTERNAL, "IndexError: Bad Vec Access Index");
        return *_items[idx];
    }
private:
    std::vector<T*> _items;
    bool delete_members = true;
};