/**
 *
 * Advanced operators
 *
 */

#pragma once

#include "./allocating_expr.hh"
#include "./compilation_unit.hh"
#include "./container.hh"
#include "./include.hh"
#include "./primary.hh"
#include "./type.hh"
#include <llvm/IR/DerivedTypes.h>

class CallLike : public Expr {
  public:
    Vec<Expr> args;
    TypeVec type_args;
    Method* target = nullptr;

    LLVMValue create_call(VariableTable& vars, std::vector<llvm::Value*> injected_args, TypeVec second_type_args = {}) {

        std::vector<llvm::Value*> args;
        llvm::Value* ret_val_if_param = nullptr;

        if (target->must_parameterize_return_val()) {
            auto alloc_ty = target->return_type;
            auto alloc = create_allocation(target->return_type->llvm_type(), builder()->GetInsertBlock()->getParent());
            args.push_back(alloc);
            ret_val_if_param = alloc;
        }
        for (auto a : injected_args)
            args.push_back(a);
        auto mod = builder()->GetInsertBlock()->getModule();
        auto fn = mod->getFunction(target->source_name());

        if (target->is_generic()) {

            fn = make_fn(*target, mod, llvm::GlobalValue::LinkageTypes::ExternalLinkage, true);

            // Clone the type arguments so they persist past the lifetime of the
            // current substitution

            TypeVec cloned_fn_type_args;
            TypeVec cloned_tgt_type_args;

            for (auto t : type_args) {
                cloned_fn_type_args.push_back(t->clone());
            }

            for (auto t : second_type_args) {
                cloned_tgt_type_args.push_back(t->clone());
            }

            target->parent->parent->add_impl(target, cloned_fn_type_args, cloned_tgt_type_args);
        }
        if (!fn)
            except(E_BAD_CALL, "Failed to load function for call: " + target->source_name());

        for (size_t i = 0; i < this->args.len(); i++) {
            auto& arg = this->args[i];
            auto param = target->get_parameter(i);

            auto expected_type = param->type->llvm_type();
            auto arg_val = cast(arg.llvm_value(vars, expected_type), expected_type);
            args.push_back(arg_val);
        }

        auto call = builder()->CreateCall(fn, args);

        auto returns = target->return_type->clone();

        return LLVMValue{ret_val_if_param
                             ? (llvm::Value*)builder()->CreateLoad(ret_val_if_param->getType()->getPointerElementType(),
                                                                   (llvm::Value*)ret_val_if_param)
                             : call,
                         returns};
    }
};

class MethodCall : public CallLike {
  public:
    std::unique_ptr<ContainerMemberRef> name;

    std::string str() {
        std::string ret = name->str();

        if (type_args.size()) {
            ret += "<";
            bool first = true;
            for (auto ta : type_args) {
                if (!first)
                    ret += ", ";
                ret += ta->str();
                first = false;
            }
            ret += ">";
        }
        ret += "(";
        bool first = true;
        for (auto& a : args) {
            if (!first)
                ret += ", ";
            ret += a->str();
            first = false;
        }
        ret += ")";
        return ret;
    }

    LLVMValue llvm_value(VariableTable& vars, LLVMType expected_type = {}) {

        target->apply_generic_substitution(this->type_args, {});
        auto call = create_call(vars, {});

        Logger::debug("Created Call, casting to: " + (expected_type ? expected_type.qn_type->str() : "?"));
        auto result = cast(call, expected_type);
        target->undo_generic_substitution();
        return result;
    }

    std::vector<Statement*> flatten() {
        std::vector<Statement*> ret = {this};
        for (auto a : args)
            for (auto m : a->flatten())
                ret.push_back(m);
        return ret;
    }

    LLVMValue assign_ptr(VariableTable& vars) {
        // an assign_ptr may only be generated for calls which internally
        // pass a reference to the function, which is abstracted as a return
        // value
        if (!target->must_parameterize_return_val())
            except(E_BAD_OPERAND, "Indirection is only allowed on functions which return Arrays, Slices, or Structs");
        except(E_INTERNAL, "assign_ptr not implemented for MethodCall: " + str());
    }

    std::vector<Type*> flatten_types() {
        std::vector<Type*> ret;

        for (const auto& ta : type_args) {
            for (const auto& t : ta->flatten()) {
                ret.push_back(t);
            }
        }

        for (const auto& val : args) {
            for (const auto& t : val->flatten_types()) {
                ret.push_back(t);
            }
        }

        return ret;
    }

