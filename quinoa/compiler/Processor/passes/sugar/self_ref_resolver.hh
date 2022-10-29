#pragma once
#include "../include.h"
using namespace std;

void resolve_self_refs(SourceBlock *content, ModuleRef *mod)
{
  auto flat = content->flatten();

  for (auto m : flat)
  {
    if (instanceof <MethodCall>(m))
    {
      auto call = (MethodCall *)m;
      if (call->builtin())
        continue;
      if(call->inst)continue;
      if(!call->name->mod){
        call->name->mod = mod;
      }
    }
  }
}
void resolve_self_refs(CompilationUnit &unit)
{
  for (auto fn : unit.getAllMethods())
  {
    resolve_self_refs(fn, fn->sig->name->mod );
  }
}