/**
 *
 * Advanced operators
 *
 */

#pragma once

#include "./primary.hh"
#include "./type.hh"
#include "./container.hh"
#include "./include.hh"
#include "./allocating_expr.hh"
#include <llvm/IR/DerivedTypes.h>


class CallLike : public Expr {
public:
    Vec<Expr> args;
    std::vector <std::shared_ptr<Type>> type_args;
    Method *target = nullptr;


    LLVMValue create_call(VariableTable &vars, std::vector<llvm::Value*> injected_args, std::vector<std::shared_ptr<Type>> second_type_args = {}) {

        std::vector <llvm::Value*> args;
        llvm::Value* ret_val_if_param = nullptr;

        target->apply_generic_substitution(this->type_args, second_type_args);


        if(target->must_parameterize_return_val()){
            auto alloc_ty = target->return_type;
            auto alloc = create_allocation(target->return_type->llvm_type(), builder()->GetInsertBlock()->getParent());
            args.push_back(alloc);
            ret_val_if_param =alloc;
        }
        for(auto a : injected_args)args.push_back(a);
        auto mod = builder()->GetInsertBlock()->getModule();
        auto fn = mod->getFunction(target->source_name());

        if(target->is_generic()){
  
          fn = make_fn(*target, mod, llvm::GlobalValue::LinkageTypes::ExternalLinkage, true);

          GenericImpl impl;
          std::vector<std::shared_ptr<Type>> persist_type_args;
          std::vector<std::shared_ptr<Type>> persist_second_type_args;
          for(auto t : type_args){
            persist_type_args.push_back(t->clone_persist());
          }

          for(auto t : second_type_args){
            persist_second_type_args.push_back(t->clone_persist());
          }
          impl.method_generic_args = persist_type_args;
          impl.target_generic_args = persist_second_type_args;
          target->generate_usages.push(impl);
        }
        if (!fn)except(E_BAD_CALL, "Failed to load function for call: " + target->source_name());


        for (size_t i = 0; i < this->args.len(); i++) {
            auto &arg = this->args[i];
            auto param = target->get_parameter(i);

            auto expected_type = param->type->llvm_type();
            auto arg_val = cast(arg.llvm_value(vars, expected_type), expected_type);
            args.push_back(arg_val);
        }

        auto call = builder()->CreateCall(fn, args);

        
        auto ret = LLVMValue{ret_val_if_param ? (llvm::Value*)builder()->CreateLoad(ret_val_if_param->getType()->getPointerElementType(), (llvm::Value*)ret_val_if_param) : call, target->return_type->clone_persist()->llvm_type()};
        // target->undo_generic_substitution();  
        return ret;  
  }
};

class MethodCall : public CallLike {
public:
    std::unique_ptr <ContainerMemberRef> name;


    std::string str() {
        std::string ret = name->str();

        if (type_args.size()) {
            ret += "<";
            bool first = true;
            for (auto ta: type_args) {
                if (!first)
                    ret += ", ";
                ret += ta->str();
                first = false;
            }
            ret += ">";
        }
        ret += "(";
        bool first = true;
        for (auto &a: args) {
            if (!first)
                ret += ", ";
            ret += a->str();
            first = false;
        }
        ret += ")";
        return ret;
    }

    LLVMValue llvm_value(VariableTable &vars, LLVMType expected_type = {}) {

        auto call = create_call(vars, {});
        return cast(call, expected_type);
    }

    std::vector<Statement *> flatten() {
        std::vector < Statement * > ret = {this};
        for (auto a: args)for (auto m: a->flatten())ret.push_back(m);
        return ret;
    }

    LLVMValue assign_ptr(VariableTable &vars) {
        // an assign_ptr may only be generated for calls which internally
        // pass a reference to the function, which is abstracted as a return value
        if(!target->must_parameterize_return_val())except(E_BAD_OPERAND, "Indirection is only allowed on functions which return Arrays, Slices, or Structs");
        except(E_INTERNAL, "assign_ptr not implemented for MethodCall: " + str());
    }

