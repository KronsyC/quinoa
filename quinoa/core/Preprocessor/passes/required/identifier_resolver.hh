#pragma once
#include "../include.h"

void resolve_identifiers(Block& block, Container* parent) {
    for (const auto& stm : block.flatten()) {
        if (auto var = dynamic_cast<SourceVariable*>(stm)) {
            if (var->name->parts.len() == 1)
                continue;

            // replace the namespace with the new namespace
            auto ns = var->name->all_but_last();
            auto resolved_ns = parent->aliases[ns->str()];
            auto new_name = std::make_unique<LongName>();
            for (auto part : resolved_ns.parts) {
                new_name->parts.push(*part);
            }
            new_name->parts.push(var->name->last());
            var->name = std::move(new_name);
        }
    }
}

void resolve_identifiers(Method& fn) {
    if (fn.content)
        resolve_identifiers(*fn.content, fn.parent);
}

void resolve_identifiers(CompilationUnit& unit) {

    for (auto fn : unit.get_methods()) {
        resolve_identifiers(*fn);
    }
}