  protected:
    _Type get_type() {

        if (!target)
            return _Type(nullptr);

        target->apply_generic_substitution(this->type_args);
        auto ret_ty = target->return_type->clone();
        target->undo_generic_substitution();
        return ret_ty;
    }

    bool is_generic_type_args() {
        for (auto ta : type_args) {
            if (ta->get<Generic>())
                return true;
        }
        return false;
    }
};

class MethodCallOnType : public CallLike {
  public:
    std::unique_ptr<Name> method_name;
    std::unique_ptr<Expr> call_on;

    unsigned deref_count = 0;
    std::vector<Type*> flatten_types() {
        std::vector<Type*> ret;

        for (const auto& ta : type_args) {
            for (const auto& t : ta->flatten()) {
                ret.push_back(t);
            }
        }

        for (const auto& val : args) {
            for (const auto& t : val->flatten_types()) {
                ret.push_back(t);
            }
        }

        for (const auto& t : call_on->flatten_types())
            ret.push_back(t);

        return ret;
    }

    std::string str() {
        std::string ret = call_on->str() + "." + method_name->str();
        if (type_args.size()) {
            bool first = true;
            ret += "<";
            for (auto& ta : type_args) {
                if (!first)
                    ret += ", ";
                ret += ta->str();
                first = false;
            }
            ret += ">";
        }

        ret += "(";
        bool first = true;
        for (auto p : args) {
            if (!first)
                ret += ", ";
            ret += p->str();
            first = false;
        }
        ret += ")";
        return ret;
    }

    LLVMValue llvm_value(VariableTable& vars, LLVMType expected_type = {}) {

        Logger::debug("Calling method of type: " + this->call_on->type()->str());
        auto ptr = call_on->assign_ptr(vars);
        auto tmp_ref_count = deref_count;
        while (tmp_ref_count) {
            ptr = ptr.load();
            tmp_ref_count--;
        }

        TypeVec targ_type_args = {};
        if (auto ptref = ptr.type.qn_type->pointee()->get<ParameterizedTypeRef>()) {
            targ_type_args = ptref->params;
        }

        target->apply_generic_substitution(type_args, targ_type_args);

        auto call = create_call(vars, {ptr}, targ_type_args);
        auto result = cast(call, expected_type);
        target->undo_generic_substitution();
        return result;
    }

    std::vector<Statement*> flatten() {
        std::vector<Statement*> ret = {this};

        for (auto a : args)
            for (auto m : a->flatten())
                ret.push_back(m);
        for (auto f : call_on->flatten())
            ret.push_back(f);
        return ret;
    }

    LLVMValue assign_ptr(VariableTable& vars) { except(E_INTERNAL, "assign_ptr not implemented for MethodCallOnType"); }

  protected:
    _Type get_type() {

        if (!target)
            return _Type(nullptr);

        if (target == (Method*)1) {
            // Handle compiler implemented methods
            if (auto ref = call_on->type()->get<ReferenceType>()) {
#define X(n, ret)                                                                                                      \
    if (method_name->str() == #n)                                                                                      \
        return ret;
                X(len, Primitive::get(PR_int64))
                X(as_ptr, ref->of)
#undef X
            }
        }

        auto cot = call_on->type();
        if (auto ptref = cot->get<ParameterizedTypeRef>()) {
            ptref->apply_generic_substitution();
            target->apply_generic_substitution(this->type_args, ptref->params);

        } else {
            target->apply_generic_substitution(this->type_args);
        }
        auto ret_ty = target->return_type->clone();
        target->undo_generic_substitution();
        return ret_ty;
    }
};

class Return : public Statement {
  public:
    std::unique_ptr<Expr> value;

    std::string str() { return "return " + value->str(); }

    void generate(Method* qn_fn, llvm::Function* func, VariableTable& vars, ControlFlowInfo CFI) {
        if (value) {
            Logger::debug("fn returns: " + qn_fn->return_type->str());
            Logger::debug("Return: " + value->type()->str());
            auto return_value = value->llvm_value(vars, qn_fn->return_type);

            if (qn_fn->must_parameterize_return_val()) {
                auto write_to = func->getArg(0);
                builder()->CreateStore(return_value, write_to);
            } else {
                builder()->CreateRet(return_value);
            }
        } else {
            builder()->CreateRetVoid();
        }
    }

    std::vector<Statement*> flatten() {
        std::vector<Statement*> ret = {this};
        if (value)
            for (auto m : value->flatten())
                ret.push_back(m);
        return ret;
    }