    std::vector<Type*> flatten_types(){
        std::vector<Type*> ret;
    
        for(const auto& ta : type_args){
            for(const auto& t : ta->flatten()){
                ret.push_back(t);
            }
        }

        for(const auto& val : args){
            for(const auto& t : val->flatten_types()){
                ret.push_back(t);
            }
        }

        return ret;
    }
protected:
    std::shared_ptr <Type> get_type() {


        Logger::debug("Get return type of method call: " + str()); 
        // if any of the type args are unresolved generics 
        // simply return the result of a generic call
        // otherwise return cloned substituted ret type
    
        if (!target)return std::shared_ptr<Type>(nullptr);

        target->apply_generic_substitution(this->type_args);

        auto ret = target->return_type->copy_with_substitutions({});

        target->undo_generic_substitution();
        return ret;
    }

    bool is_generic_type_args(){
      for(auto ta : type_args){
        if(ta->get<Generic>())return true;
      }
      return false;
    }
};

class MethodCallOnType : public CallLike {
public:
    std::unique_ptr <Name> method_name;
    std::unique_ptr <Expr> call_on;


    unsigned deref_count = 0;
    std::vector<Type*> flatten_types(){
        std::vector<Type*> ret;
    
        for(const auto& ta : type_args){
            for(const auto& t : ta->flatten()){
                ret.push_back(t);
            }
        }

        for(const auto& val : args){
            for(const auto& t : val->flatten_types()){
                ret.push_back(t);
            }
        }

        for(const auto& t : call_on->flatten_types())ret.push_back(t);

        return ret;
    }


    std::string str() {
        std::string ret = call_on->str() + "." + method_name->str();
        if (type_args.size()) {
            bool first = true;
            ret += "<";
            for (auto &ta: type_args) {
                if (!first)
                    ret += ", ";
                ret += ta->str();
                first = false;
            }
            ret += ">";
        }

        ret += "(";
        bool first = true;
        for (auto p: args) {
            if (!first)ret += ", ";
            ret += p->str();
            first = false;

        }
        ret += ")";
        return ret;
    }

    LLVMValue llvm_value(VariableTable &vars, LLVMType expected_type = {}) {

        Logger::debug("Calling method of type: " + this->call_on->type()->str());
        auto ptr = call_on->assign_ptr(vars);
        auto tmp_ref_count = deref_count;
        while(tmp_ref_count){
          ptr = ptr.load();
          tmp_ref_count--;
        }

        std::vector<std::shared_ptr<Type>> targ_type_args = {};
        if(auto ptref = ptr.type.qn_type->pointee()->get<ParameterizedTypeRef>()){
          targ_type_args = ptref->params;
        } 
        auto call = create_call(vars, {ptr}, targ_type_args);
        return cast(call, expected_type);
    }

    std::vector<Statement *> flatten() {
        std::vector < Statement * > ret = {this};

        for (auto a: args)for (auto m: a->flatten())ret.push_back(m);
        for (auto f: call_on->flatten())ret.push_back(f);
        return ret;

    }

    LLVMValue assign_ptr(VariableTable &vars) {
        except(E_INTERNAL, "assign_ptr not implemented for MethodCallOnType");
    }

protected:
    std::shared_ptr <Type> get_type() {
        if (!target)return std::shared_ptr<Type>(nullptr);
        if (target == (Method*) 1) {
            // Handle compiler implemented methods
            if (auto ref = call_on->type()->get<ReferenceType>()) {
                #define X(n, ret)if(method_name->str() == #n)return ret;
                X(len, Primitive::get(PR_int64))
                X(as_ptr, ref->of)
                #undef X
            }

        }

        auto cot = call_on->type();
        std::shared_ptr<Type> ret_t;
        Logger::debug("Call on: " + cot->str() + " > " + call_on->str());
        if(auto ptref = cot->get<ParameterizedTypeRef>()){
          target->apply_generic_substitution(this->type_args, ptref->params);
          ret_t = target->return_type->clone_persist();
      
        }
        else{
          target->apply_generic_substitution(this->type_args);
          ret_t = target->return_type->drill()->copy_with_substitutions({});
        }
        target->undo_generic_substitution();
        return ret_t;
    }
};

class Return : public Statement {
public:
    std::unique_ptr <Expr> value;

    std::string str() {
        return "return " + value->str();
    }

