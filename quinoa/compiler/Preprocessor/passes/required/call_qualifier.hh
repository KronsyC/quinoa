#pragma once

#include "../include.h"
#include "../../../AST/advanced_operators.hh"


struct MatchRanking{
public:
  bool possible = true;

  // general compatibility (Type Matching)
  int general_compat = 0;

  // number of generic-reliant parameters (lower is better, overrides general compatibility (when applicable))
  int generic_count = 0;

  // wether the match is variadic, only pick this as a last resort
  bool is_variadic = false;


  MatchRanking(bool possible = false){
    possible = possible;
  }
};
MatchRanking rank_method_against_call(Method* method, MethodCall* call){
  // Sanity Checks

  if(method->name->container->name->str() != call->name->container->name->str())MatchRanking();
  if(method->name->member->str() != call->name->member->str())MatchRanking();

  return MatchRanking(true);
}

Method* get_best_target(MethodCall* call, CompilationUnit& unit){
  // Find the module that the call targets
  Container* target = nullptr;
  auto call_mod_name = call->name->container->name->str();
  for(auto cont : unit.get_containers()){
    if(cont->type != CT_MODULE)continue;
    if(call_mod_name == cont->name->str()){
      target = cont;
      break;
    }
    Logger::debug("check against: " + cont->name->str()); 
  }
  if(!target)except(E_BAD_CALL, "Failed to locate module '" + call_mod_name + "' for call");

  // Find all methods with the name
  std::vector<Method*> methods;

  auto call_method_name = call->name->member->str();
  for(auto method : target->get_methods()){
    if(call_method_name == method->name->member->str()){
      methods.push_back(method);
    }
  }

  if(methods.size() == 0)except(E_BAD_CALL, "No methods of '"+call_mod_name+"' have the name '" + call_method_name + "'");

  std::vector<MatchRanking> ranks;
  for(auto method : methods){
    auto rank = rank_method_against_call(method, call);
    ranks.push_back(rank);
  }
  Logger::debug("call member of " + target->name->str());
  except(E_INTERNAL, "get_best_target not implemented");
}

std::pair<bool, int> qualify_calls(Method &code, CompilationUnit &unit) {
  int resolvedCount = 0;
  bool success = true;
  for (auto item : code.content->flatten()) {
    if (auto call = dynamic_cast<MethodCall*>(item)){
      Logger::debug("Qualify call: " + call->name->str());
      auto best_fn = get_best_target(call, unit);
      Logger::debug("calls " + best_fn->name->str());
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