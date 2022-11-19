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

    if(f.acts_upon){
        auto self_t = Ptr::get(f.acts_upon)->llvm_type();
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
	Logger::debug("Created function " + name);
	for (unsigned int i = 0; i < fn->arg_size() - (bool)f.acts_upon; i++)
	{
		auto &param = f.parameters[i];
		auto name = param.name.str();
		auto arg = fn->getArg(i);
		arg->setName(name);
	}
	return fn;
}

llvm::GlobalValue *make_global(
	Property *prop,						   
	llvm::Module *mod,
	llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage
	)
{
	Logger::debug("Create global " + prop->name->str());
	auto type = prop->type->llvm_type();
	llvm::Constant *const_initializer = nullptr;
	if (prop->initializer) const_initializer = prop->initializer->const_value(type);
	else const_initializer = prop->type->default_value(type);
	
	auto global = new llvm::GlobalVariable(*mod, type, false, linkage, const_initializer, prop->name->str());
	return global;
}

VariableTable generate_variable_table(llvm::Function *fn, CompilationUnit &ast, Method *method)
{
	if (fn == nullptr)
		except(E_INTERNAL, "Cannot varify the args of a null function");
	VariableTable vars;

	// Inject the peer properties as non-prefixed variables
	for (auto prop : method->parent->get_properties())
	{
		auto prop_name = prop->name->member->str();
		auto init = (llvm::AllocaInst *)fn->getParent()->getGlobalVariable(prop->name->str());
		vars[prop_name] = Variable(prop->type.get(), init);
	}



	// Inject all properties as full variables
	for (auto prop : ast.get_properties())
	{
		auto prop_name = prop->name->str();
		auto init = (llvm::AllocaInst *)fn->getParent()->getGlobalVariable(prop->name->str());
		vars[prop_name] = Variable(prop->type.get(), init);
	}

    // Inject All Enum Members as variables
    for(auto type : ast.get_types()){
        if(auto _enum = dynamic_cast<EnumType*>(type->refers_to.get())){
            for(auto member : _enum->get_members()){
                auto alloc = builder()->CreateAlloca(member.second->getType());
                builder()->CreateStore(member.second, alloc);
                auto full_name = type->name->str() + "::" + member.first;
                vars[full_name] = Variable(Primitive::get(PR_int32).get(), alloc, true);

                if(type->parent == method->parent){
                    auto local_name = type->name->member->str() + "::" + member.first;
                    vars[local_name] = Variable(Primitive::get(PR_int32).get(), alloc, true);
                }
            }
        }
    }

    if(auto ty = method->acts_upon){
        auto arg = fn->getArg(0);
        auto alloc = builder()->CreateAlloca(arg->getType(), nullptr, "self");

        builder()->CreateStore(arg, alloc);
        vars["self"] = Variable(Ptr::get(ty).get(), alloc);
    }

	// Inject the args as variables
    int diff = (bool)method->acts_upon;
	for (unsigned int i = 0; i < method->parameters.len(); i++)
	{
        Logger::debug("Get param: " + std::to_string(i));
		auto& param = method->parameters[i];
		auto arg = fn->getArg(i+diff);
		auto alloc = builder()->CreateAlloca(arg->getType(), nullptr, "param " + arg->getName().str());

		builder()->CreateStore(arg, alloc);
		vars[param.name.str()] = Variable(param.type.get(), alloc);
	}

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
		else if(auto prop = dynamic_cast<Property*>(hoist)){
			make_global(prop, llmod.get());
		}
		else except(E_INTERNAL, "Attempt to hoist unrecognized node");
	}
	for(auto method : mod.get_methods()){
			if(!method->content)continue;
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