#pragma once
template<typename T>
class Vec{
public:
    Vec() = default;
    Vec(const Vec&) = delete;

    Vec(const Vec&& donor){
        delete_members = false;
    }
    ~Vec(){
        if(!delete_members)return;
        for(auto item: _items){
            delete item;
        }
    }
    std::size_t len(){
        return _items->size();
    }
    template<typename U>
    void push(U item){
        auto alloc = new U(item);
        _items.push_back(alloc);
    }

    T& pop(){
        T* last = _items[_items.size() - 1];
        _items.pop_back();
        return *last;
    }
    

private:
    std::vector<T*> _items;
    bool delete_members = true;
};