#pragma once
#include<vector>

class AstNode{
public:
    virtual ~AstNode() = default;
};

class Expression: public AstNode{};
template<typename T>
class Block: public AstNode{
public:
    std::vector<T*> items;
    size_t push(T& item){
        auto alloc = new T(item);
        items.push_back(alloc);
        return items.size();
    }
    ~Block(){
        for(auto item:items){
            delete item;
        }
    }
};