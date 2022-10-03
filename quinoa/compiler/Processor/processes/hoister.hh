#pragma once
#include "../processor.h"

using namespace std;


void hoistDefinitions(CompilationUnit &unit) {
  vector<TopLevelExpression *> items;
  for (auto child : unit.items) {
    if (instanceof <Module>(child)) {
      auto mod = (Module *)child;
      for (auto d : mod->items) {
        if (instanceof <Method>(d)) {
          auto method = (Method *)d;
          auto dec = new MethodPredeclaration;
          dec->sig = method->sig;
          items.push_back(dec);
        }
      }
    }
  }
  for (auto i : items) {
    pushf(unit.items, i);
  }
}