#include "../../lib/error.h"
#include "../../lib/list.h"
#include "../../lib/logger.h"
#include "../AST/ast.hh"
#include "./processor.h"

// Preprocessor Pipeline Modules
#include "./passes/syntactic_sugar.hh"
#include "./passes/finalization.hh"
#include "./passes/required_processes.hh"
#include "./passes/metadata.hh"



void Processor::process(CompilationUnit *unit, bool finalize)
{
  
  apply_syntactic_sugar(*unit);
  process_required(unit);
  if(finalize){
    process_metadata(*unit);
    finalize_ast(*unit);
  }
};
