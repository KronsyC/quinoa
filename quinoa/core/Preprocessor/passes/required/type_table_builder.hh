#pragma once

#include "../include.h"
#include <llvm/IR/GlobalVariable.h>

void declare(Container& cont, std::string name, _Type type, bool is_global) {
    auto long_name = cont.full_name().str() + "::" + name;
    cont.scope->set_type(name, type);

    if (is_global) {
        cont.parent->scope->set_type(long_name, type);
    } else {
        cont.scope->set_type(long_name, type);
    }
}

void build_method_type_table(Method& fn) {
    // Inject parameter types
    for (auto param : fn.parameters) {
        fn.scope->set_type(param->name.str(), param->type);
    }
    // Inject self (if applicable)
    if (fn.acts_upon) {
        fn.scope->set_type("self", ReferenceType::get(fn.get_target_type()));
    }
    if (!fn.content)
        return;
    // Inject local variables
    for (auto node : fn.content->flatten()) {
        if (auto init = dynamic_cast<InitializeVar*>(node)) {
            if (!init->type)
                return;

            init->scope->set_type(init->var_name.str(), init->type);
        }
    }
}

void build_container_type_table(Container& cont) {
    // Properties
    set_active_container(cont);
    for (auto prop : cont.get_properties()) {
        Logger::debug("Init prop: " + prop->name->str());
        auto init = prop->initializer->const_value(prop->type->llvm_type());
        declare(cont, prop->name->member->str(), prop->type, !prop->local_only);
        cont.member_decls.emplace_back(Container::MemberDeclaration{.name = prop->name->member->str(),
                                                                    .type = prop->type,
                                                                    .initializer = init,
                                                                    .is_constant = prop->constant,
                                                                    .is_public = !prop->local_only});
    }

    // enums
    for (auto m : cont.members) {
        if (auto typ = dynamic_cast<TypeMember*>(m.ptr)) {
            auto _enum = typ->refers_to->get<EnumType>();
            if (!_enum)
                continue;
            for (auto m : _enum->get_members()) {
                declare(cont, typ->name->member->str() + "::" + m.first, _enum->self, !typ->local_only);
                cont.member_decls.emplace_back(
                    Container::MemberDeclaration{.name = typ->name->member->str() + "::" + m.first,
                                                 .type = _enum->self,
                                                 .is_public = !typ->local_only,
                                                 .initializer = m.second,
                                                 .is_constant = true});
            }
        }
    }

    // build out method tables
    for (auto fn : cont.get_methods()) {
        build_method_type_table(*fn);
    }
}

void build_type_table(CompilationUnit& unit) {
    for (auto cont : unit.get_containers()) {
        build_container_type_table(*cont);
    }
}
