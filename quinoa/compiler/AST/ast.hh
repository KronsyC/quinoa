#pragma once
#include <map>
#include <string>
#include "../llvm_globals.h"
#define LocalTypeTable std::map<std::string, Type*>

#define printTypeTable(tt) \
for(auto pair:tt){ \
    std::string message = pair.first + " : "; \
    if(pair.second == nullptr)message+="unknown"; \
    else message+=pair.second->str(); \
    Logger::debug(message); \
} \


template <typename Base, typename T>
inline bool instanceof (T * item)
{
    return dynamic_cast<Base *>(item) ;
}


#include "../../lib/logger.h"

#include "./base.hh"
#include "./identifier.hh"


Type *getCommonType(Type *t1, Type *t2, bool second_pass = false);
Type *getCommonType(std::vector<Expression *> items);
Type *getCommonType(std::vector<Type *> items);



#include "./module.hh"
class Property;
class MethodCall;
struct CompilationUnit;
class MethodSignature;
llvm::StructType* structify(Module* mod);
size_t getModuleMemberIdx(Module* mod, std::string name);

Property* getProperty(Module* mod, std::string propname);
Method* getMethod(Module* mod,  MethodCall* call);
MethodSignature *getMethodSig(Module *mod, MethodCall *call);
#include "./type.hh"
#include "./module_member.hh"

#include "./compilation_unit.hh"
#include "./constant.hh"
#include "./operations.hh"
#include "./control_flow.hh"


