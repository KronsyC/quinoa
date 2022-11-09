/**
 * Symbol (Simple) operators, i.e builtin Binary and Unary operations
*/

#pragma once
#include "./primary.hh"
#include "./type.hh"
#include "../../GenMacro.h"
#include "./type_utils.h"


enum UnaryOpType{
    UNARY_ENUM_MEMBERS
};

enum BinaryOpType{
    INFIX_ENUM_MEMBERS
};

static std::map<TokenType, UnaryOpType> prefix_op_mappings { PREFIX_ENUM_MAPPINGS };
static std::map<TokenType, UnaryOpType> postfix_op_mappings { POSTFIX_ENUM_MAPPINGS };
static std::map<TokenType, BinaryOpType> binary_op_mappings { INFIX_ENUM_MAPPINGS };

class UnaryOperation: public Expr{
public:
    std::unique_ptr<Expr> operand;
    UnaryOpType op_type;
    std::string str(){
        return "Some Unary op";
    }

    UnaryOperation(std::unique_ptr<Expr> operand, UnaryOpType kind){
        this->operand = std::move(operand);
        this->op_type = kind;
    }
    llvm::Value* llvm_value(VariableTable& vars, llvm::Type* expected_type = nullptr){
        except(E_INTERNAL, "llvm_value not implemented for UnaryOperation");
    }
    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        for(auto m : operand->flatten())ret.push_back(m);
        return ret;
    }
    llvm::Value* assign_ptr(VariableTable& vars){
        switch(op_type){
            case PRE_star:
                return operand->llvm_value(vars);
            default:
                except(E_BAD_ASSIGNMENT, "Cannot get an assignable reference to unary operation of type " + std::to_string(op_type));
        }
        except(E_INTERNAL, "assign_ptr not implemented for Subscript");
    }
protected:
    std::shared_ptr<Type> get_type(){
        except(E_INTERNAL, "get_type not implemented");
        // auto bool_t    = Primitive::get(PR_boolean);
        // auto same_t    = std::make_unique<Type>(operand->type());
        // auto same_ptr  = Ptr::get(std::make_unique<Type>(operand->type()));
        // auto pointee_t = std::make_unique<Type>(same_t->pointee());
        // switch(op_type){
        //     case PRE_amperand:    return same_ptr;
        //     case PRE_bang:        return bool_t;
        //     case PRE_star:        return pointee_t;
        //     case PRE_minus:       return same_t;
        //     case PRE_bitwise_not: return same_t;
        //     case PRE_increment:   return same_t;
        //     case PRE_decrement:   return same_t;
        //     case POST_increment:  return same_t;
        //     case POST_decrement:  return same_t;
        //     default: except(E_INTERNAL, "Failed to get return type of unary operation: " + std::to_string(op_type));
        // }
    }
};

class BinaryOperation: public Expr{
public:
    std::unique_ptr<Expr> left_operand;
    std::unique_ptr<Expr> right_operand;
    BinaryOpType op_type;

    // The name of the variable that this expression
    // is responsible for initializing (if any)
    bool initializes;

    BinaryOperation(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, BinaryOpType op_type){
        left_operand = std::move(left);
        right_operand = std::move(right);
        this->op_type = op_type;
    }
    std::string str(){
        return "Some Binary op";
    }
    llvm::Value* assign_ptr(VariableTable& vars){
        except(E_BAD_ASSIGNMENT, "Binary Operations are not assignable");
    }
    llvm::Value* llvm_value(VariableTable& vars, llvm::Type* expected_type = nullptr){
        
        if(op_type == BIN_assignment){
            auto assignee = left_operand->assign_ptr(vars);
            auto value    = right_operand->llvm_value(vars, assignee->getType()->getPointerElementType());



            // constant assignment check
            if(auto var = dynamic_cast<SourceVariable*>(left_operand.get())){
                auto& va = vars[var->name->str()];
                if(this->initializes && !va.is_initialized){
                    va.is_initialized = true;
                }
                else{
                    if(va.is_initialized && va.constant)except(E_BAD_ASSIGNMENT, "Cannot reassign a constant variable: " + var->name->str());
                }

            }

            builder()->CreateStore(value, assignee);


            return cast(value, expected_type);
        }



        auto left_t  = left_operand->type();
        auto right_t = right_operand->type();

        auto common_t = TypeUtils::get_common_type(left_t, right_t);

        auto left_val = left_operand->llvm_value(vars, common_t->llvm_type());
        auto right_val = right_operand->llvm_value(vars, common_t->llvm_type());

        auto op = get_op(left_val, right_val);
        return cast(op, expected_type);
    }

    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        for(auto m : left_operand->flatten())ret.push_back(m);
        for(auto m : right_operand->flatten())ret.push_back(m);
        return ret;
    }
private:
    llvm::Value* get_op(llvm::Value* l, llvm::Value* r){
        #define X(myop, opname)case BIN_##myop: return builder()->Create##opname(l, r);
        switch(op_type){
            X(plus, Add)
            X(minus, Sub)
            X(star, Mul)
            X(slash, SDiv)
            X(percent, SRem)
            X(lesser, ICmpSLT)
            X(greater, ICmpSGT)
            X(lesser_eq, ICmpSLE)
            X(greater_eq, ICmpSGE)
            X(not_equals, ICmpNE)
            X(equals, ICmpEQ)
            X(bitwise_or, Or)
            X(bitwise_and, And)
            X(bitwise_shl, Shl)
            X(bitwise_shr, AShr)
            X(bitwise_xor, Xor)
            X(bool_and, LogicalAnd)
            X(bool_or, LogicalOr)
            default: except(E_INTERNAL, "Failed to generate binary operation");
        }
    }
protected:
    std::shared_ptr<Type> get_type(){
        except(E_INTERNAL, "cannot get_type for BinaryOperation");
        // switch(op_type){
        //     	case BIN_plus:
        //         case BIN_minus:
        //         case BIN_star:
        //         case BIN_slash:
        //         case BIN_percent:
        //         case BIN_bitiwse_or:
        //         case BIN_bitwise_and:
        //         case BIN_bitwise_shl:
        //         case BIN_bitwise_shr:
        //         case BIN_bitwise_xor:
        //         case BIN_bool_and:
        //         case BIN_bool_or:
        //         case BIN_greater:
        //         case BIN_greater_eq:
        //         case BIN_equals:
        //         case BIN_not_equals:
        //         case BIN_lesser:
        //         case BIN_lesser_eq:
        //             return TypeUtils::get_common_type(left_operand->type(), right_operand->type());
        //         case BIN_assignment:
        //             return std::make_unique<Type>(right_operand->type());
        //         default: except(E_INTERNAL, "Failed to get type for op: " + std::to_string(op_type));
        // }
    }
};


