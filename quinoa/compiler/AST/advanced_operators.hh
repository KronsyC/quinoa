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
    llvm::Value *llvm_value()
    {
        except(E_INTERNAL, "llvm_value not implemented for MethodCall");
    }

protected:
    std::unique_ptr<Type> get_type()
    {
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
    void generate()
    {
        except(E_INTERNAL, "generate not implemented for return node");
    }
};

class InitializeVar : public Statement
{
public:
    std::unique_ptr<Type> type;
    Name var_name;
    std::unique_ptr<Expr> initializer;
    bool is_constant = false;
    

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
    void generate(){
        except(E_INTERNAL, "generate() not implemented for initializers");
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

    std::string str(){
        return target->str() + "[" + index->str() + "]";
    }
    llvm::Value* llvm_value(){
        except(E_INTERNAL, "llvm_value not implemented for Subscript");
    }
    std::unique_ptr<Type> get_type(){
        except(E_INTERNAL, "get_type not implemented for Subscript");
    }
};