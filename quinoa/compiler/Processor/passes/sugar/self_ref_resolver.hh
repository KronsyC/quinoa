#pragma once
#include "../include.h"
using namespace std;

// void resolve_self_refs(SourceBlock *content, CompoundIdentifier* space) {
//   auto flat = content->flatten();

//   for (auto m : flat) {
//     if (instanceof<MethodCall>(m)) {
//       auto call = (MethodCall *)m;
//       auto ctx = call->ctx;
//       if(call->builtin())continue;
//       call->name->flatify();
//       if (call->name->size() == 1) {
//         pushf(call->name->parts, (Identifier *)space);
//         call->name->flatify();
//       }
//     }
//   }
// }
void resolve_self_refs(CompilationUnit &unit) {
    // for (auto fn : unit.getAllMethods()) {
      // resolve_self_refs(fn, fn->fullname()->all_but_last());
    // }
}