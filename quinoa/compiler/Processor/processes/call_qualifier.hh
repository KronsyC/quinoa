#pragma once

#include "../processor.h"
#include "../util.hh"

static std::map<PrimitiveType, std::string> primitive_group_mappings{
    PRIMITIVES_ENUM_GROUPS};
int getCompatabilityScore(QualifiedMethodSigStr base, QualifiedMethodSigStr target) {
  if (base.name->str() != target.name->str())
    return -1;
// compare namespaces
    if(base.space->str() != target.space->str())return -1;
    
  // compare param lengths TODO: Reimplement this once varargs are implemented
  if (base.params.size() != target.params.size())
    return -1;
  // Start with a base score, each infraction has a cost based on how different
  // it is
  int score = 0;
  for (int i = 0; i < base.params.size(); i++) {
    auto baram = base.params[i]->type;
    auto taram = target.params[i]->type;
    if (baram->equals(taram))
      continue;
    if (instanceof <Primitive>(baram) && instanceof <Primitive>(taram)) {
      // same group, different type is +1, otherwise no match

      auto bprim = (Primitive *)baram;
      auto tprim = (Primitive *)taram;
      auto bg = primitive_group_mappings[bprim->type];
      auto tg = primitive_group_mappings[tprim->type];
      if (bg == tg)
        score++;
      else
        score = -1;
    }
  }

  // TODO: Implement Type Reference Inheritance Tree Crawling (Each step up the tree is +1)
  return score;
}

// Takes a function and returns the best matching signature that it fits into to
// be called
MethodSignature *qualify(
  MethodCall *call,
   std::map<std::string, MethodSignature *> sigs,
   LocalTypeTable type_info
 ) {

  auto params = call->params;
  vector<Param *> testparams;
  for (auto p : params)
    testparams.push_back(new Param(p->getType(type_info), nullptr));
  auto callsig = new MethodSignature;
  callsig->name = call->name->last();
  callsig->params = testparams;
  callsig->space = call->name->all_but_last();
  auto sigstr = callsig->sigstr();

  CompoundIdentifier callname(call->name->parts);
  // replace the calls name with its mangled form
  callname.parts.pop_back();
  callname.parts.push_back((Identifier *)new Ident(sigstr.str()));
  // attempt to find a function with the exact sig
  auto fn = sigs[callname.str()];
  if (fn == nullptr) {
    sigs.erase(callname.str());
    // Run Compatibility Checks on each sigstr pair to find
    // the most compatible function to match to
    vector<int> compatabilityScores;
    for (auto sigpair : sigs) {
      auto sig = sigpair.second;
      auto name = sigpair.first;
      int compat = getCompatabilityScore(sig->sigstr(), callsig->sigstr());
      compatabilityScores.push_back(compat);
    }
    int max = -1;
    int prev = -1;
    int idx = -1;
    for (int i = 0; i < compatabilityScores.size(); i++) {
      auto s = compatabilityScores[i];
      if (s == -1)
        continue;
      if (s <= max || max == -1) {
        prev = max;
        max = s;
        idx = i;
      }
    }
    if (idx == -1)
      return nullptr;
    int ind = 0;
    for (auto pair : sigs) {
      if (ind == idx)
        return pair.second;
      ind++;
    }
    return nullptr;
  }
  return fn;
}
void qualifyCalls(SourceBlock &code,
                  std::map<std::string, MethodSignature *> sigs) {
  auto flat = flatten(code);
  for (auto item : flat) {
    if (instanceof <MethodCall>(item)) {
      auto call = (MethodCall *)item;
      auto tgtsig = qualify(call, sigs, code.local_types);
      if (tgtsig == nullptr)
        error("Failed to locate appropriate function call for " + call->name->str());
      call->target = tgtsig;
    }
  }
}
void qualifyCalls(CompilationUnit &unit) {
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