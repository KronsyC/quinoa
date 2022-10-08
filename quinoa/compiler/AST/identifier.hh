#pragma once
#include "./ast.hh"

struct Identifier:public Expression{
public:
    virtual std::string str(){
        return "";
    }
    virtual const char* c_str(){
        return "";
    }
    std::vector<Statement*> flatten(){
        return {this};
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
        auto name = this->name;
        return this->name;
    }
    const char* c_str(){
        return std::move(str().c_str());
    }

    Type* getType(LocalTypeTable types){

        auto type = types[name];
        

        return type;
    }
};
class CompoundIdentifier:public Ident{
public:
    std::vector<Identifier*> parts;
    CompoundIdentifier(std::vector<Identifier*> parts){
        this->parts = parts;
        flatify();
    }
    CompoundIdentifier(std::string value)
    {
        auto ident = new Ident(value);
        this->parts = {};
        this->parts.push_back(ident);
        flatify();
    }
    CompoundIdentifier(){
    };
    inline bool empty(){
        return this->parts.size() == 0;
    }
    void flatify(){
        std::vector<Identifier*> flattened;
        for(auto p:parts){
            if(instanceof<CompoundIdentifier>(p)){
                auto q = (CompoundIdentifier*)p;
                q->flatify();
                for(auto p:q->parts){
                    flattened.push_back(p);
                }
            }
            else if(instanceof<Ident>(p)){
                flattened.push_back(p);
            }
            else if(instanceof<Identifier>(p)){
                //FIXME: This will probably break
                continue;
            }
            else if(p == nullptr)error("Cannot flatten as a nullptr was found", true);
            else error("Failed to flatten ident");
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
    const char* c_str(){
        auto s = str();
        return std::move(s.c_str());
    }
    Ident* last(){
        flatify();
        if(parts.size() == 0)error("Cannot get last element of 0-length name");
        // guaranteed to be an Ident after flattening
        auto p = (Ident*)parts.at(parts.size()-1);
        return p;
    }
    // the exact opposite of the last function
    // useful for grabbing the namespace from a fully
    // declared member name
    CompoundIdentifier* all_but_last(){
        flatify();
        auto ret = new CompoundIdentifier;
        for(int i  = 0;i<parts.size()-1;i++){
            auto item = (Ident*)(parts[i]);
            ret->parts.push_back(item);
        }
        return ret;
    }

    bool equals(CompoundIdentifier* compare){
        compare->flatify();
        this->flatify();
        if(compare->parts.size() != this->parts.size())return false;
        for(int i = 0;i<compare->parts.size();i++){
            auto mine = (Ident*)this->parts[i];
            auto cmp = (Ident*)compare->parts[i];
            if(mine->name != cmp->name)return false;
        }
        return true;
    }
};