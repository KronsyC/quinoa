#pragma once
#include "../processor.h"
#include "../util.hh"
using namespace std;

void resolveBlockSelfRefs(SourceBlock *content, CompoundIdentifier* space) {
  auto flat = content->flatten();

  for (auto m : flat) {
    if (instanceof <MethodCall>(m)) {
      auto call = (MethodCall *)m;
      auto ctx = call->ctx;

      call->name->flatify();
      if (call->name->parts.size() == 1) {
        Logger::log("Injecting Namespace for " + call->name->str());
        pushf(call->name->parts, (Identifier *)space);
        call->name->flatify();
        Logger::log("Call is now " + call->name->str());
      }
    }
  }
}
void resolveSelfReferences(CompilationUnit &unit) {
    for (auto fn : unit.getAllMethods()) {
      resolveBlockSelfRefs(fn, fn->fullname()->all_but_last());
    }
}