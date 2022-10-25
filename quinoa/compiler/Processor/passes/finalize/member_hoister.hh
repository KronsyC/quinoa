
/**
 * 
 * Predeclare / Hoist all methods and properties
 * This acts as implicit forward-declaration of everything
*/


#pragma once
#include "../include.h"

using namespace std;


void hoist_definitions(CompilationUnit &unit) {

  for(auto method:unit.getAllMethods()){
    if(!method->generate()){
      continue;
    }
    auto dec = new MethodPredeclaration(method);
    pushf(unit, (TopLevelExpression*)dec);
  }

  for(auto property:unit.getAllProperties()){
    auto dec = new PropertyPredeclaration(property);
    pushf(unit, (TopLevelExpression*)dec);
  }
}