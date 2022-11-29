/**
 * Symbol (Simple) operators, i.e builtin Binary and Unary operations
 */

#pragma once

#include "./primary.hh"
#include "./type.hh"
#include "../../GenMacro.h"
#include "./reference.hh"
#include "./literal.hh"
#include "./allocating_expr.hh"

enum UnaryOpType {
    UNARY_ENUM_MEMBERS
};

enum BinaryOpType {
    INFIX_ENUM_MEMBERS
};

static std::map <TokenType, UnaryOpType> prefix_op_mappings{PREFIX_ENUM_MAPPINGS};
static std::map <TokenType, UnaryOpType> postfix_op_mappings{POSTFIX_ENUM_MAPPINGS};
static std::map <TokenType, BinaryOpType> binary_op_mappings{INFIX_ENUM_MAPPINGS};


class UnaryOperation : public Expr {
public:
    std::unique_ptr <Expr> operand;
    UnaryOpType op_type;

    std::string str() {
        return "Some Unary op";
    }

    UnaryOperation(std::unique_ptr <Expr> operand, UnaryOpType kind) {
        this->operand = std::move(operand);
        this->op_type = kind;
    }

    std::vector<Statement *> flatten() {
        std::vector < Statement * > ret = {this};
        for (auto m: operand->flatten())
            ret.push_back(m);
        return ret;
    }

    LLVMValue assign_ptr(VariableTable &vars) {
        switch (op_type) {
            case PRE_star:return operand->llvm_value(vars);
            default:
                except(
                        E_BAD_ASSIGNMENT,
                       "Cannot get an assignable reference to unary operation of type " + std::to_string(op_type)
               );
        }
    };

    LLVMValue llvm_value( VariableTable &vars, LLVMType expected ) {
        auto _bool = Primitive::get(PR_boolean)->llvm_type();

        #define bld (*builder())
        #define val(expected) operand->llvm_value(vars, expected)

        #define ret(LL_INST, cast_to) return cast({bld.Create##LL_INST(val(cast_to)), type()}, expected);
        auto none = operand->type()->llvm_type();

        switch (op_type) {
            case PRE_ampersand: {
                auto ptr = operand->assign_ptr(vars);

                auto ptr_ty = ptr.type.qn_type->get<Ptr>();
                if(!ptr_ty)except(E_INTERNAL, "assign_ptr returned a non-pointer type");

                // Change the ptr to a reference, this is safe as they are
                // internally the exact same thing
                ptr.type = LLVMType(ReferenceType::get(ptr_ty->of));

                Logger::debug("Changed ref from: " + ptr_ty->str() + " to " + ptr.type.qn_type->str());
                return cast(ptr, expected);
            }
            case PRE_bang: ret(Not, _bool);
            case PRE_minus:{
                auto op_type = operand->type()->llvm_type();
                if(op_type.is_signed()){
                    ret(Neg, none)
                }
                else{
                    except(E_BAD_OPERAND, "Cannot invert the sign of an unsigned primitive\n\t\tIn operation: " + this->str());
                }

            }
            case PRE_bitwise_not:ret(Not, none);
            case PRE_star: {
                if (!operand->type()->get<Ptr>())except(E_BAD_OPERAND, "Cannot dereference non-ptr operand");
                auto value = operand->llvm_value(vars);
                return value.load();
            }
            case PRE_increment:
            case PRE_decrement:
            case POST_increment:
            case POST_decrement: {
                auto ptr = operand->assign_ptr(vars);

                auto val = ptr.load();
                auto one = cast(Integer::get(1)->llvm_value(vars, {}), val.type);

                auto updated_val = LLVMValue(op_type == POST_increment || op_type == PRE_increment ? bld.CreateAdd(val, one)
                                                                                      : bld.CreateSub(val, one), val.type);

                LLVMValue return_val =
                        op_type == POST_increment || op_type == POST_decrement ? operand->llvm_value(vars, expected)
                                                                               : updated_val;

                bld.CreateStore(updated_val, ptr);
                return return_val;
            }

            default:except(E_INTERNAL, "Failed to generate llvalue for unary operation: " + std::to_string(op_type));
        }

    }

protected:
    std::shared_ptr <Type> get_type() {
        auto same_t = operand->type();
        if (!same_t)return same_t;
        auto same_ref = ReferenceType::get(operand->type());
        auto pointee_t = same_t->pointee();
        auto bool_t = Primitive::get(PR_boolean);
        switch (op_type) {
            case PRE_ampersand:return same_ref;
            case PRE_bang:return bool_t;
            case PRE_star:return pointee_t;
            case PRE_minus:return same_t;
            case PRE_bitwise_not:return same_t;
            case PRE_increment:return same_t;
            case PRE_decrement:return same_t;
            case POST_increment:return same_t;
            case POST_decrement:return same_t;
            default:except(E_INTERNAL, "Failed to get return type of unary operation: " + std::to_string(op_type));
        }
    }
};

