#pragma once

#include "./include.h"
#include "./sugar/self_ref_resolver.hh"


void apply_syntactic_sugar(CompilationUnit &unit) {
    resolve_self_refs(unit);
}


