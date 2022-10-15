#pragma once
#include "../processor.h"
#include "../util.hh"
using namespace std;

void resolveBlockSelfRefs(SourceBlock *content, CompoundIdentifier* space) {
  auto flat = content->flatten();

  for (auto m : flat) {
    if (instanceof<MethodCall>(m)) {
      auto call = (MethodCall *)m;
      auto ctx = call->ctx;
      if(call->builtin())continue;
      call->name->flatify();
      if (call->name->parts.size() == 1) {
        pushf(call->name->parts, (Identifier *)space);
        call->name->flatify();
      }
    }
  }
}
void resolveSelfReferences(CompilationUnit &unit) {
    for (auto fn : unit.getAllMethods()) {
      resolveBlockSelfRefs(fn, fn->fullname()->all_but_last());
    }
}