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

std::variant<LLVMValue, std::string> try_cast(LLVMValue val, LLVMType to, bool is_explicit){

    if(!to)return val;


    Logger::debug("Casting " + val.type.qn_type->str() + " -- to -- " + to.qn_type->str() + ", explicit? " + (is_explicit?"yes":"no"));
    Logger::debug("Value Signage: " + std::to_string(val.type.is_signed()) + "; target signage: " + std::to_string(to.is_signed()));

    auto val_ty = val.type;

    if(val_ty == to)return val;

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
                return "Cannot implicitly downcast an integer, please use the explicit casting `as` operator";
            }
            return LLVMValue{builder()->CreateIntCast(val, to, to.is_signed()), to};

        }
        else if(!val_ty.is_signed() && to.is_signed()){
            // unsigned -> signed, perfectly legal
            return LLVMValue{builder()->CreateIntCast(val, to, true), to};
        }
        else{
            except(E_INTERNAL, "(bug) unhandled integer cast case from " + val_ty.qn_type->str() + " to " + to.qn_type->str());
        }

    }

    if(auto ref = val_ty.qn_type->get<ReferenceType>()){
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
        return result;
    }
    catch(const std::bad_variant_access& ex){
        val->print(llvm::outs());
        except(E_BAD_CAST, std::get<std::string>(cast_result));
    }
}
