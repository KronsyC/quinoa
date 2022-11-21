#include "./codegen.hh"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Linker/Linker.h"


void make_fn(
	Method &f,
	llvm::Module *mod,
	llvm::Function::LinkageTypes linkage = llvm::Function::LinkageTypes::ExternalLinkage,
	bool with_generic = true)
{

    if(f.generic_params.size() && with_generic){
        // Generate each required implementation of the function
        for(auto impl : f.generate_usages){
            Logger::debug("Found an impl for: " + f.name->str());
            if(impl->size() != f.generic_params.size())except(E_INTERNAL, "(bug) Number of implemented args does not match the number of expected args");
            for(unsigned int i = 0; i<f.generic_params.size(); i++){
                f.generic_params[i]->temporarily_resolves_to = impl->at(i);
            }
            // Generate
            make_fn(f, mod, llvm::Function::LinkageTypes::ExternalLinkage, false);
        }
        return;
    }

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

void generate_method(Method* fn, CompilationUnit& ast, llvm::Module* ll_mod, bool with_generic = true){
    Logger::debug("Generate Function: " + fn->source_name());
    if(fn->generic_params.size() && fn->generate_usages.len() == 0)return;
    if(fn->generate_usages.len() && with_generic){
        for(auto impl : fn->generate_usages){
            for(unsigned int i = 0; i < impl->size(); i++){
                fn->generic_params[i]->temporarily_resolves_to = impl->at(i);
            }
            generate_method(fn, ast, ll_mod, false);
        }
        return;
    }
    auto fname = fn->source_name();
    auto ll_fn = ll_mod->getFunction(fname);
    if (ll_fn == nullptr)
    {
        except(E_MISSING_FUNCTION, "Function " + fname + " could not be found");
    }
    auto entry_block = llvm::BasicBlock::Create(*llctx(), "entry_block", ll_fn);

    builder()->SetInsertPoint(entry_block);
    auto vars = generate_variable_table(ll_fn, ast, fn);
    for(auto pair : vars){
        Logger::debug(pair.first + " -> " + pair.second.type->str());
    }
    Logger::debug("generate content");
    fn->content->generate(fn, ll_fn, vars, {});
    if (ll_fn->getReturnType()->isVoidTy())
        builder()->CreateRetVoid();
    Logger::debug("Generated");
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
        if(!method->content || !method->content->content.len())continue;
        generate_method(method, ast, llmod.get());
	}

	return llmod;
}

llvm::Module *Codegen::codegen(CompilationUnit &ast)
{
	auto rootmod = new llvm::Module("Quinoa Program", *llctx());
	std::vector<TopLevelEntity*> definitions;



	// Generate all the modules, and link them into the root module
	for(auto container : ast.get_containers()){
        if(container->type != CT_MODULE)continue;
        Logger::debug("Generate Module: " + container->full_name().str());
        auto generated_mod = generate_module(*container, ast);
        llvm::Linker::linkModules(*rootmod, std::move(generated_mod));
	}
	return rootmod;
}