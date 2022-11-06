/**
 * A Reference is essentially anything that refers to anything else
 * this is intentionally a broad definition
*/
#pragma once
#include "./include.hh"
#include "./primary.hh"
#include "../../lib/logger.h"
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
        for(auto& p : parts){
            if(!first)ret+="::";
            ret += p->str();
            first = false;
        }
        return ret;

    }

    Name& last(){
        return parts[parts.len() - 1];
    }
};

class SourceVariable : public Name, public Expr{
public:
    std::unique_ptr<LongName> name;
    SourceVariable(LongName name){
        this->name = std::make_unique<LongName>(name);
    }
    SourceVariable(Name name){
        this->name = std::make_unique<LongName>();
        this->name->parts.push(name);
    }

    std::string str(){
        return name->str();
    }
    llvm::Value* llvm_value(VariableTable& vars, llvm::Type* expected_type = nullptr){
        auto& var = vars[name->str()];
        auto value = builder()->CreateLoad(var.value->getType()->getPointerElementType(), var.value);
        return value;
    }
    std::unique_ptr<Type> get_type(){
        except(E_INTERNAL, "get_type not implemented for SourceVariable");
    }

    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        return ret;
    }
};

class Container;
class ContainerRef : public Reference{
public:
    Container* refers_to = nullptr;
    std::unique_ptr<LongName> name;
    Vec<Type> generic_args;
    

    std::string str(){
        return name->str();
    }
};

class ContainerMemberRef : public Reference{
public:
    std::shared_ptr<ContainerRef> container;
    std::unique_ptr<Name> member;
    bool trunc = false;
    std::string str(){
        auto mem_name = member->str();
        if(trunc || !container)return mem_name;
        auto con_name = container->str();
        return con_name + "::" + mem_name;
    }
};