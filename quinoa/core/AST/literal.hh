#pragma once

#include "./include.hh"

#include "./type.hh"

class ArrayLiteral : public Expr {
  public:
    Vec<Expr> members;

    std::vector<Type*> flatten_types() {
        std::vector<Type*> ret;

        for (const auto& m : members) {
            for (auto t : m->flatten_types()) {
                ret.push_back(t);
            }
        }
        return ret;
    }
    void write_to(LLVMValue alloc, VariableTable& vars) {
        if (!alloc->getType()->getPointerElementType()->isArrayTy())
            except(E_BAD_ASSIGNMENT, "You can only write an array literal to a list typed variable");

        auto tgt_member_t = LLVMType(alloc.type.qn_type->pointee()->get<ListType>()->of);

        int idx = 0;
        for (auto member : members) {
            auto ep = builder()->CreateConstGEP2_32(alloc->getType()->getPointerElementType(), alloc, 0, idx);
            auto val = member->llvm_value(vars, tgt_member_t);
            builder()->CreateStore(val, ep);
            idx++;
        }
    }

    LLVMValue llvm_value(VariableTable& vars, LLVMType expected_type = {}) {

        auto my_type = expected_type ? expected_type : type()->llvm_type();
        auto alloca = LLVMValue{builder()->CreateAlloca(my_type), LLVMType(Ptr::get(my_type.qn_type))};
        if (expected_type && !expected_type->isArrayTy())
            except(E_BAD_CAST, "Cannot cast an array to a non-array type");
        this->write_to(alloca, vars);

        return alloca.load();
    }

    LLVMValue assign_ptr(VariableTable& vars) { except(E_BAD_ASSIGNMENT, "Array Literals are not assignable"); }

    std::string str() {
        std::string ret = "[ ";
        bool first = true;
        for (auto member : members) {
            if (!first)
                ret += ", ";
            ret += member->str();
            first = false;
        }
        return ret + " ]";
    }

    std::vector<Statement*> flatten() {
        std::vector<Statement*> ret = {this};
        for (auto r : members)
            for (auto m : r->flatten())
                ret.push_back(m);
        return ret;
    }

  private:
    _Type get_type() {
        TypeVec member_types;
        for (auto m : members) {
            member_types.push_back(m->type());
        }
        if (!member_types[0])
            return member_types[0];
        return ListType::get(member_types[0], Integer::get(members.len()));
    }
};
