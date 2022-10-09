#pragma once
#include "../processor.h"

using namespace std;


void hoistDefinitions(CompilationUnit &unit) {

  for(auto method:unit.getAllMethods()){
    auto dec = new MethodPredeclaration;
    dec->sig = method->sig;
    pushf(unit.items, (TopLevelExpression*)dec);
  }
}