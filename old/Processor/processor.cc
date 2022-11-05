#include "./processor.h"

// Preprocessor Pipeline Modules
#include "./passes/finalization.hh"
#include "./passes/metadata.hh"
#include "./passes/required_processes.hh"
#include "./passes/syntactic_sugar.hh"

void Processor::process(CompilationUnit* unit, bool finalize)
{

    apply_syntactic_sugar(*unit);
    process_required(unit);
    if(finalize) {
	process_metadata(*unit);
	finalize_ast(*unit);
    }
};
