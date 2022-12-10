#include "./llvm_utils.h"
#include "../lib/error.h"
#include "../lib/logger.h"
#include "./AST/type.hh"
#include <optional>
#include <variant>
#include "./AST/container_member.hh"
static llvm::LLVMContext _ctx;
static llvm::IRBuilder<> _builder(_ctx);



llvm::AllocaInst* create_allocation(llvm::Type* type, llvm::Function* func){
  auto& entry_block = func->getEntryBlock();
  auto current_ins_pt = builder()->GetInsertBlock();

  builder()->SetInsertPoint(&entry_block, entry_block.getFirstInsertionPt());
  auto alloc = builder()->CreateAlloca(type);

  builder()->SetInsertPoint(current_ins_pt);

  return alloc;
}

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

llvm::Function* make_fn(
	Method &f,
	llvm::Module *mod,
	llvm::Function::LinkageTypes linkage, bool as_resolved_generic)
{
  if(f.is_generic() && !as_resolved_generic)return nullptr;

	llvm::Type* ret = f.return_type->llvm_type();
	auto name = f.source_name();

  Logger::debug("Make function signature: "  + name);
	std::vector<llvm::Type *> args;


    int skip_count = 0;
    if(f.must_parameterize_return_val()){
        // func foo() -> int[] or other similar situation
        // converts to
        // func foo( __internal_arg__ : int[]* ) -> void;
        skip_count++;
        ret = builder()->getVoidTy();
        args.push_back(f.return_type->llvm_type()->getPointerTo());
    }
    if(f.acts_upon){
        skip_count++;
        Logger::debug("acts upon: " + f.acts_upon->str());
        auto self_t = Ptr::get(f.get_target_type())->llvm_type();
        args.push_back(self_t);

    }
    for (auto a : f.parameters)
	{
		auto param_type = a->type->llvm_type();
		args.push_back(param_type);
	}

    bool isVarArg = false;


	auto sig = llvm::FunctionType::get(ret, args, isVarArg);
	auto fn = llvm::Function::Create(sig, linkage, name, mod);

    // Prettify the parameter names
#ifdef DEBUG
	for (unsigned int i = skip_count; i < fn->arg_size(); i++)
	{
		auto &param = f.parameters[i-skip_count];
		auto name = param.name.str();
		auto arg = fn->getArg(i);
		arg->setName(name);
	}
#endif
  return fn;
}


std::variant<LLVMValue, std::string> try_cast(LLVMValue _val, const LLVMType& _to, bool is_explicit){

    if(!bool(_to))return _val;

    Logger::debug("Cast: " + _val.type.qn_type->str() + " to " + _to.qn_type->str());
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

    // pointer -> pointer, only allow when explicit
    if(val_ty.qn_type->get<Ptr>() && to.qn_type->get<Ptr>()){
        if(!is_explicit)return "Casting from a pointer to another pointer is an explicit operation (" + val_ty.qn_type->str() +" -> " + to.qn_type->str() + " )";

        // simply bitcast
        return LLVMValue{builder()->CreateBitCast(val, to), to};
    }

    // int -> float, implicit is fine
    if(isInt(val_ty) && to->isFloatingPointTy()){
        Logger::debug("int to float");
        if(val_ty.is_signed()){
            return LLVMValue{builder()->CreateSIToFP(val, to), to};
        }
        else return LLVMValue{builder()->CreateUIToFP(val, to), to};
    }

    // float -> int, explicit only
    if(isInt(to) && val_ty->isFloatingPointTy()){
        if(!is_explicit)return "Casting from a float to an integer is an explicit operation";
        if(to.is_signed())return LLVMValue{builder()->CreateFPToSI(val, to), to};
        else return LLVMValue{builder()->CreateFPToUI(val, to), to};
    }


    // float -> float, implicit grow, explicit shrink
    if(to->isFloatingPointTy() && val_ty->isFloatingPointTy()){
      if(to->getPrimitiveSizeInBits() < val_ty->getPrimitiveSizeInBits() && !is_explicit){
        return "Cannot implicitly downcast a float, please use the explicit casting `as` operator\n\t\t in cast " + val_ty.qn_type->str() + " -> " + to.qn_type->str();
      }
      return LLVMValue{builder()->CreateFPCast(val, to), to};

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
        return result;
    }
    catch(const std::bad_variant_access& ex){
        except(E_BAD_CAST, std::get<std::string>(cast_result));
    }
}
