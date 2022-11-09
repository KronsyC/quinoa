#pragma once

#include "../include.h"
#include "../../../AST/advanced_operators.hh"


struct MatchRanking{
public:

  Method* against = nullptr;

  /**
   * Different ranking metrics
   * (organized by evaluation order)
  */

  bool possible = true;

  // Number of arguments which are matched as varargs (lower is better)
  bool vararg_count = 0;

  // number of generic-reliant parameters (lower is better, overrides general compatibility (when applicable))
  int generic_count = 0;

  // general compatibility (Type Matching)
  int general_compat = 0;

  MatchRanking(bool possible = false){
    possible = possible;
  }
};

std::map<PrimitiveType, std::string> primitive_group_mappings{
  PRIMITIVES_ENUM_GROUPS
};

int get_type_distance_from(Type& target, Type& type){

    // if they perfectly match, +0
    // if they match upon implicit casting (primitive) return 1
    // if they match upon implicit casting (inherited) return distance
    // if they do not match, return -1

  if(target.get<Ptr>() && type.get<Ptr>()){
    auto ptr1 = target.pointee();
    auto ptr2 = type.pointee();
    if(!ptr1 || !ptr2)return -1;
    return get_type_distance_from(*ptr1, *ptr2);
  }

  if(target.get<Primitive>() && type.get<Primitive>()){
    auto p1 = target.get<Primitive>();
    auto p2 = type.get<Primitive>();

    if(p1->kind == p2->kind)return 0;

    auto g1 = primitive_group_mappings[p1->kind];
    auto g2 = primitive_group_mappings[p2->kind];

    if( g1 == g2 )return 1;
    else{
      Logger::error("Cannot implicitly cast " + type.str() + " to " + target.str());
      return -1;
    }
  }
  #if DEBUG_MODE
    Logger::enqueueMode(false);
    Logger::warn("Failed to get distance from " + type.str() + " to " + target.str());
    Logger::enqueueMode(true);
  #endif
  return -1;
}

MatchRanking rank_method_against_call(Method* method, MethodCall* call){
  // Sanity Checks
  MatchRanking ranking(true);
  ranking.against = method;


  // Compare parameter counts (if applicable)
  if(!method->is_variadic()){
    if(method->parameters.len() != call->args.len())return MatchRanking();
  }
  else{
    // Call must have at least `len(parameters) - 1` arguments 
    if(call->args.len() < method->parameters.len() - 1)return MatchRanking();

    ranking.vararg_count = call->args.len() - (method->parameters.len() - 1);

  }


  if(method->generic_params.len()){
    // TODO: reimplement generics under the new system
    // this is a huge undertaking as the old implementation
    // used highly unsafe/sketchy tactics
    except(E_INTERNAL, "Generic methods are not yet supported");
  }
  for(size_t i = 0; i < call->args.len(); i++){
    Logger::debug("arg: " + call->args[0].str());
    auto&  arg_t  = *call->args[i].type();
    auto& param_t = *method->get_parameter(i)->type;

    // Compare the types of the arg and param

    auto score = get_type_distance_from(param_t, arg_t);
    if(score == -1){
      ranking.possible = false;
      break;
    }

    ranking.general_compat += score;
  }

  return ranking;
}

