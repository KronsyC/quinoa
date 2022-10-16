#pragma once
#include "../processor.h"

using namespace std;


void hoistDefinitions(CompilationUnit &unit) {

  for(auto method:unit.getAllMethods()){
    if(!method->generate)continue;
    auto dec = new MethodPredeclaration(method->sig);
    pushf(unit, (TopLevelExpression*)dec);
  }
}