#pragma once

//
//	This Process is responsible for injecting all of the primitive functions into the ast
// 	(denoted as dunder methods)
// 	Primitive Functions include __syscall__ and __cast__<T>
//  they are all defined in the lang.primitives module
//

#include "../processor.h"

void injectPrimitiveFunctions(CompilationUnit& unit){
    
}