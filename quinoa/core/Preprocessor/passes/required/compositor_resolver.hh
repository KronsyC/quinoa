/**
 * Resolve Compositor Symbolic Module References
*/

#pragma once

#include "../include.h"



void resolve_compositor_refs(Container *mod, CompilationUnit &unit) {
    for (auto &comp: mod->compositors) {
        auto name = comp->name->str();
        if (includes(Container::NATIVE_MODULES, name))continue;

        for (auto mod: unit.get_containers()) {
            auto fullname = std::make_unique<LongName>();
            if (mod->name_space)fullname->parts.push(*mod->name_space);
            fullname->parts.push(*mod->name);
            if (name == fullname->str()) {
                comp->refers_to = mod;
                break;
            }
        }
        if (!comp->refers_to) {
            error("Failed to resolve compositor for " + comp->name->str());
        }
        if (comp->refers_to->type != ContainerType::CT_SEED) {
            except(E_BAD_COMPOSITOR,
                   "Container '" + mod->name->str() + "' cannot inherit from non-seed '" + comp->str() + "'");
        }


        for (auto method: comp->refers_to->get_methods()) {

            // if there is no implementation, ensure the derived
            // module implements the method
            if (!method->content) {
                auto derives = mod->implements_compatible_method(method);
                if (!derives)
                    except(E_BAD_COMPOSITOR,
                           "Container " + mod->name->str() + " does not fully implement the compositor: " +
                           comp->name->str()
                           + "\n\t missing implementation for method: " + method->name->member->str());
            } else {
                auto derives = mod->implements_compatible_method(method);
                if (derives)continue;
                else {
                    mod->inherited_members.push((ContainerMember *) method);
                }
            }

        }
    }
}


void resolve_compositors(CompilationUnit &unit) {
    for (auto mod: unit.get_containers()) {
        resolve_compositor_refs(mod, unit);
    }
}
