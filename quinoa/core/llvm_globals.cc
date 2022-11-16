#include "./llvm_globals.h"
#include "../lib/error.h"
#include "../lib/logger.h"
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

bool isInt(llvm::Type* t)
{
    return t->isIntegerTy();
}
llvm::Value* cast(llvm::Value* val, llvm::Type* type)
{

    if(type == nullptr)
	return val;
    auto tape = val->getType();
    if(type == tape)
	return val;

    if(isInt(tape) && isInt(type))
	return builder()->CreateIntCast(val, type, true);
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