class BinaryOperation : public Expr {
public:
    std::unique_ptr <Expr> left_operand;
    std::unique_ptr <Expr> right_operand;
    BinaryOpType op_type;

    // The name of the variable that this expression
    // is responsible for initializing (if any)
    bool initializes;

    BinaryOperation(std::unique_ptr <Expr> left, std::unique_ptr <Expr> right, BinaryOpType op_type) {
        left_operand = std::move(left);
        right_operand = std::move(right);
        this->op_type = op_type;
    }

    std::string str() {
        auto symbol = this->get_symbol_as_str();
        return "( " + left_operand->str() + " " + symbol + " " + right_operand->str() + " )";
    }

    LLVMValue assign_ptr(VariableTable &vars) {
        except(E_BAD_ASSIGNMENT, "Binary Operations are not assignable");
    }

    LLVMValue llvm_value(VariableTable &vars, LLVMType expected_type = {}) {
        
        if (op_type == BIN_assignment) {
            auto assignee = left_operand->assign_ptr(vars);
            // constant assignment check
            if (auto var = dynamic_cast<SourceVariable *>(left_operand.get())) {
                auto &va = vars[var->name->str()];
                if (this->initializes && !va.is_initialized) {
                    va.is_initialized = true;
                } else {
                    if (va.is_initialized && va.constant)
                        except(E_BAD_ASSIGNMENT, "Cannot reassign a constant variable: " + var->name->str());
                }
            }



            auto assignment_type = LLVMType(assignee.type.qn_type->pointee());

            // Optimization for constructs like arrays and structs
            // removes a copy instruction
            if (auto allocating_expr = dynamic_cast<AllocatingExpr *>(right_operand.get())) {
                allocating_expr->write_direct(assignee, vars, assignment_type);
                return allocating_expr->llvm_value(vars, assignment_type);
            }

            auto value = right_operand->llvm_value(vars, assignment_type);
            builder()->CreateStore(value, assignee);
            return value;

        }


        auto left_val = left_operand->llvm_value(vars);
        auto right_val = right_operand->llvm_value(vars);
        auto op = get_op(left_val, right_val);
        return cast(op, expected_type);
    }

    std::vector<Statement *> flatten() {
        std::vector < Statement * > ret = {this};
        for (auto m: left_operand->flatten())
            ret.push_back(m);
        for (auto m: right_operand->flatten())
            ret.push_back(m);
        return ret;
    }

private:
    std::string get_symbol_as_str() {
#define r(x, y)case BIN_##x: return #y;

        switch (this->op_type) {
            r(percent, %)
            r(star, *)
            r(plus, +)
            r(minus, -)
            r(bool_or, ||)
            r(bool_and, &&)
            r(dot, .)
            r(slash, /)
            r(lesser, <)
            r(greater, >)
            r(lesser_eq, <=)
            r(greater_eq, >=)
            r(assignment, =)
            r(equals, ==)
            r(not_equals, !=)
            r(bitwise_and, &)
            r(bitwise_or, |)
            r(bitwise_xor, ^)
            r(bitwise_shl, <<)
            r(bitwise_shr, >>)
#undef r
        }
    }

    LLVMValue get_op(LLVMValue l, LLVMValue r) {
        auto mutual_type = get_common_type(l.type, r.type);
        auto left = cast(l, mutual_type);
        auto right = cast(r, mutual_type);
        auto bool_ty = Primitive::get(PR_boolean)->llvm_type();
        #define X(myop, opname, resultant_type)                            \
            case BIN_##myop:                                               \
                return {builder()->Create##opname(left, right), resultant_type};

        switch (op_type) {
            X(plus,         Add,         mutual_type)
            X(minus,        Sub,         mutual_type)
            X(star,         Mul,         mutual_type)
            X(slash,        SDiv,        mutual_type)
            X(percent,      SRem,        mutual_type)
            X(lesser,       ICmpSLT,     bool_ty)
            X(greater,      ICmpSGT,     bool_ty)
            X(lesser_eq,    ICmpSLE,     bool_ty)
            X(greater_eq,   ICmpSGE,     bool_ty)
            X(not_equals,   ICmpNE,      bool_ty)
            X(equals,       ICmpEQ,      bool_ty)
            X(bitwise_or,   Or,          mutual_type)
            X(bitwise_and,  And,         mutual_type)
            X(bitwise_shl,  Shl,         mutual_type)
            X(bitwise_shr,  AShr,        mutual_type)
            X(bitwise_xor,  Xor,         mutual_type)
            X(bool_and,     LogicalAnd, mutual_type)
            X(bool_or,      LogicalOr,  mutual_type)
            case BIN_dot: except(E_INTERNAL, "Unreachable code path detected");
            case BIN_assignment: except(E_INTERNAL, "Unreachable code path detected");
        }
        #undef X
    }

protected:
    std::shared_ptr <Type> get_type() {
        auto boo = Primitive::get(PR_boolean);
        switch (op_type) {
            case BIN_bool_and:
            case BIN_bool_or:
            case BIN_greater:
            case BIN_greater_eq:
            case BIN_equals:
            case BIN_not_equals:
            case BIN_lesser:
            case BIN_lesser_eq:return boo;

            case BIN_plus:
            case BIN_minus:
            case BIN_star:
            case BIN_slash:
            case BIN_percent:
            case BIN_bitwise_or:
            case BIN_bitwise_and:
            case BIN_bitwise_shl:
            case BIN_bitwise_shr:
            case BIN_bitwise_xor:return left_operand->type();
            case BIN_assignment:return right_operand->type();

            default:except(E_INTERNAL, "Failed to get type for op: " + std::to_string(op_type));
        }
    }
};

class MemberAccess : public Expr {
public:
    std::unique_ptr <Expr> member_of;
    std::unique_ptr <Name> member_name;

