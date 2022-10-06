#include "../../lib/error.h"
#include "../../lib/list.h"
#include "../../lib/logger.h"
#include "../AST/ast.hh"
#include "../compiler.h"
#include "./processor.h"
#include "./util.hh"

// Preprocessor Pipeline Modules
#include "./processes/importer.hh"
#include "./processes/hoister.hh"
#include "./processes/var_init_hoister.hh"
#include "./processes/self_ref_resolver.hh"
#include "./processes/call_qualifier.hh"
using namespace std;








void genEntryPoint(CompilationUnit &unit) {
  vector<Module *> entryPointCandidates;
  for (auto member : unit.items) {
    if (instanceof <Module>(member)) {
      auto mod = (Module *)member;
      if (mod->is("Entry"))
        entryPointCandidates.push_back(mod);
    }
  }
  if (entryPointCandidates.size() == 0)
    error("Failed to locate a suitable entrypoint");
  else if (entryPointCandidates.size() > 1) {
    Logger::warn(
        "Multiple Entry-Points were found, this may cause Unexpected Behavior");
  }
  auto entry = entryPointCandidates[0];
  string entryName = "main";
  if (entry->hasMethod(entryName)) {
    auto main = entry->getMethod(entryName);
    auto name = main->fullname();
    for (auto p : name->parts) {
      auto pname = p->str();
    }

    Entrypoint e(main->sig);
    e.sig = new MethodSignature();
    vector<Param *> params;
    params.push_back(new Param(new Primitive(PR_int32), new Ident("argc")));
    params.push_back(
        new Param(new TPtr(new Primitive(PR_string)), new Ident("argv")));
    e.sig->name = new Ident("main");
    e.sig->params = params;
    e.sig->returnType = new Primitive(PR_int32);
    auto call = new MethodCall();
    call->target = main->sig;
    auto ret = (Primitive *)main->sig->returnType;
    if (ret->type == PR_void) {
      call->params = {};
      e.items.push_back(new Return(new Integer(0)));
    } else {

      call->params = {new Ident("argc"), new Ident("argv")};
      e.items.push_back(new Return(call));
    }
    unit.items.push_back(new Entrypoint(e));
  } else
    error("The Entrypoint '" + entry->name->str() +
          "' does not contain a main method");
}

void Processor::process(CompilationUnit &unit, bool finalize) {
  /**
   * Preprocess the tree via various different processes:
   * ----------------------------------------------------
   * ✅ Import resolution
   * ✅ Method Shorthand Self-Referencing (foo() -> method.foo())
   * Generic Implementation (Type Substitution and method gen)
   * ✅ Method Mangling (For Overloads)
   * ✅ Function Call Qualification (Signature Reference Injection)
   * Duplicate Name Detection
   * ✅ Function/Property Hoisting (at the module level)
   * Type Checking
   * Unused Variable Warning / Removal
   * Unreachable Code Warning / Removal
   * Static Statement Resolution ( 11 + 4 -> 15 )
   * Local Initializer Hoisting (optimization) (may require renames for
   * block-scoped overrides) ✅ Entrypoint Generation
   */

  resolveImports(unit);
  resolveSelfReferences(unit);
  hoistVarInitializations(unit);
  if (finalize) {
    qualifyCalls(unit);
    hoistDefinitions(unit);
    genEntryPoint(unit);
  }
};
