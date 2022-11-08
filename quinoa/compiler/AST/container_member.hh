/**
 * Container Members are anything that goes inside a container
 * includes:
 * - Properties
 * - Methods 
*/

#pragma once
#include "./reference.hh"
class Container;

class ContainerMember : public ANode{
public:
    std::unique_ptr<ContainerMemberRef> name;
    Container* parent;
};


class Property : public ContainerMember{
public:
    std::unique_ptr<Expr> initializer;
};


class Param : public ANode{
public:
    Name name;
    std::unique_ptr<Type> type;
    bool is_variadic = false;
    Param(Name pname, std::unique_ptr<Type> type)
    : name(pname)
    {
        this->type = std::move(type);
    }
};

class Method : public ContainerMember{
public:
    std::unique_ptr<Scope> content;
    Vec<Generic>           generic_params;
    Vec<Param>             parameters;
    std::unique_ptr<Type>  return_type;

    bool is_variadic(){
        // Check if the last parameter is a var-arg
        if(!parameters.len())return false;
        auto& last = parameters[parameters.len()-1];
        return last.is_variadic;
    }

    // Get the parameter at a specific index
    // this method is smart and accounts for varargs
    // returns `nullptr` if there is no parameter at the given index
    Param* get_parameter(size_t idx){
        if(idx < parameters.len())return &parameters[idx];
        else if(is_variadic())return &parameters[parameters.len()-1];
        else return nullptr;
    }

    std::string source_name(){
        if(this->name->trunc)return this->name->str();
        auto name = this->name->str();
        for(auto p : parameters){
            name += "." + p->type->str();
        }
        return name;
    }
};