    void generate(Method *qn_fn, llvm::Function *func, VariableTable &vars, ControlFlowInfo CFI) {
        if (value) {
            Logger::debug("fn returns: " + qn_fn->return_type->str());
            Logger::debug("Return: " + value->type()->str());
            auto return_value = value->llvm_value(vars, qn_fn->return_type);

            if(qn_fn->must_parameterize_return_val()){
                auto write_to = func->getArg(0);
                builder()->CreateStore(return_value, write_to);
            }
            else{
                builder()->CreateRet(return_value);

            }
        } else {
            builder()->CreateRetVoid();
        }
    }

    std::vector<Statement *> flatten() {
        std::vector < Statement * > ret = {this};
        if (value)for (auto m: value->flatten())ret.push_back(m);
        return ret;
    }
    
    std::vector<Type*> flatten_types(){
        return value ? value->flatten_types() : std::vector<Type*>();
    }

    ReturnChance returns() {
        return ReturnChance::DEFINITE;
    }
};

class InitializeVar : public Statement {
public:
    std::shared_ptr <Type> type;
    Name var_name;
    std::unique_ptr <Expr> initializer;
    bool is_constant = false;

    std::vector<Statement *> flatten() {
        std::vector < Statement * > ret = {this};
        if (initializer)for (auto m: initializer->flatten())ret.push_back(m);
        return ret;
    }

    std::vector<Type*> flatten_types(){
      auto ret = type->flatten();
      if(initializer)for(auto t : initializer->flatten_types())ret.push_back(t);
     return ret;
    }

    std::string str() {
        std::string ret = is_constant ? "const " : "let ";
        ret += var_name;
        if (type) {
            ret += " : " + type->str();
        }
        if (initializer) {
            ret += " = " + initializer->str();
        }
        return ret;
    }

    void generate(Method *qn_fn, llvm::Function *func, VariableTable &vars, ControlFlowInfo CFI) {

        Logger::debug("INITIALIZE: " + var_name.str());
        Logger::debug("init of type: " + type->str());
        auto copy = type->copy_with_substitutions(qn_fn->generics_as_table());
        Logger::debug("now: " + copy->str());
        auto ll_type = copy->llvm_type();
        ll_type->print(llvm::outs());
        auto name = var_name.str();

        auto alloc = create_allocation(ll_type, func);
        alloc->setName(name);
    
        vars[name] = Variable(type, alloc, is_constant);
    }

    ReturnChance returns() {
        return ReturnChance::NEVER;
    }
};

class ExplicitCast : public Expr {
public:
    std::unique_ptr <Expr> value;
    std::shared_ptr <Type> cast_to;

    std::vector<Statement *> flatten() {
        std::vector < Statement * > ret = {this};
        for (auto m: value->flatten())ret.push_back(m);
        return ret;
    }

    std::vector<Type*> flatten_types(){
        auto ret = value->flatten_types();

        for(auto t : cast_to->flatten())ret.push_back(t);

        return ret;
    }

    LLVMValue llvm_value(VariableTable &vars, LLVMType expected_type = {}) {
        return cast(cast_explicit(value->llvm_value(vars), cast_to), expected_type);
    }

    std::string str() {
        return value->str() + " as " + cast_to->str();
    }

    std::shared_ptr <Type> get_type() {
        return cast_to;
    }

    LLVMValue assign_ptr(VariableTable &vars) {


        except(E_BAD_ASSIGNMENT, "Cannot assign a value to an explicitly casted value");
    }
};

// Similar to the ExplicitCast instruction
// but can only be used internally

class ImplicitCast : public Expr {
public:
    std::unique_ptr <Expr> value;
    std::shared_ptr <Type> cast_to;

    std::vector<Type*> flatten_types(){
        auto ret = value->flatten_types();

        for(auto t : cast_to->flatten())ret.push_back(t);

        return ret;
    }

    std::vector<Statement *> flatten() {
        std::vector < Statement * > ret = {this};
        for (auto m: value->flatten())ret.push_back(m);
        return ret;
    }
    ImplicitCast(std::unique_ptr<Expr> val, std::shared_ptr<Type> cast_to){
        this->value = std::move(val);
        this->cast_to = cast_to;
        this->scope = this->value->scope;
    }
    LLVMValue llvm_value(VariableTable &vars, LLVMType expected_type = {}) {
        Logger::debug("Implicit cast: " + value->str() +"("+value->type()->str()+") to " + cast_to->str() + "; and finally to " + (expected_type ? expected_type.qn_type->str() : std::string("?")));
        auto val = value->llvm_value(vars, cast_to);
        return cast(val, expected_type);
    }

