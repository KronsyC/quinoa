#pragma once

/**
 * All Type References can be resolved immediately
 * i.e they never depend on inferrable data
 */
#include "../../../AST/intrinsic.hh"
#include "../include.h"
#include <memory>

void attempt_resolve_typeref(TypeRef& ref, Container* container) {

    // case: local type
    //       OR aliased module type
    if (ref.name->parts.len() == 1) {
        auto type_name = ref.name->str();

        auto maybe_local = container->get_type(type_name);

        if (maybe_local) {
            ref.resolves_to = maybe_local;
            return;
        }

        auto resolved_mod_name = container->aliases[type_name];

        if (!resolved_mod_name.parts.len())
            return;

        Container* target_container = nullptr;
        for (auto cont : container->parent->get_containers()) {
            if (cont->full_name().str() == resolved_mod_name.str()) {
                target_container = cont;
                break;
            }
        }
        if (!target_container)
            except(E_INTERNAL, "search for target container failed");

        // the container has to have an `_` member

        auto type_member = target_container->get_type();

        if (!type_member)
            except(E_BAD_TYPE, "A type was found to refer to the module " + type_name +
                                   ", however the module does not contain a default '_' type member\n\t\t");

        ref.resolves_to = type_member;
        return;
    }

    else {
        auto module_name = ref.name->all_but_last()->str();
        auto type_name = ref.name->last().str();

        auto module_name_resolved = container->aliases[module_name];

        if (!module_name_resolved) {
            except(E_BAD_TYPE, "A type was found to reference the module: " + module_name +
                                   ", but it could not be found"
                                   "\n\t\tin " +
                                   container->full_name().str());
        }

        Container* mod = nullptr;
        for (auto m : container->parent->get_containers()) {
            if (m->full_name().str() == module_name_resolved.str()) {
                mod = m;
                break;
            }
        }
        if (!mod)
            except(E_INTERNAL, "module lookup failed despite an alias existing");

        auto type = mod->get_type(type_name);

        if (!type) {
            // TODO: Fallback to module-aliased type references ( '_' types )
            except(E_BAD_TYPE, "The type reference: '" + ref.name->str() + "' was found to refer to the module '" +
                                   mod->full_name().str() +
                                   "'"
                                   "\n\t\tHowever, the module does not have a definition for type: '" +
                                   type_name + "'");
        }

        ref.resolves_to = type;
        return;
    }

    except(E_INTERNAL, "attempt_resolve_typeref not implemented for containers");
}

void attempt_resolve_typeref(TypeRef& ref, Method* method) {

    for (auto generic : method->generic_params) {
        if (generic->name->str() == ref.name->str()) {
            ref.resolves_to = generic;
            return;
        }
    }

    if (method->acts_upon) {
        for (auto generic : method->acts_upon_generic_args) {
            if (generic->name->str() == ref.name->str()) {
                ref.resolves_to = generic;
                return;
            }
        }
    }

    attempt_resolve_typeref(ref, method->parent);

    // Check the local container type definitions

    if (!ref.resolves_to)
        except(E_UNRESOLVED_TYPE, "Failed to resolve type " + ref.name->str());
}

void attempt_resolve_typeref(TypeRef& ref, TypeMember* mem) {

    for (auto generic : mem->generic_args) {
        if (generic->name->str() == ref.name->str()) {
            ref.resolves_to = generic;
            return;
        }
    }

    attempt_resolve_typeref(ref, mem->parent);

    // Check the local container type definitions

    if (!ref.resolves_to)
        except(E_UNRESOLVED_TYPE, "Failed to resolve type " + ref.name->str());
}

std::vector<TypeRef*> get_refs_from(Type& type) {
    std::vector<TypeRef*> ret;
    for (auto ty : type.flatten()) {
        if (auto tr = dynamic_cast<TypeRef*>(ty)) {
            ret.push_back(tr);
        }
    }
    return ret;
}

void resolve_if_typeref(Type& type, Method* method) {
    for (auto t : get_refs_from(type)) {
        attempt_resolve_typeref(*t, method);
    }
}

void resolve_if_typeref(Type& type, Container* container) {
    for (auto t : get_refs_from(type)) {
        attempt_resolve_typeref(*t, container);
    }
}

void resolve_type_references(CompilationUnit& unit) {

    for (auto method : unit.get_methods()) {

        // Resolves methods defined on types
        // i.e func foo.T
        if (auto au = method->acts_upon) {
            attempt_resolve_typeref(*au, method->parent);
        }

        // Resolve Return Type (if it is a type-ref)
        resolve_if_typeref(*method->return_type, method);

        // Resolve Parameter Type (if it is a type-ref)
        for (auto param : method->parameters) {
            resolve_if_typeref(*param->type, method);
        }

        // Resolve Initializer Type Refs
        if (!method->content)
            continue;

        for (auto code : method->content->flatten()) {
            if (auto init = dynamic_cast<InitializeVar*>(code)) {
                if (!init->type)
                    continue;
                resolve_if_typeref(*init->type, method);
            }

            if (auto cast = dynamic_cast<ExplicitCast*>(code)) {
                resolve_if_typeref(*cast->cast_to, method);
            }

            if (auto intr = dynamic_cast<_Intrinsic*>(code)) {
                for (auto arg : intr->type_args) {
                    resolve_if_typeref(*arg, method);
                }
            }
            if (auto call = dynamic_cast<CallLike*>(code)) {
                for (auto ta : call->type_args) {
                    resolve_if_typeref(*ta, method);
                }
            }
            if (auto sa = dynamic_cast<StructInitialization*>(code)) {
                resolve_if_typeref(*sa->type, method);
            }
        }
    }

    for (auto cont : unit.get_containers()) {
        for (auto mem : cont->members) {
            if (auto type = dynamic_cast<TypeMember*>(mem.ptr)) {
                for (auto flat : type->refers_to->flatten()) {
                    if (auto tref = dynamic_cast<TypeRef*>(flat)) {
                        attempt_resolve_typeref(*tref, type);
                    }
                }
            }
            if (auto prop = dynamic_cast<Property*>(mem.ptr)) {
                resolve_if_typeref(*prop->type, cont);
            }
        }
    }
}