    MemberAccess(std::unique_ptr <Expr> member_of, std::unique_ptr <Name> member_name) {
        this->member_of = std::move(member_of);
        this->member_name = std::move(member_name);
    }

    std::string str() {
        return member_of->str() + "." + member_name->str();
    }

    LLVMValue assign_ptr(VariableTable &vars) {
        auto strct = member_of->assign_ptr(vars);
        auto retrv_t = member_of->type();

        while(auto ref_t = retrv_t->get<ReferenceType>()){
            // Implicitly de-reference the op for member access
            retrv_t = ref_t->of;
            strct = strct.load();
        }
        auto strct_t = retrv_t->get<StructType>();
        if (!strct_t)except(E_BAD_MEMBER_ACCESS, "Cannot access members of non-struct");

        auto idx = strct_t->member_idx(member_name->str());

        auto ptr = builder()->CreateStructGEP(strct_t->llvm_type(), strct, idx);
        return {ptr, LLVMType(Ptr::get(type()))};
    }

    LLVMValue llvm_value(VariableTable &vars, LLVMType expected_type = {}) {

        // Special case for slices (len and ptr properties)
        if (this->member_of->type()->get<DynListType>()) {
            auto slice_val = member_of->assign_ptr(vars);
            if (member_name->str() == "len") {
                auto len_ptr = builder()->CreateStructGEP(slice_val->getType()->getPointerElementType(), slice_val, 0);
                auto len = builder()->CreateLoad(len_ptr->getType()->getPointerElementType(), len_ptr);
                return cast({len, type()}, expected_type);
            } else if (member_name->str() == "ptr") {
                auto ref_ptr = builder()->CreateStructGEP(slice_val->getType()->getPointerElementType(), slice_val, 1);
                auto ref = builder()->CreateLoad(ref_ptr->getType()->getPointerElementType(), ref_ptr);
                return cast({ref, type()}, expected_type);
            } else except(E_BAD_MEMBER_ACCESS, "slices only have the 'ptr' and 'len' properties");
        }
        auto ptr = this->assign_ptr(vars);
        return cast(ptr.load(), expected_type);
    }

    std::vector<Statement *> flatten() {
        std::vector < Statement * > ret = {this};
        for (auto m: member_of->flatten())
            ret.push_back(m);
        return ret;
    }

protected:
    std::shared_ptr <Type> get_type() {
        auto left_t = member_of->type();
        if (!left_t)return std::shared_ptr<Type>(nullptr);
        auto member = member_name->str();

        while(auto ref_t = left_t->get<ReferenceType>()){
            // Implicitly de-reference the op for member access
            left_t = ref_t->of;
        }

        if (auto strct = left_t->get<StructType>()) {
            if (strct->member_idx(member) == -1)
                except(E_BAD_MEMBER_ACCESS, "The struct: " + member_of->str() + " does not have a member: " + member);
            return strct->members[member];
        } else if (auto slice = left_t->get<DynListType>()) {
            if (member == "len") {
                return Primitive::get(PR_uint64);
            } else if (member == "ptr") {
                return Ptr::get(slice->of);
            } else except(E_BAD_MEMBER_ACCESS, "slices only have the properties 'len' and 'ptr'");
        }

        except(E_BAD_MEMBER_ACCESS, "You may only access members of a container type (structs, slices, arrays), but the type was found to be: " + left_t->str());


    }
};
