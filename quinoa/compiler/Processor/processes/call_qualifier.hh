#pragma once

#include "../processor.h"
#include "../util.hh"


void qualifyCalls(SourceBlock &code,
                  std::map<std::string, MethodSignature *> sigs) {
  auto flat = code.flatten();
  for (auto item : flat) {
    if (instanceof <MethodCall>(item)) {
      auto call = (MethodCall *)item;
      call->qualify(sigs, *code.local_types);
    }
  }
}

std::map<std::string, MethodSignature*> fetchSignatures(CompilationUnit unit){
    // Construct a table of all call names -> their signatures
  std::map<std::string, MethodSignature *> sigs;
  for (auto item : unit.items) {
    if (instanceof <Module>(item)) {
      auto mod = (Module *)item;
      for (auto item : mod->items) {
        if (instanceof <Method>(item)) {
          auto method = (Method *)item;
          auto name = method->sig->sourcename();
          sigs[name] = method->sig;
        }
      }
    }
  }
  return sigs;
}

void qualifyCalls(CompilationUnit &unit) {

  auto sigs = fetchSignatures(unit);
  // Attempt to Qualify all Calls
  for (auto item : unit.items) {
    if (instanceof <Module>(item)) {
      auto mod = (Module *)item;
      for (auto item : mod->items) {
        if (instanceof <Method>(item)) {
          auto method = (Method *)item;
          qualifyCalls(*method, sigs);
        }
      }
    }
  }
}