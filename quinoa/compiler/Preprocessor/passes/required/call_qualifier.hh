#pragma once

#include "../include.h"
#include "../../../AST/advanced_operators.hh"

std::pair<bool, int> qualify_calls(Method &code, CompilationUnit &unit) {
  int resolvedCount = 0;
  bool success = true;
  for (auto item : code.content->flatten()) {
    if (auto call = dynamic_cast<MethodCall*>(item)){
      Logger::debug("Qualify call: " + call->name->str());
      // auto sig = getMethodSig(code.memberOf, call);
      // Logger::debug("Qualified");
      // if(sig ){
      //   call->target = sig;
      //   auto method = call->target->belongsTo;
      //   if(!method->public_access){
      //     auto method_mod = method->memberOf;
      //     auto my_mod = code.memberOf;
      //     if(method_mod != my_mod)
      //     error("Cannot call private method " + method->sig->name->str() + " from " + code.sig->name->str());
      //   }
      //   resolvedCount++;
      // }
      // else success = false;
    }
  }
  return {success, resolvedCount};
}

std::pair<bool, int> qualify_calls(CompilationUnit &unit) {

  int count = 0;
  bool success = true;
  // Attempt to Qualify all Calls
  for (auto method : unit.get_methods()){
      auto result = qualify_calls(*method, unit);
      if(!result.first)success=false;
      count+=result.second;
  }
  return {success, count};
}