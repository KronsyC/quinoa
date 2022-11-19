#pragma once
#include "./primary.hh"
#include "./include.hh"

class AllocatingExpr : public Expr{
public:
    llvm::Value* llvm_value(VariableTable& vars, llvm::Type* expected_type = nullptr){
        auto alloca = builder()->CreateAlloca(this->type()->llvm_type());
        write_direct(alloca, vars, expected_type);
        return builder()->CreateLoad(this->type()->llvm_type(), alloca);
    }

    virtual void write_direct(llvm::Value* alloc, VariableTable& vars, llvm::Type* expected_type = nullptr) = 0;
};