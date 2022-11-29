#pragma once

#include "./include.h"

#include "./required/compositor_resolver.hh"
#include "./required/array_validator.hh"
#include "./required/call_qualifier.hh"
#include "./required/type_resolver.hh"
#include "./required/type_checker.hh"
#include "./required/instance_call_resolver.hh"
#include "./required/type_table_builder.hh"
#include "./required/variable_usage_validator.hh"
#include "./required/unreachable_code_warner.hh"
#include "./required/type_ref_resolver.hh"

void process_required(CompilationUnit *unit) {
    resolve_compositors(*unit);
    resolve_type_references(*unit);
    // resolve_props(*unit);
    bool resolvedTypes = false;
    bool resolvedCalls = false;
    bool resolvedInstances = false;

#ifndef DEBUG
    Logger::enqueueMode(true);
#endif

    int run = 2;
    while (run) {
        Logger::clearQueue();
        auto typeres = resolve_types(*unit);
        auto res = qualify_calls(*unit);
        auto ins = resolve_instance_calls(*unit);
        resolvedCalls = res.first;
        resolvedTypes = typeres.first;
        resolvedInstances = ins.first;

        if (resolvedCalls && resolvedTypes && resolvedInstances) {
            run--;
            continue;
        }

        // if nothing was resolved this iteration
        if (!res.second && !typeres.second && !ins.second) {
            Logger::printQueue();
            error("Type-Call Resolution Failed with " + std::to_string(res.second) + " resolved calls and " +
                  std::to_string(typeres.second) + " resolved types");
        }
    }
    Logger::clearQueue();
    Logger::enqueueMode(false);
    build_type_table(*unit);
    validate_array_literals(*unit);
    check_types(*unit);
    validate_variable_usage(*unit);
    warn_unreachable_code(*unit);

}