    std::vector<Type*> flatten_types() { return value ? value->flatten_types() : std::vector<Type*>(); }

    ReturnChance returns() { return ReturnChance::DEFINITE; }
};

class InitializeVar : public Statement {
  public:
    _Type type;
    Name var_name;
    std::unique_ptr<Expr> initializer;
    bool is_constant = false;

    std::vector<Statement*> flatten() {
        std::vector<Statement*> ret = {this};
        if (initializer)
            for (auto m : initializer->flatten())
                ret.push_back(m);
        return ret;
    }

    std::vector<Type*> flatten_types() {
        auto ret = type->flatten();
        if (initializer)
            for (auto t : initializer->flatten_types())
                ret.push_back(t);
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

    void generate(Method* qn_fn, llvm::Function* func, VariableTable& vars, ControlFlowInfo CFI) {

        auto ll_type = type->llvm_type();
        auto name = var_name.str();

        auto alloc = create_allocation(ll_type, func);
        alloc->setName(name);
        scope->decl_new_variable(name, type);
        scope->get_var(name).value = alloc;

        // vars[name] = Variable(type, alloc, is_constant);
    }

    ReturnChance returns() { return ReturnChance::NEVER; }
};

class ExplicitCast : public Expr {
  public:
    std::unique_ptr<Expr> value;
    _Type cast_to;

    std::vector<Statement*> flatten() {
        std::vector<Statement*> ret = {this};
        for (auto m : value->flatten())
            ret.push_back(m);
        return ret;
    }

    std::vector<Type*> flatten_types() {
        auto ret = value->flatten_types();

        for (auto t : cast_to->flatten())
            ret.push_back(t);

        return ret;
    }

    LLVMValue llvm_value(VariableTable& vars, LLVMType expected_type = {}) {
        return cast(cast_explicit(value->llvm_value(vars), cast_to), expected_type);
    }

    std::string str() { return value->str() + " as " + cast_to->str(); }

    _Type get_type() { return cast_to; }

    LLVMValue assign_ptr(VariableTable& vars) {

        except(E_BAD_ASSIGNMENT, "Cannot assign a value to an explicitly casted value");
    }
};

// Similar to the ExplicitCast instruction
// but can only be used internally

class ImplicitCast : public Expr {
  public:
    std::unique_ptr<Expr> value;
    _Type cast_to;

    std::vector<Type*> flatten_types() {
        auto ret = value->flatten_types();

        for (auto t : cast_to->flatten())
            ret.push_back(t);

        return ret;
    }

    std::vector<Statement*> flatten() {
        std::vector<Statement*> ret = {this};
        for (auto m : value->flatten())
            ret.push_back(m);
        return ret;
    }
    ImplicitCast(std::unique_ptr<Expr> val, _Type cast_to) {
        this->value = std::move(val);
        this->cast_to = cast_to;
        this->scope = this->value->scope;
    }
    LLVMValue llvm_value(VariableTable& vars, LLVMType expected_type = {}) {
        auto val = value->llvm_value(vars, cast_to);
        return cast(val, expected_type);
    }

    std::string str() { return value->str() + " => " + cast_to->str(); }

    _Type get_type() { return cast_to; }

    LLVMValue assign_ptr(VariableTable& vars) {

        except(E_BAD_ASSIGNMENT, "Cannot assign a value to an implicitly casted value");
    }
};
class StructInitialization : public AllocatingExpr {
  public:
    std::map<std::string, std::unique_ptr<Expr>> initializers;
    _Type type;

    std::vector<Type*> flatten_types() {
        auto ret = type->flatten();

        for (auto& [_, v] : initializers) {
            for (auto t : v->flatten_types())
                ret.push_back(t);
        }

        return ret;
    }

    StructInitialization(_Type tgt) { this->type = tgt; }

    std::vector<Statement*> flatten() {
        std::vector<Statement*> ret = {this};
        for (auto& i : initializers)
            for (auto m : i.second->flatten())
                ret.push_back(m);
        return ret;
    }

    std::string str() {
        std::string ret = type->str() + " :: {";
        for (auto& i : initializers) {
            ret += "\n\t" + i.first + " : " + i.second->str() + ";";
        }
        ret += "\n}";
        return ret;
    }

