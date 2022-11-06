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

    
};