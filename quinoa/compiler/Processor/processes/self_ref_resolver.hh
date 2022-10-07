#pragma once
#include "../processor.h"
#include "../util.hh"
using namespace std;

void resolveBlockSelfRefs(SourceBlock *content, Module *mod) {
  auto flat = content->flatten();

  for (auto m : flat) {
    if (instanceof <MethodCall>(m)) {
      auto call = (MethodCall *)m;
      auto ctx = call->ctx;

      call->name->flatify();
      if (call->name->parts.size() == 1) {
        Logger::log("Injecting Namespace for " + call->name->str());
        auto space = mod->name;
        pushf(call->name->parts, (Identifier *)space);
        call->name->flatify();
        Logger::log("Call is now " + call->name->str());
      }
    }
  }
}
void resolveSelfReferences(CompilationUnit &unit) {
  for (auto tli : unit.items) {
    if (instanceof <Module>(tli)) {
      auto mod = (Module *)tli;
      for (auto child : mod->items) {
        if (instanceof <Method>(child)) {
          auto fn = (Method *)child;
          resolveBlockSelfRefs(fn, mod);
        }
      }
    }
  }
}