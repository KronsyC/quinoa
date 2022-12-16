#pragma once

// Resolves Implicitly Typed Variables into a statically typed form
// this operation is crucial to the language
// This should be done after varname mangling to ensure for consistent
// type identification

#include "../include.h"
#include "./type_table_builder.hh"

proc_result resolve_types(Method& fn) {

    if (!fn.content)
        return {};

    build_method_type_table(fn);

    std::vector<std::string> errors;
    unsigned int res_count = 0;

    for (auto code : fn.content->flatten()) {

        // Resolve implicitly-typed variables 'let x = 73'
        if (auto init = dynamic_cast<InitializeVar*>(code)) {
            if (init->type)
                continue;

            if (!init->initializer) {
                // TODO: build out an inference engine to also take info from usage
                errors.emplace_back("Types can only be inferred for variables with initializers");
                continue;
            }

            init->type = init->initializer->type();
            if (!init->type) {
                errors.push_back("Failed to get the type of an unresolved expression: " + init->str());
            } else {
                init->scope->set_type(init->var_name.str(), init->type);
                res_count++;
            }
            continue;
        }
    }

    if (errors.size())
        return {res_count, errors};
    return {res_count, MaybeError()};
}
