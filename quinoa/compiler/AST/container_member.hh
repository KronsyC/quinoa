/**
 * Container Members are anything that goes inside a container
 * includes:
 * - Properties
 * - Methods 
*/

#pragma once
#include "./reference.hh"
class Container;

class ContainerMember{
public:
    std::unique_ptr<ContainerMemberRef> name;
    Container* parent;
};


class Property : public ContainerMember{

};


class Param : public ANode{
public:
    Name name;
    std::unique_ptr<Type> type;
    
    // TODO: default value, var_args
};

class Method : public ContainerMember{
public:
    std::unique_ptr<Scope> content;
    Vec<Generic>           generic_params;
    Vec<Param>             parameters;
};