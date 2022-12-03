#pragma once
#include "./include.hh"
#include "../../GenMacro.h"
#include "./primary.hh"
#include "./type.hh"
enum IntrinsicType{
    INTRINSICS_ENUM_MEMBERS
};

#define FLAG_CONST_INIT_RIGHTS "const_initialization_rights"

static std::map<std::string, IntrinsicType> intrinsic_mappings = {INTRINSICS_ENUM_MAPPINGS};
static std::map<IntrinsicType, std::string> intrinsic_names = {INTRINSICS_ENUM_NAMES};

class _Intrinsic : public Expr{
public:
    std::vector<std::shared_ptr<Type>> type_args;
    Vec<Expr>                          args;

    // Help keep track of certain attributes, such as const assignment rights
    std::vector<std::string> flags;
};

template< IntrinsicType IT >
class Intrinsic : public _Intrinsic {
public:

    std::string name = intrinsic_names[IT];

    Intrinsic(Vec<Expr> args, std::vector<std::shared_ptr<Type>> type_args){
        this->args = std::move(args);
        this->type_args = type_args;
        this->validate();
    }
    static std::unique_ptr<_Intrinsic> create(IntrinsicType type, Vec<Expr> args, std::vector<std::shared_ptr<Type>> type_args){
        #define C(intr_ty) case intr_ty: return std::make_unique<Intrinsic<(intr_ty)>>(std::move(args), type_args);

        switch(type){
            C(intr_add)
            C(intr_add_one)
            C(intr_assign)
            C(intr_bitwise_and)
            C(intr_bitwise_not)
            C(intr_sub)
            C(intr_mul)
            C(intr_div)
            C(intr_mod)
            C(intr_bool_or)
            C(intr_bool_and)
            C(intr_get_member)
            C(intr_cmp_lesser)
            C(intr_cmp_greater)
            C(intr_cmp_lesser_eq)
            C(intr_cmp_greater_eq)
            C(intr_cmp_eq)
            C(intr_cmp_neq)
            C(intr_bitwise_or)
            C(intr_bitwise_xor)
            C(intr_bitwise_shl)
            C(intr_bitwise_shr)
            C(intr_bool_not)
            C(intr_pointer_to)
            C(intr_dereference)
            C(intr_negate)
            C(intr_sub_one)
            C(intr_size_of)
            C(intr_make_slice)
            C(intr_subscript)
        }
#undef C
    }

    std::shared_ptr<Type> get_type(){
        // Default behaviour (non-overridden)
        // Return parameter type for unary operations
        // Return left type for binary operations (most operations enforce left and right type equality)
        // Error for any other operations

        if(this->args.len() == 1)return args[0].type();
        else if(this->args.len() == 2)return args[0].type();

        except(E_INTERNAL, "no default implementation for get_type, for intrinsic " + name);
    }
    LLVMValue assign_ptr(VariableTable& vars){
        except(E_INTERNAL, "assign_ptr not implemented for intrinsics");
    }
    LLVMValue llvm_value(VariableTable& vars, LLVMType expected);

    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        for(auto a : args){
            for(auto m : a->flatten())ret.push_back(m);
        }

        return ret;
    }

    std::string str(){
        std::string ret = intrinsic_names[IT];

        if(!type_args.empty()){
            ret+="(<";
            bool first = true;
            for(auto& ta : type_args){
                if(!first)ret+=", ";
                ret+= ta->str();
                first = false;
            }
            ret+=">)";
        }

        ret+="(";
        bool first = true;
        for(auto a : args){
            if(!first)ret+=", ";
            ret+=a->str();
            first = false;
        }
        ret+=")";
        return ret;
    }

private:
    void validate(){
        // Search for the appropriate table entry, then run a comparison
        for(auto entry : defs){
            if(!entry->alias.empty() && entry->alias[0] == name){

                std::size_t arg_count = entry->intrinsic[0];
                std::size_t type_arg_count = entry->intrinsic[1];

                if(args.len() != arg_count){
                    except(
                            E_BAD_INTRINSIC_CALL,
                            "Bad argument count for intrinsic function: '" + name + "'" \
                            "\n\t\texpected: " + std::to_string(arg_count) + " args, but got: " + std::to_string(args.len())
                    );
                }
                if(type_args.size() != type_arg_count){
                    except(
                            E_BAD_INTRINSIC_CALL,
                            "Bad type generic count for intrinsic function: '" + name + "'" \
                            "\n\t\texpected: " + std::to_string(type_arg_count) + " generic args, but got: " + std::to_string(type_args.size())
                    );
                }
                return;

            }
        }
        except(E_INTERNAL, "failed to do a table lookup for intrinsic function: " + name);
    }

};


