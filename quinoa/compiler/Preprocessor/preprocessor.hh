#pragma once
#include "../AST/compilation_unit.hh"
#include "../../lib/logger.h"
#include "../../lib/error.h"

#include "./passes/required_processes.hh"

namespace Preprocessor
{
    void process_ast(CompilationUnit& unit){
        // except(E_INTERNAL, "Preprocessor not implemented");
        process_required(&unit);
    }
};
