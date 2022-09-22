#pragma once
#include<vector>
#include<string>
class AstNode{
public:
    virtual ~AstNode() = default;
};



class Statement:public AstNode{

};
class Expression: public Statement{};
template<typename T>
class Block: public AstNode{
public:
    std::vector<T*> items;
    size_t push(T* item){
        items.push_back(item);
        return items.size();
    }
    ~Block(){
        for(auto item:items){
            delete item;
        }
    }
};

class TopLevelExpression:public AstNode{};
class ModuleMember:public AstNode{};
class CompilationUnit:public Block<TopLevelExpression>{

};
class Identifier:public Expression{
public:
    virtual std::string str(){
        return "";
    }

};
class Ident:public Identifier{
public:
    std::string name;
    Ident(std::string name){
        this->name = name;
    }
    Ident() = default;
    std::string str(){
        return this->name;
    }
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
    std::string str(){
        std::string name;
        bool first = true;
        for(auto p:parts){
            if(!first)name+=".";
            name+=p->str();
            first = false;
        }
        return name;
    }

    Ident* last(){
        flatten();
        // guaranteed to be an Ident after flattening
        auto p = (Ident*)parts.at(-1);
        return p;
    }
};