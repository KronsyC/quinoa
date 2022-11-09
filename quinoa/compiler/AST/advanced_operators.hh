/**
 *
 * Advanced operators
 *
 */

#pragma once
#include "./primary.hh"
#include "./type.hh"
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
        return target->return_type;
        except(E_INTERNAL, "get_type not implemented for MethodCall");
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
        for(auto m : initializer->flatten())ret.push_back(m);
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
};

class Subscript : public Expr{
public:
    std::unique_ptr<SourceVariable> target;
    std::unique_ptr<Expr>           index;

    Subscript(std::unique_ptr<SourceVariable> tgt, std::unique_ptr<Expr> idx){
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
        except(E_INTERNAL, "llvm_value not implemented for Subscript");
    }
    llvm::Value* assign_ptr(VariableTable& vars){
        except(E_INTERNAL, "assign_ptr not implemented for Subscript");
    }
    std::shared_ptr<Type> get_type(){
        except(E_INTERNAL, "get_type not implemented for Subscript");
    }
};