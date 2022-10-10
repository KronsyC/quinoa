#include "./llvm_globals.h"

static llvm::LLVMContext _ctx;
static llvm::IRBuilder<> _builder(_ctx);


llvm::LLVMContext* ctx(){
    return &_ctx;
}
llvm::IRBuilder<>* builder(){
    return &_builder;
}