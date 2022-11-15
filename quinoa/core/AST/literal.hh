#pragma once
#include "./include.hh"

#include "./type_utils.h"
#include "./type.hh"

class ArrayLiteral : public Expr{
public:
    Vec<Expr> members;

    void write_to(llvm::Value* alloc, VariableTable& vars){
        if(!alloc->getType()->getPointerElementType()->isArrayTy())except(E_BAD_ASSIGNMENT, "You can only write an array literal to a list typed variable");


        auto tgt_member_t = alloc->getType()->getPointerElementType()->getArrayElementType();

        int idx = 0;
        for(auto member : members){
            auto ep = builder()->CreateConstGEP2_32(alloc->getType()->getPointerElementType(), alloc, 0, idx);
            auto val = member->llvm_value(vars, tgt_member_t);
            builder()->CreateStore(val, ep);
            idx++;
        }
    }
    llvm::Value* llvm_value(VariableTable& vars, llvm::Type* expected_type = nullptr){
        auto my_type = type()->llvm_type();
        auto alloca = builder()->CreateAlloca(my_type);
        this->write_to(alloca, vars);
        return builder()->CreateLoad(my_type, alloca);
    }
    llvm::Value* assign_ptr(VariableTable& vars){
        except(E_BAD_ASSIGNMENT, "Array Literals are not assignable");
    }

    std::string str(){
        std::string ret = "[ ";
        bool first = true;
        for(auto member : members){
            if(!first)ret+=", ";
            ret += member->str();
            first = false;
        }
        return ret + " ]";
    }
    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        for(auto r : members)for(auto m : r->flatten())ret.push_back(m);
        return ret;
    }
private:
    std::shared_ptr<Type> get_type() {
        std::vector<std::shared_ptr<Type>> member_types;
        for(auto m : members){
            member_types.push_back(m->type());
            Logger::debug("member: " + m->type()->str());
        }
        auto common = TypeUtils::get_common_type(member_types);
        if(!common)except(E_BAD_TYPE, "Failed to get common type for array members");
        return ListType::get(common, Integer::get(members.len()));
    }

};