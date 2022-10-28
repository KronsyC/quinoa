#pragma once
#include "../include.h"


void gen_entrypoint(CompilationUnit &unit)
{
  std::vector<Module *> entryPointCandidates;
  for (auto member : unit)
  {
    if (instanceof <Module>(member))
    {
      auto mod = (Module *)member;
      if (mod->comp("Entry"))
        entryPointCandidates.push_back(mod);
    }
  }
  if (entryPointCandidates.size() == 0)
    error("Failed to locate a suitable entrypoint");
  else if (entryPointCandidates.size() > 1)
  {
    Logger::warn(
        "Multiple Entry-Points were found, this may cause Unexpected Behavior");
  }
  auto entry = entryPointCandidates[0];
  std::string entryName =entry->name->str()+"::main";
  for(auto item:*entry){
    if(instanceof<Method>(item)){
      auto m = (Method*)item;
      if(m->sig->name->str() == entryName){
        if(!m->public_access)error("The main() method must be public");
        unit.push_back(new Entrypoint(m->sig));
        return;
      }
    }
  }
    error("The Entrypoint '" + entry->name->str() +
          "' does not contain a main method");
}