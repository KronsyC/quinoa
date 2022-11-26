#pragma once

/**
* All Type References can be resolved immediately
 * i.e they never depend on inferrable data
*/
#include "../include.h"

void attempt_resolve_typeref(TypeRef &ref, Container *container) {
    auto locally_defined_type = container->get_type(ref.name->str());
    if (locally_defined_type) {
        ref.resolves_to = locally_defined_type;
        Logger::debug("Resolved locally as: " + ref.resolves_to->str());
        return;
    }

    // Check other modules
    for (auto type: container->parent->get_types()) {
        if (type->name->str() == ref.name->str()) {
            ref.resolves_to = type->refers_to;
            Logger::debug("Resolved remotely as: " + ref.resolves_to->str());
            return;
        }
    }
}

void attempt_resolve_typeref(TypeRef &ref, Method *method) {
    Logger::debug("Resolving ref: " + ref.name->str());

    for (auto generic: method->generic_params) {
        if (generic->name->str() == ref.name->str()) {
            ref.resolves_to = generic;
            return;
        }
    }

    attempt_resolve_typeref(ref, method->parent);

    // Check the local container type definitions

    if (!ref.resolves_to)except(E_UNRESOLVED_TYPE, "Failed to resolve type " + ref.name->str());
}

std::vector<TypeRef *> get_refs_from(Type &type) {
    std::vector < TypeRef * > ret;
    for (auto ty: type.flatten()) {
        if (auto tr = dynamic_cast<TypeRef *>(ty)) {
            ret.push_back(tr);
        }
    }
    return ret;
}

void resolve_if_typeref(Type &type, Method *method) {
    for (auto t: get_refs_from(type)) {
        attempt_resolve_typeref(*t, method);
    }
}

void resolve_if_typeref(Type &type, Container *container) {
    for (auto t: get_refs_from(type)) {
        attempt_resolve_typeref(*t, container);
    }
}

void resolve_type_references(CompilationUnit &unit) {

    for (auto method: unit.get_methods()) {

        if (auto au = method->acts_upon) {
            attempt_resolve_typeref(*au, method);
        }


        // Resolve Return Type (if it is a type-ref)
        resolve_if_typeref(*method->return_type, method);

        // Resolve Parameter Type (if it is a type-ref)
        for (auto param: method->parameters) {
            resolve_if_typeref(*param->type, method);
        }

        // Resolve Initializer Type Refs
        if (!method->content)continue;

        for (auto code: method->content->flatten()) {
            if (auto init = dynamic_cast<InitializeVar *>(code)) {
                if (!init->type)continue;
                resolve_if_typeref(*init->type, method);
            }

            if (auto cast = dynamic_cast<ExplicitCast *>(code)) {
                resolve_if_typeref(*cast->cast_to, method);
            }
        }
    }

    for (auto cont: unit.get_containers()) {
        for (auto mem: cont->members) {
            if (auto type = dynamic_cast<TypeMember *>(mem.ptr)) {
                for (auto flat: type->refers_to->flatten()) {
                    resolve_if_typeref(*flat, cont);
                }

            }
        }
    }

}