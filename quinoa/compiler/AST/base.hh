#pragma once
#include<vector>
#include<string>
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

class Identifier:public Expression{

};
class Ident:public Identifier{
public:
    std::string name;
    Ident(std::string name){
        this->name = name;
    }
    Ident() = default;
};
class CompoundIdentifier:public Ident{
public:
    std::vector<Identifier*> parts;
    CompoundIdentifier(std::vector<Identifier*> parts){
        this->parts = parts;
    }
    CompoundIdentifier() = default;
    void flatten(){
        std::vector<Identifier*> flattened;
        for(auto p:parts){
            if(instanceof<CompoundIdentifier>(p)){
                auto q = dynamic_cast<CompoundIdentifier*>(p);
                q->flatten();
                for(auto p:q->parts){
                    flattened.push_back(p);
                }
            }
            else{
                flattened.push_back(p);
            }
        }
        this->parts = flattened;
    }
};
