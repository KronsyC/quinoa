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
#include "./intrinsic.hh"
#include "./advanced_operators.hh"
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
        auto op_t = operand->type();
        if(!op_t)except(E_INTERNAL, "operand has no type: " + operand->str());
        auto none = op_t->llvm_type();

        switch (op_type) {
            case PRE_ampersand: {
                auto ptr = operand->assign_ptr(vars);

                auto ptr_ty = ptr.type.qn_type->get<Ptr>();
                if(!ptr_ty)except(E_INTERNAL, "assign_ptr returned a non-pointer type");

                // Change the ptr to a reference, this is safe as they are
                // internally the exact same thing
                ptr.type = LLVMType(ReferenceType::get(ptr_ty->of));

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
private:
    static IntrinsicType intrinsic_from_bot(BinaryOpType bot){
#define C(x, y)case x: return intr_##y;
        switch (bot) {

            C(BIN_percent, mod)
            C(BIN_star, mul)
            C(BIN_plus, add)
            C(BIN_minus, sub)
            C(BIN_bool_or, bool_or)
            C(BIN_bool_and, bool_and)
            C(BIN_dot, get_member)
            C(BIN_slash, div)
            C(BIN_lesser, cmp_lesser)
            C(BIN_greater, cmp_greater)
            C(BIN_lesser_eq, cmp_lesser_eq)
            C(BIN_greater_eq, cmp_greater_eq)
            C(BIN_assignment, assign)
            C(BIN_equals, cmp_eq)
            C(BIN_not_equals, cmp_neq)
            C(BIN_bitwise_and, bitwise_and)
            C(BIN_bitwise_or, bitwise_or)
            C(BIN_bitwise_xor, bitwise_xor)
            C(BIN_bitwise_shl, bitwise_shl)
            C(BIN_bitwise_shr, bitwise_shr)
        }
    }
public:
    std::unique_ptr<_Intrinsic> internal_intrinsic;
    BinaryOpType op_type;


    BinaryOperation(std::unique_ptr <Expr> left, std::unique_ptr <Expr> right, BinaryOpType op_type) {
        Vec<Expr> args;
        args.push(std::move(left));
        args.push(std::move(right));

        auto int_ty = intrinsic_from_bot(op_type);
        this->internal_intrinsic = Intrinsic<intr_mul>::create(int_ty, std::move(args), {});
        this->op_type = op_type;
    }

    void set_initializing(){
        this->internal_intrinsic->flags.push_back(FLAG_CONST_INIT_RIGHTS);
    }

    std::string str() {
        return this->internal_intrinsic->str();
    }

    LLVMValue assign_ptr(VariableTable &vars) {
        except(E_BAD_ASSIGNMENT, "Binary Operations are not assignable");
    }
    bool has_normalized = false;
    LLVMValue llvm_value(VariableTable &vars, LLVMType expected_type = {}) {

        // if it has already normalized
        // assume it has been cast
        // and unwrap the casts, before rewrapping


        Logger::debug("normalizing binop: " + this->str());
        auto rel = internal_intrinsic->args.release();
        auto left = std::unique_ptr<Expr>(rel[0]);
        auto right = std::unique_ptr<Expr>(rel[1]);

        left->scope = this->scope;
        right->scope = this->scope;

        // special casting rules for assignment
        if(this->op_type == BIN_assignment){
            auto var_ty = left->type();
            auto val_ty = right->type();
            // implicit cast the val_ty to the var_ty
            if(has_normalized){
                static_cast<ImplicitCast*>(right.get())->cast_to = var_ty;
            }
            else{
                right =  std::make_unique<ImplicitCast>(std::move(right), var_ty);

            }
            right->scope = this->scope;

        }
        else{
            auto casted_ty = get_operand_cast_type(left->type(), right->type());

            if(has_normalized){
                static_cast<ImplicitCast*>(left.get())->cast_to = casted_ty.qn_type;
                static_cast<ImplicitCast*>(right.get())->cast_to = casted_ty.qn_type;
            }
            else{

            }

            // Implicitly cast both operands, then write back to the intrinsic

            left = std::make_unique<ImplicitCast>(std::move(left), casted_ty.qn_type);
            right = std::make_unique<ImplicitCast>(std::move(right), casted_ty.qn_type);

        }

        Vec<Expr> new_args;
        new_args.push(std::unique_ptr<Expr>(left.release()));
        new_args.push(std::unique_ptr<Expr>(right.release()));

        internal_intrinsic->args = std::move(new_args);
        has_normalized = true;


        return internal_intrinsic->llvm_value(vars, expected_type);
    }

    std::vector<Statement *> flatten() {
        std::vector < Statement * > ret = {this};
        for(auto n : internal_intrinsic->flatten())ret.push_back(n);
        return ret;
    }


private:
    LLVMType get_operand_cast_type(std::shared_ptr<Type> left, std::shared_ptr<Type> right){
#define common return get_common_type(left->llvm_type(), right->llvm_type());
#define boolean return Primitive::get(PR_boolean)->llvm_type();
#define right_t  return right->llvm_type();
        switch (op_type) {

            case BIN_percent:common
            case BIN_star:common
            case BIN_plus:common
            case BIN_minus:common
            case BIN_bool_or:boolean
            case BIN_bool_and:boolean
            case BIN_dot:except(E_INTERNAL, "unreachable");
            case BIN_slash:common
            case BIN_lesser:common
            case BIN_greater:common
            case BIN_lesser_eq:common
            case BIN_greater_eq:common
            case BIN_assignment:right_t
            case BIN_equals:common
            case BIN_not_equals:common
            case BIN_bitwise_and:common
            case BIN_bitwise_or:common
            case BIN_bitwise_xor:common
            case BIN_bitwise_shl:common
            case BIN_bitwise_shr:common
        }
#undef common
#undef boolean
#undef right_t
    }
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
protected:
    std::shared_ptr <Type> get_type() {
        return this->internal_intrinsic->type();
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
