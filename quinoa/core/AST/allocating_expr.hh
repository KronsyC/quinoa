#pragma once

#include "./primary.hh"
#include "./include.hh"

class AllocatingExpr : public Expr {
public:
    LLVMValue llvm_value(VariableTable &vars, LLVMType expected_type = {}) {
        auto alloca = builder()->CreateAlloca(this->type()->llvm_type(), nullptr, ".tmp");
        auto alloca_value = LLVMValue(alloca, LLVMType(Ptr::get(type())));
        write_direct(alloca_value, vars, expected_type);
        return alloca_value.load();
    }

    virtual void write_direct(LLVMValue alloc, VariableTable &vars, LLVMType expected_type = {}) = 0;
};