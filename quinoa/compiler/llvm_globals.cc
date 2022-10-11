#include "./llvm_globals.h"
#include "../lib/error.h"
#include "../lib/logger.h"
static llvm::LLVMContext _ctx;
static llvm::IRBuilder<> _builder(_ctx);


llvm::LLVMContext* ctx(){
    return &_ctx;
}
llvm::IRBuilder<>* builder(){
    return &_builder;
}

bool isInt(llvm::Type *t)
{
return t->isIntegerTy();
}
llvm::Value *cast(llvm::Value *val, llvm::Type *type)
  {
    if (type == nullptr)
      return val;
    auto tape = val->getType();
    if(type==tape)return val;
    if (isInt(tape) && isInt(type))
      return builder()->CreateIntCast(val, type, true);
    if(tape->isPointerTy() && type->isIntegerTy())
        return builder()->CreatePtrToInt(val, type);
    Logger::debug("val type: ");
    tape->print(llvm::outs());
    printf("\n");
    Logger::debug("type: ");
    type->print(llvm::outs());
    printf("\n");
    error("Failed to cast");
    return nullptr;
}
llvm::Type *getCommonType(llvm::Type *t1, llvm::Type *t2)
  {
    if (t1 == nullptr || t2 == nullptr)
      error("one of the types is null");
    if (t1 == t2)
      return t1;
    // int casting
    int size1 = t1->getPrimitiveSizeInBits();
    int size2 = t2->getPrimitiveSizeInBits();
    if (isInt(t1) && isInt(t2))
      return builder()->getIntNTy(std::max(size1, size2));

    t1->print(llvm::outs());
    t2->print(llvm::outs());
    error("Failed To Get Common Type");
    return nullptr;
}