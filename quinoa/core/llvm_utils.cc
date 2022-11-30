#include "./llvm_utils.h"
#include "../lib/error.h"
#include "../lib/logger.h"
#include "./AST/type.hh"
#include <optional>
#include <variant>
static llvm::LLVMContext _ctx;
static llvm::IRBuilder<> _builder(_ctx);

llvm::LLVMContext* llctx()
{
    return &_ctx;
}
llvm::IRBuilder<>* builder()
{
    return &_builder;
}

bool isInt(LLVMType t)
{
    return t->isIntegerTy();
}
LLVMValue Variable::as_value(){
    return {this->value, LLVMType(Ptr::get(this->type))};
}

std::variant<LLVMValue, std::string> try_cast(LLVMValue _val, const LLVMType& _to, bool is_explicit){


    if(!_to)return _val;

    auto to = _to.qn_type->drill()->llvm_type();
    LLVMValue val(_val.val, _val.type.qn_type->drill()->llvm_type());



    auto val_ty = val.type;

    if(val_ty == to){

        return val;
    }

    auto val_ref = val_ty.qn_type->get<ReferenceType>();

    if(isInt(val_ty) && isInt(to)){

        // - Only allow upcasting, ie new integer must have more bytes than other
        // - unsigned integers may freely cast to their signed equivalents
        // - signed integers may not coerce to unsigned counterparts

        if(val_ty.is_signed() && !to.is_signed()){
            // signed -> unsigned, disallowed unless explicit due to possible loss of information
            if(is_explicit){
                return LLVMValue{builder()->CreateIntCast(val, to, to.is_signed()), to};
            }
            return "Cannot implicitly cast a signed integer to an unsigned integer, try casting using the `as` operator";
        }
        else if(to.is_signed() == val_ty.is_signed()){
            // same-signage conversion, only allow raising when explicit
            if(to->getPrimitiveSizeInBits() < val_ty->getPrimitiveSizeInBits() && !is_explicit){
                return "Cannot implicitly downcast an integer, please use the explicit casting `as` operator\n\t\t in cast " + val_ty.qn_type->str() + " -> " + to.qn_type->str();
            }
            return LLVMValue{builder()->CreateIntCast(val, to, to.is_signed()), to};

        }
        else if(!val_ty.is_signed() && to.is_signed()){
            // unsigned -> signed, perfectly legal
            return LLVMValue{builder()->CreateIntCast(val, to, false), to};
        }
        else{
            except(E_INTERNAL, "(bug) unhandled integer cast case from " + val_ty.qn_type->str() + " to " + to.qn_type->str());
        }

    }
    // Reference -> Ptr
    if(val_ref && to.qn_type->get<Ptr>()){
        return LLVMValue{val, to};
    }

    // Array Reference -> Slice
    if(val_ref && to.qn_type->get<DynListType>()){

        if(auto list_ty = val_ref->of->get<ListType>()){

            auto my_element_type = list_ty->of->llvm_type();
            auto target_element_type = to.qn_type->get<DynListType>()->of->llvm_type();

            // ensure the two types are identical, otherwise error
            if(my_element_type != target_element_type)return "Cannot cast a list to a slice with a differing element type";

            auto list_len = Integer::get(list_ty->llvm_type()->getArrayNumElements())->const_value(LLVMType(Primitive::get(PR_uint64)));

            auto slice_alloc = builder()->CreateAlloca(to);

            auto len_ptr = builder()->CreateStructGEP(to, slice_alloc, 0);
            auto list_ptr = builder()->CreateStructGEP(to, slice_alloc, 1);

            auto cast_val = builder()->CreateBitCast(val, list_ptr->getType()->getPointerElementType());

            builder()->CreateStore(list_len, len_ptr);
            builder()->CreateStore(cast_val, list_ptr);

            return LLVMValue{builder()->CreateLoad(to, slice_alloc), to};
        }
    }

    if(val_ref){
        // if the target type is a reference, ensure their contents are coercible
        if(to.qn_type->get<ReferenceType>()){
            except(E_INTERNAL, "Reference -> Reference casting is currently unsupported");
        }
        // otherwise, attempt to cast the referenced data (implicit de-referencing)
        auto loaded = val.load();

        auto casted = try_cast(loaded, to, false);
        return casted;
    }

    // pointer -> integer, only allow when explicit
    if(isInt(to) && val_ty.qn_type->get<Ptr>()){
        if(!is_explicit)return "Casting from a pointer to an integer is an explicit operation";
        else{
            return LLVMValue{builder()->CreatePtrToInt(val, to), to};
        }
    }


    val.print();
    except(E_INTERNAL, "(bug) failed to cast from " + val_ty.qn_type->str() + " to " + to.qn_type->str());
}


LLVMValue cast_explicit(LLVMValue val, LLVMType type){

    auto cast_result = try_cast(val, type, true);

    try{
        auto result = std::get<LLVMValue>(cast_result);
        return result;
    }
    catch(const std::bad_variant_access& ex){
        except(E_BAD_CAST, std::get<std::string>(cast_result));
    }
}

LLVMValue cast(LLVMValue val, LLVMType type)
{

    auto cast_result = try_cast(val, type, false);

    try{
        auto result = std::get<LLVMValue>(cast_result);
        if(type && val.type != type){
            Logger::debug("-- $$$ Implicit Casting " + val.type.qn_type->str() + " -- to -- " + type.qn_type->str());
            val.print();
            result.print();
        }

        return result;
    }
    catch(const std::bad_variant_access& ex){
        val->print(llvm::outs());
        except(E_BAD_CAST, std::get<std::string>(cast_result));
    }
}
