#pragma once
#include "../processor.h"

using namespace std;


void hoistDefinitions(CompilationUnit &unit) {

  for(auto method:unit.getAllMethods()){
    auto dec = new MethodPredeclaration(method->sig);
    pushf(unit, (TopLevelExpression*)dec);
  }
}