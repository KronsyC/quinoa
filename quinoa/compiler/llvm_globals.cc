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

    // if(tape->isArrayTy() && type->isPointerTy()){
    //   auto alloc = builder()->CreateAlloca(tape);
    //   builder()->CreateStore(val, alloc);
    //   return builder()->CreateBitCast(alloc, type);
    // }
    if(tape->isArrayTy() && type->isArrayTy()){
      return builder()->CreateBitCast(val, type);
      auto myElType = tape->getArrayElementType();
      auto tgtElType = type->getArrayElementType();
    }
    if(tape->isArrayTy() && type->isIntegerTy()){
      Logger::debug("cast array element to int");
      int i = 0;
      auto extr = builder()->CreateExtractElement(val, i);
      return extr;
    }
    
    Logger::debug("val type: ");
    tape->print(llvm::outs());
    printf("\n");
    Logger::debug("type: ");
    type->print(llvm::outs());
    printf("\n");
    error("Failed to cast", true);
    return nullptr;
}