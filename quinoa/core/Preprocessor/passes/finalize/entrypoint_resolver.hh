#pragma once

#include "../include.h"

void gen_entrypoint(CompilationUnit &unit) {
    std::vector< Container * > entryPointCandidates;
    for (auto member: unit.get_containers()) {
        if (member->type == CT_MODULE && member->has_compositor("Entry"))
            entryPointCandidates.push_back(member);

    }
    if (entryPointCandidates.size() == 0)except(E_NO_ENTRYPOINT, "Failed to locate a suitable entrypoint");
    else if (entryPointCandidates.size() > 1) {
        Logger::warn("Multiple Entry-Points were found, this may lead to Undefined Behavior");
    }
    auto entry = entryPointCandidates[0];
    for (auto method: entry->get_methods()) {
        if (method->name->member->str() == "main") {
            method->name->trunc = true;
            return;

        }
    }
    except(E_NO_ENTRYPOINT, "The Entrypoint '" + entry->name->str() +
                            "' does not contain a main method");
}