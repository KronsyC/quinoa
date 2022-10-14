#pragma once

#include "../processor.h"
#include "../util.hh"


std::pair<bool, int> qualifyCalls(SourceBlock &code,
                  std::map<std::string, MethodSignature *> sigs) {
  auto flat = code.flatten();
  int resolvedCount = 0;
  bool success = true;
  for (auto item : flat) {
    if (instanceof<MethodCall>(item)) {
      auto call = (MethodCall *)item;
      // Don't do redundant qualification
      if(call->target )continue;

      call->qualify(sigs, *code.local_types);
      if(call->target )resolvedCount++;
      else success = false;
    }
  }
  return {success, resolvedCount};
}

std::map<std::string, MethodSignature*> fetchSignatures(CompilationUnit unit){
    // Construct a table of all call names -> their signatures
  std::map<std::string, MethodSignature *> sigs;

  // All Implemented Functions
  for (auto method : unit.getAllMethods()) {
    auto name = method->sig->sourcename();
    sigs[name] = method->sig;
  }

  // All Unimplemented Functions
  for(auto item:unit){
    if(instanceof<MethodPredeclaration>(item)){
      auto sig = (MethodPredeclaration*)item;
      if(sig->sig->nomangle){
        sigs[sig->sig->name->str()] = sig->sig;
      }
    }
  }
  return sigs;
}
std::pair<bool, int> qualifyCalls(CompilationUnit &unit) {

  auto sigs = fetchSignatures(unit);
  int count = 0;
  bool success = true;
  // Attempt to Qualify all Calls
  for (auto method : unit.getAllMethods()) {
      auto result = qualifyCalls(*method, sigs);
      if(!result.first)success=false;
      count+=result.second;
  }
  return {success, count};
}