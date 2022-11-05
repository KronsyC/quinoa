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

class Method : public ContainerMember{
public:
    std::unique_ptr<Scope> content;
    Vec<Generic>           generic_params;
};