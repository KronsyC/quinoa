#include "./llvm_utils.h"
#include "../lib/error.h"
#include "../lib/logger.h"
#include "./AST/type.hh"
static llvm::LLVMContext _ctx;
static llvm::IRBuilder<> _builder(_ctx);

llvm::LLVMContext* llctx()
{
    return &_ctx;
}
llvm::IRBuilder<>* builder()
{
    return &_builder;
}

bool isInt(LLVMType t)
{
    return t->isIntegerTy();
}
llvm::Value* cast(llvm::Value* val, LLVMType type)
{

    if(type.ll_type == nullptr)
	return val;
    auto tape = val->getType();
    if(type == tape)
	return val;

    if(isInt(LLVMType{tape, nullptr}) && isInt(type)){
        Logger::debug("Int -> Int");
        bool is_signed = true;
        if(tape->isIntegerTy(1))is_signed = false;
        return builder()->CreateIntCast(val, type, is_signed, "int_cast");

    }
    if(tape->isPointerTy() && type->isIntegerTy())
	return builder()->CreatePtrToInt(val, type);

    if(tape->isPointerTy() && type->isPointerTy()) {
	return builder()->CreateBitCast(val, type);
    }
    Logger::debug("val type: ");
    tape->print(llvm::outs());
    printf("\n");
    Logger::debug("type: ");
    type->print(llvm::outs());
    printf("\n");
    except(E_BAD_CAST, "Failed to cast between types");
    return nullptr;
}
