#pragma once

#include "../include.h"

using namespace std;

void resolve_self_refs(Scope *content, Container *mod) {
    if (!mod)except(E_INTERNAL, "no module passed to resolve_self_refs");
    auto flat = content->flatten();
    for (auto m: flat) {
        if (auto call = dynamic_cast<MethodCall *>(m)) {
            if (!call->name->container.get()) {
                call->name->container = mod->get_ref();
            }
        }
    }
}

void resolve_self_refs(CompilationUnit &unit) {
    for (auto fn: unit.get_methods()) {
        if (fn->content) {
            resolve_self_refs(fn->content.get(), fn->parent);
        }
    }
}