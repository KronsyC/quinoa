/**
 * Container Members are anything that goes inside a container
 * includes:
 * - Properties
 * - Methods 
*/

#pragma once

#include "./reference.hh"
#include "./constant.hh"

class Container;

class Attribute {
public:
    std::string name;
    Vec<ConstantValue> arguments;
};

class ContainerMember : public ANode {
public:
    std::unique_ptr <ContainerMemberRef> name;
    std::shared_ptr <Name> name_space;
    Container *parent;
    Vec<Attribute> attrs;
    bool instance_only = false;
    bool local_only = false;

    bool aliases_resolved = false;

};


class Property : public ContainerMember {
public:
    std::unique_ptr <ConstantValue> initializer;
    std::shared_ptr <Type> type;
};

class TypeMember : public ContainerMember {
public:
    std::shared_ptr <Type> refers_to;

    TypeMember(std::shared_ptr <Type> t) {
        this->refers_to = t;
    }
};

class Param : public ANode {
public:
    Name name;
    std::shared_ptr <Type> type;
    bool is_variadic = false;

    Param(Name pname, std::shared_ptr <Type> type)
            : name(pname) {
        this->type = std::move(type);
    }
};


class Method : public ContainerMember {
public:
    // `func foo.Type` where `Type` is what is acted upon
    std::shared_ptr <TypeRef> acts_upon;
    std::vector <std::shared_ptr<Generic>> generic_params;
    Vec<Param> parameters;
    std::shared_ptr <Type> return_type;
    std::unique_ptr <Scope> content;

    Vec<std::vector < std::shared_ptr < Type>>>
    generate_usages;

    bool is_variadic() {
        // Check if the last parameter is a var-arg
        if (!parameters.len())return false;
        auto &last = parameters[parameters.len() - 1];
        return last.is_variadic;
    }

    // Get the parameter at a specific index
    // this method is smart and accounts for varargs
    // returns `nullptr` if there is no parameter at the given index
    Param *get_parameter(size_t idx) {
        if (idx < parameters.len())return &parameters[idx];
        else if (is_variadic())return &parameters[parameters.len() - 1];
        else return nullptr;
    }

    std::string source_name() {
        if (this->name->trunc)return this->name->str();
        std::string name = name_space ? name_space->str() + "::" : "";
        name += this->name->str();
        if (this->generic_params.size()) {
            name += "<";
            bool first = true;
            for (auto g: generic_params) {
                if (!first)name += ",";
                name += g->str();
                first = false;
            }
            name += ">";
        }
        for (auto p: parameters) {
            name += "." + p->type->str();
        }
        return name;
    }

    bool is_equivalent_to(Method *method) {

        // basic checks (extremely strict, possibly lax this in the future)
        if (name->member->str() != method->name->member->str())return false;
        if (generic_params.size() != method->generic_params.size())return false;
        if (parameters.len() != method->parameters.len())return false;

        // type-related checks
        if (return_type->distance_from(*method->return_type) < 0)return false;
        for (size_t i = 0; i < parameters.len(); i++) {
            if (parameters[i].type->distance_from(*method->parameters[i].type) < 0)return false;
        }
        return true;
    }
};