    std::string str() {
        return value->str() + " => " + cast_to->str();
    }

    std::shared_ptr <Type> get_type() {
        return cast_to;
    }

    LLVMValue assign_ptr(VariableTable &vars) {


        except(E_BAD_ASSIGNMENT, "Cannot assign a value to an implicitly casted value");
    }
};
class StructInitialization : public AllocatingExpr {
public:
    std::map <std::string, std::unique_ptr<Expr>> initializers;
    std::shared_ptr<Type> type;


    std::vector<Type*> flatten_types(){
        auto ret = type->flatten();

        for(auto& [_, v] : initializers){
            for(auto t : v->flatten_types() )ret.push_back(t);
        }


        return ret;
    }

    StructInitialization(std::shared_ptr<Type> tgt) {
        this->type = tgt;
    }

    std::vector<Statement *> flatten() {
        std::vector < Statement * > ret = {this};
        for (auto &i: initializers)for (auto m: i.second->flatten())ret.push_back(m);
        return ret;
    }

    std::string str() {
        std::string ret = type->str() + " :: {";
        for(auto& i : initializers){
            ret+="\n\t"+i.first+" : " + i.second->str() + ";";
        }
        ret+="\n}";
        return ret;
    }

    void write_direct(LLVMValue alloc, VariableTable &vars, LLVMType expected_type = {}) {

        auto struct_ty = type->get<StructType>();
        GenericTable generics;


        if(auto pref_ty = type->get<ParameterizedTypeRef>()){

            struct_ty = pref_ty->resolves_to->get<StructType>();

            if(!struct_ty)except(E_BAD_TYPE, "Struct initialization expressions require either a struct, or parameterized struct type, but the type was found to be: " + pref_ty->str());

            generics = pref_ty->get_mapped_params();

        }


        if(struct_ty){
            auto lltype = type->llvm_type(generics);

            // ensure that all members are explicitly initialized
            for (auto [name, ty]: struct_ty->members) {
                auto &lookup = initializers[name];
                if (!lookup){
                    except(E_BAD_ASSIGNMENT,
                           "Initialization for struct '" + type->str() + "' is missing a member: '" + name + "' of type " + ty->str()
                    );
                }

            }

            for(auto pair : generics){
              Logger::debug(pair.first + " |=> " + pair.second->str());
            }

            for(const auto& init : initializers){
                auto member_idx = struct_ty->member_idx(init.first);
                if (member_idx == -1)except(E_BAD_ASSIGNMENT, "Bad Struct Key: " + init.first);

                auto member_ty = struct_ty->members[init.first]->copy_with_substitutions(generics)->llvm_type();

                member_ty->print(llvm::outs());
                Logger::debug(init.first + " is a " + member_ty.qn_type->str());

                auto init_value = init.second->llvm_value(vars, member_ty);

                alloc.type->print(llvm::outs());
                lltype->print(llvm::outs());

                auto member_ptr = builder()->CreateStructGEP(lltype, alloc, member_idx);

                builder()->CreateStore(init_value, member_ptr);
            }
        }
        else{
            except(E_BAD_TYPE, "cannot use the struct initialisation expression on non-struct types");

        }



    }

    LLVMValue assign_ptr(VariableTable &vars) {
        except(E_BAD_ASSIGNMENT, "Struct Initializers are not assignable");
    }

    std::shared_ptr <Type> get_type() {
        return type;
    }
};

class Subscript : public Expr {
public:
    std::unique_ptr <Expr> target;
    std::unique_ptr <Expr> index;

    Subscript(std::unique_ptr <Expr> tgt, std::unique_ptr <Expr> idx) {
        this->target = std::move(tgt);
        this->index = std::move(idx);
    }

    std::vector<Type*> flatten_types(){
        auto ret = target->flatten_types();
        for(auto t : index->flatten_types())ret.push_back(t);

        return ret;
    }

    std::vector<Statement *> flatten() {
        std::vector < Statement * > ret = {this};
        for (auto m: index->flatten())ret.push_back(m);
        return ret;
    }

    std::string str() {
        return target->str() + "[" + index->str() + "]";
    }

    LLVMValue llvm_value(VariableTable &vars, LLVMType expected_type = {}) {
        auto ptr = assign_ptr(vars);
        return cast(ptr.load(), expected_type);
    }

