#pragma once

// Resolves Implicitly Typed Variables into a statically typed form
// this operation is crucial to the language
// This should be done after varname mangling to ensure for consistent
// type identification

#include "../include.h"
#include "./type_table_builder.hh"


// Returns True, if all types are resolved
std::pair<bool, int> resolve_types(CompilationUnit &unit) {


    bool isGood = true;
    int resolveCount = 0;
    for (auto method: unit.get_methods()) {
        build_method_type_table(method, unit);


        if (!method->content)continue;

        for (auto code: method->content->flatten()) {

            // Resolve implicitly-typed variables 'let x = 73'
            if (auto init = dynamic_cast<InitializeVar *>(code)) {
                if (init->type)continue;
                if (!init->initializer)continue;
                Logger::debug("Implicit type for: " + init->var_name.str());
                init->type = init->initializer->type();
                Logger::debug("init with: " + init->initializer->str());

                if (init->type) {
                    Logger::debug("Init as " + init->type->str());
                    init->scope->set_type(init->var_name.str(), init->type);
                    resolveCount++;
                } else isGood = false;
            }
            if (auto init = dynamic_cast<StructInitialization *>(code)) {
                if (init->type)continue;
                auto &name = init->target;

                std::shared_ptr <Type> resolves_to;
                for (auto typ: unit.get_types()) {
                    if (typ->name->member->str() == "_") {
                        // Match module, direct exit route
                        auto mod_name = typ->name->container->str();
                        if (mod_name == name->str()) {
                            resolves_to = typ->refers_to;
                            break;
                        }
                    } else {
                        if (typ->name->str() == name->str()) {
                            resolves_to = typ->refers_to;
                        }
                    }
                }

                if (!resolves_to) {
                    resolves_to = method->parent->get_type(name->str());
                }
                if (!resolves_to) {
                    isGood = false;
                    continue;
                }
                if (resolves_to->get<StructType>()) {
                    init->type = std::static_pointer_cast<StructType>(resolves_to);
                    resolveCount++;
                } else except(E_BAD_TYPE,
                              "Attempt to initialize " + name->str() + " as a struct, but it has the type: " +
                              resolves_to->str());
            }

        }

    }

    return {isGood, resolveCount};
}
