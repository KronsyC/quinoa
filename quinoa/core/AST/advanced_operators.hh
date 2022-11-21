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



class CallLike : public Expr{
public:
    Vec<Expr> args;
    std::vector<std::shared_ptr<Type>> type_args;
    std::shared_ptr<Type> return_type;
    Method*   target = nullptr;
};
class MethodCall : public CallLike
{
public:
    std::unique_ptr<ContainerMemberRef> name;


    std::string str()
    {
        std::string ret = name->str();

        if (type_args.size())
        {
            ret+="<";
            bool first = true;
            for (auto ta : type_args)
            {
                if (!first)
                    ret += ", ";
                ret += ta->str();
                first = false;
            }
            ret+=">";
        }
        ret += "(";
        bool first = true;
        for (auto &a : args)
        {
            if (!first)
                ret += ", ";
            ret += a->str();
            first = false;
        }
        ret += ")";
        return ret;
    }
    llvm::Value *llvm_value(VariableTable& vars, llvm::Type* expected_type = nullptr)
    {
        auto mod = builder()->GetInsertBlock()->getModule();
        auto fn = mod->getFunction(target->source_name());

        if(!fn)except(E_BAD_CALL, "Failed to load function for call: " + target->source_name());

        std::vector<llvm::Value*> args;
        for(size_t i = 0; i < this->args.len(); i++){
            auto& arg   = this->args[i];
            auto param = target->get_parameter(i);

            auto expected_type = param->type->llvm_type();
            auto arg_val = arg.llvm_value(vars, expected_type);
            args.push_back(arg_val);
        }

        auto call = builder()->CreateCall(fn, args);
        return cast(call, expected_type);
    }
    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        for(auto a : args)for(auto m : a->flatten())ret.push_back(m);
        return ret;
    }
    llvm::Value* assign_ptr(VariableTable& vars){
        except(E_INTERNAL, "assign_ptr not implemented for MethodCall");
    }

protected:
    std::shared_ptr<Type> get_type()
    {
        if(!target)return std::shared_ptr<Type>(nullptr);
        return target->return_type;
    }
};

class MethodCallOnType : public CallLike{
public:
    std::unique_ptr<Name> method_name;
    std::unique_ptr<Expr> call_on;

    std::string str(){
        std::string ret = call_on->str() + "." + method_name->str();
        if (type_args.size())
        {
            bool first = true;
            ret+="<";
            for (auto &ta : type_args)
            {
                if (!first)
                    ret += ", ";
                ret += ta->str();
                first = false;
            }
            ret+=">";
        }

        ret += "(";
        bool first = true;
        for(auto p : args){
            if(!first)ret += ", ";
            ret += p->str();
            first = false;

        }
        ret += ")";
        return ret;
    }

    llvm::Value *llvm_value(VariableTable& vars, llvm::Type* expected_type = nullptr)
    {
        if(!target)except(E_BAD_CALL, "Cannot create a method call to an unresolved method (this is a bug)");

        auto mod = builder()->GetInsertBlock()->getModule();

        if(target == (Method*)1){
            except(E_INTERNAL, "unimplemented compiler method: " + method_name->str());
        }

        auto fn = mod->getFunction(target->source_name());

        if(!fn)except(E_BAD_CALL, "Failed to load function for call: " + target->name->str());

        std::vector<llvm::Value*> args;

        // Inject the target variable as the first argument
        args.push_back(call_on->assign_ptr(vars));


        for(size_t i = 0; i < this->args.len(); i++){
            auto& arg   = this->args[i];
            auto param = target->get_parameter(i);

            auto expected_type = param->type->llvm_type();
            auto arg_val = arg.llvm_value(vars, expected_type);
            args.push_back(arg_val);
        }

        auto call = builder()->CreateCall(fn, args);
        return cast(call, expected_type);
    }

    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};

        for(auto a : args)for(auto m : a->flatten())ret.push_back(m);
        for(auto f : call_on->flatten())ret.push_back(f);
        return ret;

    }
    llvm::Value* assign_ptr(VariableTable& vars){
        except(E_INTERNAL, "assign_ptr not implemented for MethodCallOnType");
    }
protected:
    std::shared_ptr<Type> get_type()
    {
        if(!target)return std::shared_ptr<Type>(nullptr);
        if(target == (Method*)1){
            if(auto ref = call_on->type()->get<ReferenceType>()){
                #define X(n, ret)if(method_name->str() == #n)return ret;
                X(len, Primitive::get(PR_int64))
                X(as_ptr, ref->of)
#undef X
            }

        }
        return target->return_type;

    }
};