    LLVMValue assign_ptr(VariableTable &vars) {

        // Returns a pointer to the item at the desired index

        auto ptr = target->assign_ptr(vars);
        auto tgt_ty = target->type();
        auto tgt_ll_ty = target->type()->llvm_type();

        auto idx = index->llvm_value(vars, Primitive::get(PR_uint64)->llvm_type());

        auto ptr_to_element = LLVMType(Ptr::get(target->type()->pointee()));

        auto zero = Integer::get(0)->const_value(index->type()->llvm_type());

        if (tgt_ty->get<ListType>()) {
            // Subscripts for arrays may only be integers, iX;
            if (!index->type()->llvm_type()->isIntegerTy())
                except(E_BAD_INDEX, "The Index of an array subscript expression must be an integer");


            generate_bounds_check(idx, builder()->getInt64(ptr->getType()->getPointerElementType()->getArrayNumElements()));


            auto ep = builder()->CreateGEP(ptr->getType()->getPointerElementType(), ptr, {zero, idx});
            return {ep, ptr_to_element};

        }
        else if (tgt_ty->get<DynListType>()) {
            auto len_ptr = builder()->CreateStructGEP(tgt_ll_ty, ptr, 0);
            auto len = builder()->CreateLoad(builder()->getInt64Ty(), len_ptr);



            generate_bounds_check(idx, len);

            auto internal_arr_ptr = builder()->CreateStructGEP(tgt_ll_ty, ptr, 1);
            auto arr = builder()->CreateLoad(internal_arr_ptr->getType()->getPointerElementType(), internal_arr_ptr);

            auto item_ptr = builder()->CreateGEP(arr->getType()->getPointerElementType(), arr, {zero, idx});

            item_ptr->print(llvm::outs());
            return {item_ptr, ptr_to_element};


        } else if (ptr->getType()->getPointerElementType()->isPointerTy()) {
            auto val = builder()->CreateLoad(ptr->getType()->getPointerElementType(), ptr);
            auto ep = builder()->CreateGEP(val->getType()->getPointerElementType(), val, idx);
            return {ep, ptr_to_element};
        } else{
            ptr.print();
            except(E_BAD_OPERAND, "You may only access subscripts of an array type, operand was found to be of type: " + tgt_ty->str());
        }
    }

    std::shared_ptr <Type> get_type() {
        if (!target)except(E_INTERNAL, "subscript bad target");
        if (!target->type())return target->type();
        return target->type()->pointee();
    }

private:
    void generate_bounds_check(LLVMValue access_idx, llvm::Value* array_len_val) {
        auto func = builder()->GetInsertBlock()->getParent();
        auto mod = func->getParent();
        auto exception_block = llvm::BasicBlock::Create(*llctx(), "bounds_check_err", func);
        auto continue_block = llvm::BasicBlock::Create(*llctx(), "bounds_check_cont", func);

        auto is_valid_access = builder()->CreateICmpSLT(access_idx, array_len_val);

        builder()->CreateCondBr(is_valid_access, continue_block, exception_block);

        builder()->SetInsertPoint(exception_block);

        // Requires both write() and abort() from libc, ensure they are present

        auto write_sig = llvm::FunctionType::get(builder()->getInt32Ty(),
                                                 {builder()->getInt32Ty(), builder()->getInt8Ty()->getPointerTo(),
                                                  builder()->getInt64Ty()}, false);
        auto abort_sig = llvm::FunctionType::get(builder()->getVoidTy(), {}, false);

        auto write_fn = mod->getFunction("write") ? mod->getFunction("write") : llvm::Function::Create(write_sig,
                                                                                                       llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                                                                                       "write", mod);
        auto abort_fn = mod->getFunction("abort") ? mod->getFunction("abort") : llvm::Function::Create(abort_sig,
                                                                                                       llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                                                                                       "abort", mod);

        std::string message =
                "\033[0;31mERROR:\033[0;0m Array subscript out of bounds\n\tfor expression '" + str() + "'\n\n";

        builder()->CreateCall(write_fn, {
                builder()->getInt32(2),
                builder()->CreateGlobalStringPtr(message),
                builder()->getInt64(message.size())
        });
        builder()->CreateCall(abort_fn);

        builder()->CreateBr(continue_block);
        builder()->SetInsertPoint(continue_block);
    }
};
