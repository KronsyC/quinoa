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



class UnaryOperation: public Expr{
public:
    std::unique_ptr<Expr> operand;
    UnaryOpType op_type;

protected:
    std::unique_ptr<Type> get_type(){
        auto bool_t    = Primitive::get(PR_boolean);
        auto same_t    = std::make_unique<Type>(operand->type());
        auto same_ptr  = Ptr::get(std::make_unique<Type>(operand->type()));
        auto pointee_t = std::make_unique<Type>(same_t->pointee());
        switch(op_type){
            case PRE_amperand:    return same_ptr;
            case PRE_bang:        return bool_t;
            case PRE_star:        return pointee_t;
            case PRE_minus:       return same_t;
            case PRE_bitwise_not: return same_t;
            case PRE_increment:   return same_t;
            case PRE_decrement:   return same_t;
            case POST_increment:  return same_t;
            case POST_decrement:  return same_t;
            default: except(E_INTERNAL, "Failed to get return type of unary operation: " + std::to_string(op_type));
        }
    }
};

class BinaryOperation: public Expr{
public:
    std::unique_ptr<Expr> left_operand;
    std::unique_ptr<Expr> right_operand;
    BinaryOpType op_type;


protected:
    std::unique_ptr<Type> get_type(){
        switch(op_type){
            	case BIN_plus:
                case BIN_minus:
                case BIN_star:
                case BIN_slash:
                case BIN_percent:
                case BIN_bitiwse_or:
                case BIN_bitwise_and:
                case BIN_bitwise_shl:
                case BIN_bitwise_shr:
                case BIN_bitwise_xor:
                case BIN_bool_and:
                case BIN_bool_or:
                case BIN_greater:
                case BIN_greater_eq:
                case BIN_equals:
                case BIN_not_equals:
                case BIN_lesser:
                case BIN_lesser_eq:
                    return TypeUtils::get_common_type(left_operand->type(), right_operand->type());
                case BIN_assignment:
                    return std::make_unique<Type>(right_operand->type());
                default: except(E_INTERNAL, "Failed to get type for op: " + std::to_string(op_type));
        }
    }
};


