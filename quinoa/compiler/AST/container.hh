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
#include "./reference.hh"
#include "./type.hh"
#include "./container_member.hh"

class Compositor;

class TopLevelEntity : public ANode{

};

class Import : public TopLevelEntity{
public:
    LongName target;
    Name alias;
    bool is_stdlib = false;

    Import() = default;
};

class Container : public TopLevelEntity
{
public:
    std::unique_ptr<Name> name;
    std::shared_ptr<Name> name_space;

    Vec<ContainerMember> members;
    Vec<Generic>         generics;
    Vec<ContainerRef>    compositors;

    std::shared_ptr<ContainerRef> get_ref(){
        return self_ref;
    }
    Container(){
        self_ref = std::make_shared<ContainerRef>();
        self_ref->refers_to = this;
    }
private:
    std::shared_ptr<ContainerRef> self_ref;

};

class Seed : public TopLevelEntity{

};



class Module : public Container{
public:
    Module(Module&) = default;
};

class Struct : public Container{

};