    void write_direct(LLVMValue alloc, VariableTable& vars, LLVMType expected_type = {}) {
        auto member = this->type;

        // For PTref, resolve
        if (auto ptref = member->get<ParameterizedTypeRef>()) {
            ptref->apply_generic_substitution();
            member = ptref->resolves_to->clone();
            ptref->undo_generic_substitution();
        }

        auto struct_ty = member->get<StructType>();

        // Assert that it is a structtype
        if (!struct_ty)
            except(E_BAD_TYPE, "A struct initialization type may only be a StructType");

        // Assert all keys are present
        std::vector<std::string> missing_keys;
        for (auto [key, _] : struct_ty->members) {
            if (!initializers.contains(key)) {
                missing_keys.push_back(key);
            }
        }

        if (missing_keys.size()) {
            std::string message = "Failed to initialize a struct. The following keys were expected but not provided";
            for (auto key : missing_keys) {
                message += "\n\t>\t" + key + " : " + struct_ty->members[key]->str();
            }
            except(E_BAD_STRUCT_INITIALIZATION, message);
        }

        // TODO: Add erroring for unexpected keys

        // Write each key to the struct
        for (auto [key_name, key_type] : struct_ty->members) {
            auto& init_value = this->initializers[key_name];

            auto init_ll_value = init_value->llvm_value(vars, key_type);

            auto idx = struct_ty->member_idx(key_name);

            auto member_ptr = builder()->CreateStructGEP(struct_ty->llvm_type(), alloc, idx);

            builder()->CreateStore(init_ll_value, member_ptr);
        }

        // except(E_INTERNAL, "write_direct not implemented for structs: " +
        // member->str());
    }

    LLVMValue assign_ptr(VariableTable& vars) { except(E_BAD_ASSIGNMENT, "Struct Initializers are not assignable"); }

    _Type get_type() { return type; }
};

class Subscript : public Expr {
  public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> index;

    Subscript(std::unique_ptr<Expr> tgt, std::unique_ptr<Expr> idx) {
        this->target = std::move(tgt);
        this->index = std::move(idx);
    }

    std::vector<Type*> flatten_types() {
        auto ret = target->flatten_types();
        for (auto t : index->flatten_types())
            ret.push_back(t);

        return ret;
    }

    std::vector<Statement*> flatten() {
        std::vector<Statement*> ret = {this};
        for (auto m : index->flatten())
            ret.push_back(m);
        return ret;
    }

    std::string str() { return target->str() + "[" + index->str() + "]"; }

    LLVMValue llvm_value(VariableTable& vars, LLVMType expected_type = {}) {
        auto ptr = assign_ptr(vars);
        return cast(ptr.load(), expected_type);
    }

    LLVMValue assign_ptr(VariableTable& vars) {

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

            generate_bounds_check(idx,
                                  builder()->getInt64(ptr->getType()->getPointerElementType()->getArrayNumElements()));

            auto ep = builder()->CreateGEP(ptr->getType()->getPointerElementType(), ptr, {zero, idx});
            return {ep, ptr_to_element};

        } else if (tgt_ty->get<DynListType>()) {
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
        } else {
            ptr.print();
            except(E_BAD_OPERAND, "You may only access subscripts of an array type, operand was found to be of type: " +
                                      tgt_ty->str());
        }
    }

    _Type get_type() {
        if (!target)
            except(E_INTERNAL, "subscript bad target");
        if (!target->type())
            return target->type();
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

        auto write_sig = llvm::FunctionType::get(
            builder()->getInt32Ty(),
            {builder()->getInt32Ty(), builder()->getInt8Ty()->getPointerTo(), builder()->getInt64Ty()}, false);
        auto abort_sig = llvm::FunctionType::get(builder()->getVoidTy(), {}, false);

        auto write_fn =
            mod->getFunction("write")
                ? mod->getFunction("write")
                : llvm::Function::Create(write_sig, llvm::GlobalValue::LinkageTypes::ExternalLinkage, "write", mod);
        auto abort_fn =
            mod->getFunction("abort")
                ? mod->getFunction("abort")
                : llvm::Function::Create(abort_sig, llvm::GlobalValue::LinkageTypes::ExternalLinkage, "abort", mod);

        std::string message =
            "\033[0;31mERROR:\033[0;0m Array subscript out of bounds\n\tfor expression '" + str() + "'\n\n";

        builder()->CreateCall(write_fn, {builder()->getInt32(2), builder()->CreateGlobalStringPtr(message),
                                         builder()->getInt64(message.size())});
        builder()->CreateCall(abort_fn);

        builder()->CreateBr(continue_block);
        builder()->SetInsertPoint(continue_block);
    }
};