enum SelectionStage{
  INITIAL,
  VARAGS,
  GENERICS,
  RATING
};
Method* select_best_ranked_method(std::vector<MatchRanking>& ranks, SelectionStage stage = SelectionStage::INITIAL){
  Logger::debug(std::to_string(ranks.size()) + " possible matches");

  switch(stage){
    case SelectionStage::INITIAL:{
      std::vector<MatchRanking> suitors;
      for(auto r : ranks){
        if(!r.possible)continue;
        suitors.push_back(r);
      }
      return select_best_ranked_method(suitors, SelectionStage::VARAGS);

    }
    case SelectionStage::VARAGS:{
      int smallest_vararg_count = -1;
      for(auto r : ranks){
        if(smallest_vararg_count == -1)smallest_vararg_count = r.vararg_count;
        else if(r.vararg_count < smallest_vararg_count)smallest_vararg_count = r.vararg_count;
      }
      std::vector<MatchRanking> suitors;
      for(auto r : ranks){
        if(r.vararg_count <= smallest_vararg_count)suitors.push_back(r);
      }
      return select_best_ranked_method(suitors, SelectionStage::GENERICS);
    }
    case SelectionStage::GENERICS:{
      int smallest_generic_count = -1;
      for(auto r : ranks){
        if(smallest_generic_count == -1)smallest_generic_count = r.generic_count;
        else if(r.generic_count < smallest_generic_count)smallest_generic_count = r.generic_count;
      }
      std::vector<MatchRanking> suitors;
      for(auto r : ranks){
        if(r.generic_count <= smallest_generic_count)suitors.push_back(r);
      }
      return select_best_ranked_method(suitors, SelectionStage::RATING);
    }
  case SelectionStage::RATING:{
      int best_rating = -1;
      for(auto r : ranks){
        if(best_rating == -1)best_rating = r.general_compat;
        else if(r.general_compat < best_rating)best_rating = r.general_compat;
      }
      std::vector<MatchRanking> suitors;
      for(auto r : ranks){

        if(r.generic_count <= best_rating)suitors.push_back(r);
      }
      if(suitors.size() == 0)return nullptr;
  
      // if there are more than one suitor, the call is ambiguous
      auto call_name = suitors[0].against->name->str();
      if(suitors.size() > 1)except(E_BAD_CALL, "Call to " + call_name + " is ambiguous");
      return suitors[0].against;
  }
  default: except(E_INTERNAL, "bad selection stage");
  }
} 

Method* get_best_target(MethodCall* call, CompilationUnit& unit){
  // Find the module that the call targets
  Container* target = nullptr;
  auto call_mod_name = call->name->container->name->str();
  for(auto cont : unit.get_containers()){
    if(cont->type != CT_MODULE)continue;
    if(call_mod_name == cont->full_name().str()){
      target = cont;
      break;
    }
  }
  if(!target)except(E_BAD_CALL, "Failed to locate module '" + call_mod_name + "' for call");

  // Find all methods with the name
  std::vector<Method*> methods;

  auto call_method_name = call->name->member->str();
  
  for(auto method : target->get_methods()){
    if(call_method_name != method->name->member->str())continue;
    methods.push_back(method);

  }

  if(methods.size() == 0)except(E_BAD_CALL, "No methods of '"+call_mod_name+"' have the name '" + call_method_name + "'");

  std::vector<MatchRanking> ranks;
  for(auto method : methods){
    auto rank = rank_method_against_call(method, call);
    ranks.push_back(rank);
  }

  auto best_method = select_best_ranked_method(ranks);
  return best_method;
}

std::pair<bool, int> qualify_calls(Method &code, CompilationUnit &unit) {
  int resolvedCount = 0;
  bool success = true;
  for (auto item : code.content->flatten()) {
    if (auto call = dynamic_cast<MethodCall*>(item)){
      if(call->target)continue;
      auto best_fn = get_best_target(call, unit);
      if(best_fn){
        call->target = best_fn;
        resolvedCount++;
      }
      else{
        Logger::error("Failed to resolve call to " + call->name->str());
        success = false;
      }
    }
  }
  return {success, resolvedCount};
}

std::pair<bool, int> qualify_calls(CompilationUnit &unit) {

  int count = 0;
  bool success = true;
  // Attempt to Qualify all Calls
  for (auto method : unit.get_methods()){

      // skip out on signatures
      if(!method->content)continue;


      auto result = qualify_calls(*method, unit);
      if(!result.first)success=false;
      count+=result.second;
  }
  return {success, count};
}