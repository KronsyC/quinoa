#include "../../lib/error.h"
#include "../../lib/list.h"
#include "../../lib/logger.h"
#include "../AST/ast.hh"
#include "../compiler.h"
#include "./processor.h"
#include "./util.hh"

// Preprocessor Pipeline Modules
#include "./passes/importer.hh"
#include "./passes/member_hoister.hh"
#include "./passes/var_init_hoister.hh"
#include "./passes/self_ref_resolver.hh"
#include "./passes/call_qualifier.hh"
#include "./passes/type_resolver.hh"
#include "./passes/array_validator.hh"
#include "./passes/separate_initialzers.hh"
#include "./passes/primitive_function_injector.hh"
#include "./passes/generic_implementer.hh"
using namespace std;

void genEntryPoint(CompilationUnit &unit)
{
  vector<Module *> entryPointCandidates;
  for (auto member : unit)
  {
    if (instanceof <Module>(member))
    {
      auto mod = (Module *)member;
      if (mod->is("Entry"))
        entryPointCandidates.push_back(mod);
    }
  }
  if (entryPointCandidates.size() == 0)
    error("Failed to locate a suitable entrypoint");
  else if (entryPointCandidates.size() > 1)
  {
    Logger::warn(
        "Multiple Entry-Points were found, this may cause Unexpected Behavior");
  }
  auto entry = entryPointCandidates[0];
  string entryName = "main";
  for(auto item:*entry){
    if(instanceof<Method>(item)){
      auto m = (Method*)item;
      if(m->sig->name->str() == entryName){
        unit.push_back(new Entrypoint(m->sig));
        return;
      }
    }
  }
    error("The Entrypoint '" + entry->name->str() +
          "' does not contain a main method");
}

void Processor::process(CompilationUnit &unit, bool finalize)
{
  /**
   * Preprocess the tree via various different passes:
   * ----------------------------------------------------
   * ✅ Import resolution
   * ✅ Method Shorthand Self-Referencing
   * Generic Implementation (Type Substitution and method gen)
   * ✅ Method Mangling
   * (Codependant loop)
   * \
   *  ✅ Type Resolution
   *  ✅ Function Call Qualification
   * /
   * Duplicate Name Detection
   * ✅ Function/Property Hoisting
   * Type Checking
   * Unused Variable Warning / Removal
   * Unreachable Code Warning / Removal
   * Static Statement Resolution ( 11 + 4 -> 15 )
   * Literal Array Checking (performs any necessary casts / erroing)
   * ✅ Initializer Separation
   * ✅ Local Initializer Hoisting (optimization)
   * ✅ Entrypoint Generation
   */
  resolveImports(unit);
  if (finalize)
  {
    Logger::log("Resolvint Self Refs");
    resolveSelfReferences(unit);

    bool resolvedTypes = false;
    bool resolvedCalls = false;

    Logger::log("Resolving Types / Calls");
    Logger::enqueueMode(true);
    while (!(resolvedTypes && resolvedCalls))
    {
      // ensure only this iteration's errors are reported on
      Logger::clearQueue();
      auto typeres = resolveTypes(unit);
      auto res = qualifyCalls(unit);
      resolvedCalls = res.first;
      resolvedTypes = typeres.first;

      // if no calls or types were resolved this iteration
      if (!res.second && !typeres.second && !(resolvedCalls && resolvedTypes))
      {
        Logger::printQueue();
        error("Type-Call Resolution Failed with " + std::to_string(res.second) + " resolved calls and " + std::to_string(typeres.second) + " resolved types");
      }
    }
    Logger::clearQueue();
    Logger::enqueueMode(false);
    Logger::debug("Implementing Generics");
    implGenerics(unit);
    Logger::log("Hoisting Definitions");
    hoistDefinitions(unit);
    Logger::log("Resolving Arrays");
    validateLiteralArrays(unit);
    Logger::log("Splitting Initializers");
    split_initializers(unit);
    hoistVarInitializations(unit);
    injectPrimitiveFunctions(unit);
    genEntryPoint(unit);
  }
};
