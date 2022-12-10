#include "./primary.hh"
#include "./type.hh"
#include "./reference.hh"
#include "./container.hh"
LLVMType::LLVMType(std::shared_ptr<Type> qn_type){
    this->qn_type = qn_type;
}

llvm::Type * LLVMType::operator->(){
  return get_type();
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

LLVMType::operator llvm::Type*() const{
  return get_type();
}

llvm::Type* LLVMType::get_type() const{
  if(!is_explicitly_constructed)except(E_INTERNAL, "Attempted to get type of implcitly constructed LLVMType");
  if(!cached_type)return qn_type->llvm_type();
  return cached_type;
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
    except(E_INTERNAL, "Can only load pointers or references, but the value was found to be of type: " + type.qn_type->str());

}

void LLVMValue::print(){
    this->val->print(llvm::outs());
    Logger::debug(" < LLVM Value of type: " + this->type.qn_type->str());
}

std::string ContainerRef::str(){
    return this->name->str();
}

std::string ContainerRef::mangle_str(){
  if(!this->refers_to)except(E_INTERNAL, "cannot generate mangle_str from unresolved module member");
  return this->refers_to->full_name().str();
}
bool LLVMType::operator==(LLVMType& other) const{
    return other.qn_type->drill() == qn_type->drill();
}

std::vector<Type*> Scope::flatten_types(){
  std::vector<Type*> ret;

  for(auto member : this->content){
    for(auto t : member->flatten_types()){
      ret.push_back(t);
    }
  }
  return ret;
}

#include "../llvm_utils.h"
void Scope::decl_new_variable(std::string name, std::shared_ptr<Type> type, bool is_constant){
  this->vars[name] = std::make_unique<Variable>(type, nullptr, is_constant);
}


std::shared_ptr<Type> Container::get_type(std::string name){
        for (auto &member: members) {
            if (auto type = dynamic_cast<TypeMember *>(member.ptr)) {
                if (type->name->member->str() == name)return type->refers_to;
            }
        }

        if (name == this->name->str()) {
            return get_type();
        }


        // try each compositor sequentially until a match is found
        for(auto cmp : compositors){
          if(includes(NATIVE_MODULES, cmp->name->str()))continue;
           auto cmp_mod = cmp->refers_to;
          
          if(!cmp_mod)except(E_INTERNAL, "unresolved compositor: " + cmp->name->str());
          if(auto typ = cmp_mod->get_type(name))return typ;
        }
        return std::shared_ptr<Type>(nullptr);
}
