#pragma once

#include "./container.hh"
#include "./container_member.hh"
#include "./include.hh"

struct GenericImpl {
    Method* target = nullptr;
    bool has_impl = false;
    TypeVec substituted_method_type_args;
    TypeVec substituted_target_type_args;
};

class CompilationUnit : public ANode {
  public:
    Vec<TopLevelEntity> members;

    std::vector<std::shared_ptr<GenericImpl>> generic_impls;

    // This scope holds global symbols
    std::unique_ptr<Scope> scope;
    void add_impl(Method* target, TypeVec subst_fn_ta, TypeVec subst_tgt_ta) {

        // TODO: Return early for duplicate implementations

        GenericImpl impl;
        impl.target = target;
        impl.substituted_target_type_args = subst_tgt_ta;
        impl.substituted_method_type_args = subst_fn_ta;

        generic_impls.push_back(std::make_unique<GenericImpl>(impl));
    }

    GenericImpl* get_next_impl() {
        for (auto& i : generic_impls) {
            if (i->has_impl)
                continue;
            return i.get();
        }
        return nullptr;
    }

    std::vector<Container*> get_containers() {
        std::vector<Container*> ret;
        for (auto m : members) {
            if (auto mod = dynamic_cast<Container*>(m.ptr)) {
                ret.push_back(mod);
            }
        }
        return ret;
    }

    std::vector<std::unique_ptr<TopLevelEntity>> transfer() {
        std::vector<std::unique_ptr<TopLevelEntity>> ret;
        for (auto m : members) {
            std::unique_ptr<TopLevelEntity> ptr(m.ptr);
            ret.push_back(std::move(ptr));
        }
        return ret;
    }

    std::vector<Method*> get_methods() {
        std::vector<Method*> ret;
        for (auto* cont : get_containers()) {
            for (auto method : cont->get_methods()) {
                ret.push_back(method);
            }
        }
        return ret;
    }

    std::vector<Property*> get_properties() {
        std::vector<Property*> ret;
        for (auto* cont : get_containers()) {
            for (auto prop : cont->get_properties()) {
                ret.push_back(prop);
            }
        }
        return ret;
    }

    std::vector<TypeMember*> get_types() {
        std::vector<TypeMember*> ret;
        for (auto cont : get_containers()) {
            for (auto member : cont->members) {
                if (auto type = dynamic_cast<TypeMember*>(member.ptr)) {
                    ret.push_back(type);
                }
            }
        }
        return ret;
    }

    std::vector<ContainerMember*> get_hoists() {
        std::vector<ContainerMember*> ret;
        for (auto m : get_methods())
            ret.push_back(m);
        for (auto p : get_properties())
            ret.push_back(p);
        return ret;
    }
};
