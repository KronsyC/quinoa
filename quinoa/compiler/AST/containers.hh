/**
 * Containers are entities present at the top-level of a program,
 * the current containers in quinoa are:
 * 
 * - Module
 * - Seed
 * - Struct (wip)
*/


#pragma once
#include "./include.hh"
#include "./ast.hh"


class Container : public ANode{
public:
    std::unique_ptr<Name> name;
    std::unique_ptr<Name> name_space;

    Vec<ContainerMember> members;
    Vec<Generic>         generics;
    Vec<Compositor>      compositors;

};

class Seed : public Container{

};

class Module : public Container{

};

class Struct : public Container{

};