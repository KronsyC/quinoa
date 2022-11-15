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





class MethodCall : public Expr
{
public:
    std::unique_ptr<ContainerMemberRef> name;
    Vec<Expr> args;
    Vec<Type> type_args;
    Method*   target;

    std::string str()
    {
        std::string ret = name->str();

        if (type_args.len())
        {
            bool first = true;
            for (auto &ta : type_args)
            {
                if (!first)
                    ret += ", ";
                ret += ta->str();
                first = false;
            }
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

        if(!fn)except(E_BAD_CALL, "Failed to load function for call: " + target->name->str());

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
        auto alloca  = builder()->CreateAlloca(ll_type);
        auto name    = var_name.str();
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

class StructInitialization : public Expr{
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
    llvm::Value* llvm_value(VariableTable& vars, llvm::Type* expected_type = nullptr){
        if(!type)except(E_BAD_TYPE, "Cannot initialize struct with unresolved type: " + target->str());
        auto struct_ll_type = type->llvm_type();
        auto alloc = builder()->CreateAlloca(struct_ll_type);
        for(auto& init : initializers){
            auto idx = type->member_idx(init.first);

            if(idx == -1)except(E_BAD_ASSIGNMENT, "Bad Struct Key");

            auto target_ty = type->members[init.first]->llvm_type();
            auto init_expr = init.second->llvm_value(vars, target_ty);

            auto mem = builder()->CreateStructGEP(struct_ll_type, alloc, idx);
            builder()->CreateStore(init_expr, mem);
        }
        return builder()->CreateLoad(struct_ll_type, alloc);
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
    std::unique_ptr<Expr>           index;

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

        if(ptr->getType()->getPointerElementType()->isArrayTy()){
             auto zero = Integer::get(0)->const_value(index->type()->llvm_type());
             auto ep = builder()->CreateGEP(ptr->getType()->getPointerElementType(), ptr, {zero, idx});
             return ep;
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
};