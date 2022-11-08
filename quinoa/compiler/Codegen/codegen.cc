#include "./codegen.hh"
#include "../../lib/error.h"
#include "../../lib/list.h"
#include "../../lib/logger.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Linker/Linker.h"

#include "../AST/container_member.hh"
#include "../AST/compilation_unit.hh"
#include "../AST/container.hh"
#include "../AST/primary.hh"

#include <algorithm>
#include <map>
#include <string>

llvm::Function *make_fn(
	Method &f,
	llvm::Module *mod,
	llvm::Function::LinkageTypes linkage = llvm::Function::LinkageTypes::ExternalLinkage,
	bool mangle = true)
{
	auto ret = f.return_type->llvm_type();
	auto name = f.source_name();
	std::vector<llvm::Type *> args;
	for (auto a : f.parameters)
	{
		auto param_type = a->type->llvm_type();
		args.push_back(param_type);
	}
	bool isVarArg = false;
	// if(f.parameters.len()) {

	// 	if(f.parameters[f.parameters.len() - 1].is_variadic {
	// 		isVarArg = true;
	// 		args.pop_back();
	// 		if(!f.nomangle) {
	// 			auto t = Primitive::get(PR_int32);
	// 			args.push_back(t->getLLType());
	// 			auto arglen = Ident::get("+vararg_count");
	// 			f.params[f.params.size() - 1] = new Param(t, arglen);
	// 		}
	// 	}
	// }

	auto sig = llvm::FunctionType::get(ret, args, isVarArg);
	auto fn = llvm::Function::Create(sig, linkage, name, mod);
	Logger::debug("Created function " + name);
	for (unsigned int i = 0; i < fn->arg_size(); i++)
	{
		auto &param = f.parameters[i];
		auto name = param.name.str();
		auto arg = fn->getArg(i);
		arg->setName(name);
	}
	return fn;
}

// llvm::GlobalValue *make_global(Property *prop,
// 							   llvm::Module *mod,
// 							   llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage)
// {
// 	Logger::debug("Create global " + prop->str());
// 	auto type = prop->type->getLLType();
// 	auto initializer = prop->initializer;
// 	llvm::Constant *const_initializer = nullptr;
// 	if (initializer)
// 	{
// 		auto constant = initializer->constant();
// 		if (!constant)
// 			except(E_BAD_OPERAND, "Initialzer to " + prop->name->str() + " must be a constant value");
// 		const_initializer = constant->getLLConstValue(type);
// 	}
// 	auto global = new llvm::GlobalVariable(*mod, type, false, linkage, const_initializer, prop->str());
// 	return global;
// }