class Return : public Statement
{
public:
    std::unique_ptr<Expr> value;

    std::string str()
    {
        return "return " + value->str();
    }
    void generate(llvm::Function* func, VariableTable& vars, ControlFlowInfo CFI)
    {
        auto expected_type = func->getReturnType();

        if(value){
            Logger::debug("Return: " + value->str());
            auto return_value  = value->llvm_value(vars, expected_type);
            builder()->CreateRet(return_value);
        }
        else{
            builder()->CreateRetVoid();
        }
    }
    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        if(value)for(auto m : value->flatten())ret.push_back(m);
        return ret;
    }
    ReturnChance returns(){
        return ReturnChance::DEFINITE;
    }
};

class InitializeVar : public Statement
{
public:
    std::shared_ptr<Type> type;
    Name var_name;
    std::unique_ptr<Expr> initializer;
    bool is_constant = false;

    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        if(initializer)for(auto m : initializer->flatten())ret.push_back(m);
        return ret;
    }

    std::string str(){
        std::string ret = is_constant ? "const " : "let ";
        ret += var_name;
        if(type){
            ret+= " : " + type->str();
        }
        if(initializer){
            ret += " = " + initializer->str();
        }
        return ret;
    }
    void generate(llvm::Function* func, VariableTable& vars, ControlFlowInfo CFI){

        auto ll_type = type->llvm_type();
        auto name    = var_name.str();
        auto alloca  = builder()->CreateAlloca(ll_type, nullptr, name);

        vars[name] = Variable(type.get(), alloca, is_constant);
    }

    ReturnChance returns(){
        return ReturnChance::NEVER;
    }
};

class ExplicitCast : public Expr{
public:
    std::unique_ptr<Expr> value;
    std::shared_ptr<Type> cast_to;

    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        for(auto m : value->flatten())ret.push_back(m);
        return ret;
    }
    llvm::Value* llvm_value(VariableTable& vars, llvm::Type* expected_type = nullptr){
        return cast(value->llvm_value(vars, cast_to->llvm_type()), expected_type);
    }

    std::string str(){
        return value->str() + " as " + cast_to->str();
    }
    std::shared_ptr<Type> get_type(){
        return cast_to;
    }
    llvm::Value* assign_ptr(VariableTable& vars){
        except(E_BAD_ASSIGNMENT, "Cannot assign a value to an explicitly casted value");
    }
};

class StructInitialization : public AllocatingExpr{
public:
    std::map<std::string, std::unique_ptr<Expr>> initializers;
    std::unique_ptr<ContainerMemberRef> target;
    std::shared_ptr<StructType> type;

    StructInitialization(std::unique_ptr<ContainerMemberRef> tgt){
        this->target = std::move(tgt);
    }

    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        for(auto& i : initializers)for(auto m : i.second->flatten())ret.push_back(m);
        return ret;
    }
    std::string str(){
        std::string ret = target->str();
        ret += "{\n";
        bool first = true;
        for(auto& init : initializers){
            if(!first)ret+=",\n";
            ret+="\t"+init.first + " : " + init.second->str();
            first = false;
        }
        ret += "\n}";
        return ret;
    }

    void write_direct(llvm::Value* alloc, VariableTable& vars, llvm::Type* expected_type = nullptr){
        if(!type)except(E_BAD_TYPE, "Cannot initialize struct with unresolved type: " + target->str());
        auto struct_ll_type = type->llvm_type();
        for(auto& init : initializers){
            auto idx = type->member_idx(init.first);

            if(idx == -1)except(E_BAD_ASSIGNMENT, "Bad Struct Key: " + init.first);

            auto target_ty = type->members[init.first]->llvm_type();
            auto init_expr = init.second->llvm_value(vars, target_ty);

            auto mem = builder()->CreateStructGEP(struct_ll_type, alloc, idx);
            builder()->CreateStore(init_expr, mem);
        }

        // ensure that all members are explicitly initialized
        for(auto [name, _] : type->members){
            Logger::debug("Member: " + name);
            auto& lookup = initializers[name];
            if(!lookup)except(E_BAD_ASSIGNMENT, "Initialization for struct '" + target->str() + "' is missing a member: '" + name + "'");
        }

    }

    llvm::Value* assign_ptr(VariableTable& vars){
        except(E_BAD_ASSIGNMENT, "Struct Initializers are not assignable");
    }
    std::shared_ptr<Type> get_type(){
        return type;
    }
};

class Subscript : public Expr{
public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> index;

