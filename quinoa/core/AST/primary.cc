#include "./primary.hh"
#include "./type.hh"
#include "./reference.hh"
#include "./container.hh"
LLVMType::LLVMType(std::shared_ptr<Type> qn_type){
    this->qn_type = qn_type;
    this->ll_type = qn_type->llvm_type().ll_type;
}


bool LLVMType::is_signed() {
    if(auto prim = qn_type->get<Primitive>()){
        #define X(knd)if(prim->kind == knd)return true;

        X(PR_int8)
        X(PR_int16)
        X(PR_int32)
        X(PR_int64)

        X(PR_float16)
        X(PR_float32)
        X(PR_float64)

    }
    return false;
}

LLVMValue LLVMValue::load(){
    // must be a pointer

    if(auto ptr_ty = this->type.qn_type->get<Ptr>()){
        auto loaded_ty = ptr_ty->of->llvm_type();
        auto loaded_val = builder()->CreateLoad(loaded_ty, val);

        return {loaded_val, loaded_ty};
    }

    if(auto ref_ty = this->type.qn_type->get<ReferenceType>()){
        auto loaded_ty = ref_ty->of->llvm_type();
        auto loaded_val = builder()->CreateLoad(loaded_ty, val);

        return {loaded_val, loaded_ty};
    }
    print();
    except(E_INTERNAL, "Can only load pointers or references");

}

void LLVMValue::print(){
    this->val->print(llvm::outs());
    Logger::debug(" < LLVM Value of type: " + this->type.qn_type->str());
}

std::string ContainerRef::str(){
    if(this->refers_to)return refers_to->full_name().str();
    else return this->name->str();
}
