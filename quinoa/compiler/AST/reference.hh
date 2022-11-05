/**
 * A Reference is essentially anything that refers to anything else
 * this is intentionally a broad definition
*/
#pragma once
#include "./include.hh"
#include "./primary.hh"

class Reference : public ANode{

};

class Name: public Reference{
private:
    std::string value;

public:
    Name(std::string value){
        this->value = value;
    }
    Name() = default;
    operator std::string() const{
        return value;
    }
    std::string str(){
        return value;
    }
    void set_name(std::string val){
        this->value = val;
    }
};

class LongName : public Reference{
public:
    Vec<Name> parts;

    std::string str(){
        std::string ret;
        bool first = true;
        for(auto p : parts){
            if(!first)ret+="::";
            ret += p.str();
        }
        return ret;

    }
};

class SourceVariable : public Name, public Expr{

};

class Container;
class ContainerRef : public Reference{
public:
    Container* refers_to;
    std::unique_ptr<LongName> name;
    Vec<Type> generic_args;
    

    std::string str(){
        return name->str();
    }
};

class ContainerMemberRef : public Reference{
public:
    std::unique_ptr<ContainerRef> container;
    std::unique_ptr<Name> member;

    std::string str(){
        return  container->str() + "::" + member->str();
    }
};