VariableTable generate_variable_table(llvm::Function *fn, CompilationUnit &ast, Method *method)
{
	if (fn == nullptr)
		except(E_INTERNAL, "Cannot varify the args of a null function");
	VariableTable vars;

	// // Inject the peer properties as non-prefixed variables
	// for (auto prop : method->memberOf->getAllProperties())
	// {
	// 	auto prop_name = prop->name->member;
	// 	auto init = (llvm::AllocaInst *)fn->getParent()->getGlobalVariable(prop->str());
	// 	vars[prop_name->str()] = new Variable(prop->type, init);
	// }
	// // Inject all properties as full variables
	// for (auto prop : ast.getAllProperties())
	// {
	// 	auto prop_name = prop->name;
	// 	auto init = (llvm::AllocaInst *)fn->getParent()->getGlobalVariable(prop->str());
	// 	vars[prop_name->str()] = new Variable(prop->type, init);
	// }

	// // Inject the args as variables
	for (unsigned int i = 0; i < fn->arg_size(); i++)
	{
		auto& param = method->parameters[i];
		auto arg = fn->getArg(i);
		auto alloc = builder()->CreateAlloca(arg->getType(), nullptr, "param " + arg->getName().str());

		builder()->CreateStore(arg, alloc);
		vars[arg->getName().str()] = Variable(param.type.get(), alloc);
	}

	// Inject the var-args as a known-length list
	// if (method->sig->isVariadic())
	// {
	// 	auto varParam = method->sig->params[method->sig->params.size() - 1];
	// 	auto varParamType = (ListType *)varParam->type;
	// 	auto elementType = varParamType->elements->getLLType();
	// 	auto one = builder()->getInt32(1);
	// 	auto lenptr = vars["+vararg_count"];
	// 	varParamType->size = Ident::get("+vararg_count");
	// 	auto len =
	// 		builder()->CreateLoad(lenptr->value->getType()->getPointerElementType(), lenptr->value, "varargs_len");

	// 	auto list = builder()->CreateAlloca(elementType, len, "varargs_list");
	// 	auto init = builder()->CreateAlloca(elementType->getPointerTo());
	// 	builder()->CreateStore(list, init);
	// 	vars[varParam->name->str()] = new Variable(varParamType->elements, init);
	// 	pushf(*method, (Statement *)list);

	// 	auto i32 = Primitive::get(PR_int32)->getLLType();
	// 	auto i8p = (new TPtr(Primitive::get(PR_int8)))->getLLType();
	// 	auto st = llvm::StructType::create(*llctx(), {i32, i32, i8p, i8p});
	// 	auto alloc = builder()->CreateAlloca(st, nullptr, "var_args_obj");
	// 	// Tracker / i
	// 	auto tracker = builder()->CreateAlloca(i32, nullptr, "i");
	// 	builder()->CreateStore(builder()->getInt32(0), tracker);
	// 	auto cast = builder()->CreateBitCast(alloc, i8p);
	// 	builder()->CreateUnaryIntrinsic(llvm::Intrinsic::vastart, cast);

	// 	// Create a loop to iter over each arg, and push it into the list
	// 	auto eval = llvm::BasicBlock::Create(*llctx(), "while_eval", fn);
	// 	auto body = llvm::BasicBlock::Create(*llctx(), "while_body", fn);
	// 	auto cont = llvm::BasicBlock::Create(*llctx(), "while_cont", fn);

	// 	builder()->CreateBr(eval);
	// 	builder()->SetInsertPoint(eval);

	// 	// Generate the Evaluator
	// 	auto loaded_i = builder()->CreateLoad(tracker->getType()->getPointerElementType(), tracker);
	// 	auto isSmaller = builder()->CreateICmpSLT(loaded_i, len);
	// 	builder()->CreateCondBr(isSmaller, body, cont);
	// 	builder()->SetInsertPoint(body);

	// 	// get the value and set it in the list
	// 	auto value = builder()->CreateVAArg(alloc, elementType);
	// 	auto listPtr = builder()->CreateGEP(list->getType()->getPointerElementType(), list, loaded_i);
	// 	builder()->CreateStore(value, listPtr);

	// 	// Increment and jump out
	// 	auto inc = builder()->CreateAdd(loaded_i, one);
	// 	builder()->CreateStore(inc, tracker);
	// 	builder()->CreateBr(eval);
	// 	builder()->SetInsertPoint(cont);
	// }
	
	return vars;
}

std::unique_ptr<llvm::Module> generate_module(Container &mod, CompilationUnit &ast)
{
	if(mod.type != CT_MODULE)except(E_INTERNAL, "cannot generate non-module container");
	auto llmod = std::make_unique<llvm::Module>(mod.name->str(), *llctx());

	// Write hoisted definitions
	for(auto hoist : ast.get_hoists()){
		if(auto method = dynamic_cast<Method*>(hoist)){
			make_fn(*method, llmod.get());
		}
		else except(E_INTERNAL, "Attempt to hoist unrecognized node");
	}

	for (auto child : mod.members)
	{
		if (auto method = dynamic_cast<Method *>(child))
		{
			auto fname = method->source_name();
			auto fn = llmod->getFunction(fname);
			if (fn == nullptr)
			{
				except(E_MISSING_FUNCTION, "Function " + fname + " could not be found");
			}
			if (!method->content->content.len())
				continue;
			auto entry_block = llvm::BasicBlock::Create(*llctx(), "entry_block", fn);

			builder()->SetInsertPoint(entry_block);
			auto vars = generate_variable_table(fn, ast, method);
			method->content->generate(fn, vars, {});
			if (fn->getReturnType()->isVoidTy())
				builder()->CreateRetVoid();
			continue;
		}
		else
			except(E_INTERNAL, "Failed to generate module member");
	}
	return llmod;
}

llvm::Module *Codegen::codegen(CompilationUnit &ast)
{
	auto rootmod = new llvm::Module("Quinoa Program", *llctx());
	std::vector<TopLevelEntity*> definitions;



	// Generate all of the modules, and link them into the root module
	for(auto container : ast.get_containers()){
		if(auto mod = dynamic_cast<Container*>(container)){
			if(mod->type != CT_MODULE)continue;
			Logger::debug("Generate Module: " + mod->full_name().str());
			auto generated_mod = generate_module(*mod, ast);
			llvm::Linker::linkModules(*rootmod, std::move(generated_mod));
		}
	}
	return rootmod;
}