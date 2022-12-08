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

class CompilationUnit;

class TopLevelEntity : public ANode {
public:
    CompilationUnit *parent = nullptr;
    bool is_imported = false;
};

class Import : public TopLevelEntity {
public:
    LongName target;
    LongName alias;
    bool is_stdlib = false;

    Import() = default;
};

enum ContainerType {
    CT_NOTYPE,
    CT_MODULE,
    CT_SEED,
    CT_STRUCT
};

class Container : public TopLevelEntity {
public:
    std::unique_ptr <Name> name = std::make_unique<Name>("unknown");
    std::shared_ptr <Name> name_space;

    Vec<ContainerMember> members;
    Vec<Generic> generics;
    Vec<ContainerRef> compositors;
    ContainerType type = CT_NOTYPE;

    // Map all aliases
    std::map<std::string, LongName> aliases;

    Vec<ContainerMember *> inherited_members;

    std::shared_ptr <ContainerRef> get_ref() {
        if (!self_ref) {
            self_ref = std::make_shared<ContainerRef>();
            self_ref->refers_to = this;
            self_ref->name = std::make_unique<LongName>(this->full_name());
        }
        return self_ref;
    }

    LongName full_name() const {
        LongName ret;
        if (name_space)ret.parts.push(*name_space);
        if (!name)except(E_INTERNAL, "container has no name?");
        ret.parts.push(*name);
        return ret;
    }

    ContainerRef *get_compositor(std::string comp_name) {
        for (auto comp: compositors) {
            if (comp->name->str() == comp_name) {
                return comp;
            }
        }
        return nullptr;
    }

    bool has_compositor(std::string comp_name) {
        if (get_compositor(comp_name))return true;
        return false;
    }

    std::shared_ptr <Type> get_type(std::string name = "_") {
        for (auto &member: members) {
            if (auto type = dynamic_cast<TypeMember *>(member.ptr)) {
                if (type->name->member->str() == name)return type->refers_to;
            }
        }

        if (name == this->name->str()) {
            return get_type();
        }


        return std::shared_ptr<Type>(nullptr);
    }

    std::vector<Method *> get_methods() {
        std::vector < Method * > ret;
        for (auto &member: members) {
            if (auto method = dynamic_cast<Method *>(member.ptr)) {
                ret.push_back(method);
            }
        }
        for (auto member: inherited_members) {
            if (auto method = dynamic_cast<Method *>(*member.ptr)) {
                ret.push_back(method);
            }
        }
        return ret;
    }

    std::vector<Property *> get_properties() {
        std::vector < Property * > ret;

        for (auto &member: members) {
            if (auto prop = dynamic_cast<Property *>(member.ptr)) {
                ret.push_back(prop);
            }
        }

        return ret;
    }

    bool implements_compatible_method(Method *check_against) {
        for (auto method: this->get_methods()) {
            auto is_equiv = method->is_equivalent_to(check_against);
            if (is_equiv)return true;
        }
        return false;
    }

private:
    std::shared_ptr <ContainerRef> self_ref;

};
