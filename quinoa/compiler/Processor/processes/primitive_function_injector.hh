#pragma once

//
//	This Process is responsible for injecting all of the primitive functions into the ast
// 	(denoted as dunder methods)
// 	Primitive Functions include __syscall__ and __cast__<T>
//

#include "../processor.h"

void injectPrimitiveFunctions()