#pragma once
#include "../llvm_globals.h"
#include <map>
#include <string>
#define LocalTypeTable std::map<std::string, Type*>

#define printTypeTable(tt)                        \
    for(auto pair : tt) {                         \
	std::string message = pair.first + " : "; \
	if(pair.second == nullptr)                \
	    message += "unknown";                 \
	else                                      \
	    message += pair.second->str();        \
	Logger::debug(message);                   \
    }

template <typename Base, typename T> inline bool instanceof (T * item)
{
    return dynamic_cast<Base*>(item);
}

#include "../../lib/logger.h"

#include "./base.hh"
#include "./identifier.hh"

Type* getCommonType(Type* t1, Type* t2, bool second_pass = false);
Type* getCommonType(std::vector<Expression*> items);
Type* getCommonType(std::vector<Type*> items);

#include "./module.hh"
class Property;
class MethodCall;
struct CompilationUnit;
class MethodSignature;
llvm::StructType* structify(Module* mod);
size_t getModuleMemberIdx(TLContainer* container, std::string name);

Property* getProperty(TLContainer* container, std::string propname);
Method* getMethod(TLContainer* container, MethodCall* call);
MethodSignature* getMethodSig(TLContainer* container, MethodCall* call);
#include "./module_member.hh"
#include "./type.hh"

#include "./compilation_unit.hh"
#include "./constant.hh"
#include "./control_flow.hh"
#include "./operations.hh"