    Subscript(std::unique_ptr<Expr> tgt, std::unique_ptr<Expr> idx){
        this->target = std::move(tgt);
        this->index  = std::move(idx);
    }
    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        for(auto m : index->flatten())ret.push_back(m);
        return ret;
    }
    std::string str(){
        return target->str() + "[" + index->str() + "]";
    }
    llvm::Value* llvm_value(VariableTable& vars, llvm::Type* expected_type = nullptr){
        auto ptr = assign_ptr(vars);
        auto load = builder()->CreateLoad(ptr->getType()->getPointerElementType(), ptr);
        return cast(load, expected_type);
    }
    llvm::Value* assign_ptr(VariableTable& vars){
        auto ptr = target->assign_ptr(vars);
        auto idx = index->llvm_value(vars);
        auto zero = Integer::get(0)->const_value(index->type()->llvm_type());

        if(ptr->getType()->getPointerElementType()->isArrayTy()){
             // Subscripts for arrays may only be integers, iX;
             if(!index->type()->llvm_type()->isIntegerTy())except(E_BAD_INDEX, "The Index of an array subscript expression must be an integer");


            generate_bounds_check(idx, builder()->getInt64(ptr->getType()->getPointerElementType()->getArrayNumElements()));


             auto ep = builder()->CreateGEP(ptr->getType()->getPointerElementType(), ptr, {zero, idx});
             return ep;
        }
        else if(auto slice_ty = target->type()->get<DynListType>()){
            Logger::debug("Subscript a slice");

            auto len_ptr = builder()->CreateStructGEP(ptr->getType()->getPointerElementType(), ptr, 0);
            auto len = builder()->CreateLoad(builder()->getInt64Ty(), len_ptr);

            generate_bounds_check(idx, len);

            auto arr_ptr_ptr = builder()->CreateStructGEP(ptr->getType()->getPointerElementType(), ptr, 1);
            auto arr_ptr = builder()->CreateLoad(arr_ptr_ptr->getType()->getPointerElementType(), arr_ptr_ptr);
            auto item_ptr = builder()->CreateGEP(arr_ptr->getType()->getPointerElementType(), arr_ptr, {zero, idx});
            return item_ptr;
        }
        else if(ptr->getType()->getPointerElementType()->isPointerTy()){
            auto val = builder()->CreateLoad(ptr->getType()->getPointerElementType(), ptr);
            auto ep = builder()->CreateGEP(val->getType()->getPointerElementType(), val, idx);
            return ep;
        }
        else except(E_BAD_OPERAND, "You may only access subscripts of an array type");
    }
    std::shared_ptr<Type> get_type(){
        return target->type()->pointee();
    }
private:
    void generate_bounds_check(llvm::Value* access_idx, llvm::Value* array_len_val){
        auto func = builder()->GetInsertBlock()->getParent();
        auto mod = func->getParent();
        auto exception_block = llvm::BasicBlock::Create(*llctx(), "bounds_check_err", func);
        auto continue_block = llvm::BasicBlock::Create(*llctx(), "bounds_check_cont", func);

        auto is_valid_access = builder()->CreateICmpSLT(access_idx, array_len_val);

        builder()->CreateCondBr(is_valid_access, continue_block, exception_block);

        builder()->SetInsertPoint(exception_block);

        // Requires both write() and abort() from libc, ensure they are present

        auto write_sig = llvm::FunctionType::get(builder()->getInt32Ty(), {builder()->getInt32Ty(), builder()->getInt64Ty(), builder()->getInt64Ty()}, false);
        auto abort_sig = llvm::FunctionType::get(builder()->getVoidTy(), {}, false);

        auto write_fn = mod->getFunction("write") ? mod->getFunction("write") : llvm::Function::Create(write_sig, llvm::GlobalValue::LinkageTypes::ExternalLinkage, "write", mod);
        auto abort_fn = mod->getFunction("abort") ? mod->getFunction("abort") : llvm::Function::Create(abort_sig, llvm::GlobalValue::LinkageTypes::ExternalLinkage, "abort", mod);

        std::string message = "\033[0;31mERROR:\033[0;0m Array subscript out of bounds\n\tfor expression '" + str() + "'\n\n";

        builder()->CreateCall(write_fn, {
           builder()->getInt32(2),
           builder()->CreatePtrToInt(
                   builder()->CreateGlobalStringPtr(message),
                   builder()->getInt64Ty()
           ),
           builder()->getInt64(message.size())
        });
        builder()->CreateCall(abort_fn);

        builder()->CreateBr(continue_block);
        builder()->SetInsertPoint(continue_block);
    }
};