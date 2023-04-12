#include "./codegen.hh"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Linker/Linker.h"
#include <string>

void inject_variable_definitions(Method& fn, llvm::Function* ll_fn) {

    if (auto ty = fn.get_target_type()) {
        auto arg = ll_fn->getArg(fn.must_parameterize_return_val() ? 1 : 0);
        auto alloc = builder()->CreateAlloca(arg->getType(), nullptr, "self");
        builder()->CreateStore(arg, alloc);
        fn.scope->decl_new_variable("self", ReferenceType::get(ty), false);
        fn.scope->get_var("self")->value = alloc;
    }

    int diff = fn.acts_upon ? 1 : 0;
    if (fn.must_parameterize_return_val())
        diff++;

    for (unsigned int i = 0; i < fn.parameters.len(); i++) {
        auto& param = fn.parameters[i];
        auto arg = ll_fn->getArg(i + diff);
        auto alloc = builder()->CreateAlloca(arg->getType(), nullptr, "!" + arg->getName().str());

        builder()->CreateStore(arg, alloc);

        fn.scope->decl_new_variable(param.name.str(), param.type, true);
        fn.scope->get_var(param.name.str())->value = alloc;
    }
}

void inject_module_definitions(Container& cont) {
    // Create external defintions for all global values
    for (auto m : cont.parent->get_containers()) {
        // Dont predeclare self
        if (m == &cont)
            continue;

        // Dont predeclare non-depended modules
        bool is_dep = false;
        for (auto& [_, a] : cont.aliases) {
            if (m->full_name().str() == a.str()) {
                is_dep = true;
                break;
            }
        }
        if (!is_dep)
            continue;

        for (auto dec : m->member_decls) {
            if (!dec.is_public)
                continue;
            auto name = m->full_name().str() + "::" + dec.name;

            auto global = new llvm::GlobalVariable(cont.get_mod(), dec.type->llvm_type(), dec.is_constant,
                                                   llvm::GlobalValue::LinkOnceODRLinkage, nullptr, name);
            cont.scope->decl_new_variable(name, dec.type);
            cont.scope->get_var(name)->value = (llvm::AllocaInst*)global;
        }
    }

    // Create local defintions
    for (auto def : cont.member_decls) {
        auto name = def.name;
        auto global_name = cont.full_name().str() + "::" + name;

        auto glob = new llvm::GlobalVariable(cont.get_mod(), def.type->llvm_type(), def.is_constant,
                                             def.is_public ? llvm::GlobalValue::LinkageTypes::ExternalLinkage
                                                           : llvm::GlobalValue::LinkageTypes::PrivateLinkage,
                                             def.initializer, global_name);

        cont.scope->decl_new_variable(name, def.type);
        cont.scope->decl_new_variable(global_name, def.type);
        cont.scope->get_var(name)->value = (llvm::AllocaInst*)glob;
        cont.scope->get_var(global_name)->value = (llvm::AllocaInst*)glob;
    }
}

void generate_method(Method* fn, CompilationUnit& ast, llvm::Module* ll_mod, bool allow_generic = false) {
    if (fn->is_generic() && !allow_generic)
        return;

    set_active_container(*fn->parent);
    auto fname = fn->source_name();
    auto ll_fn = ll_mod->getFunction(fname);
    if (ll_fn == nullptr) {
        ll_mod->print(llvm::outs(), nullptr);
        except(E_MISSING_FUNCTION, "Function " + fname + " could not be found");
    }
    if (ll_fn->getBasicBlockList().size()) {
        return;
    }
    auto entry_block = llvm::BasicBlock::Create(*llctx(), "entry_block", ll_fn);

    builder()->SetInsertPoint(entry_block);
    inject_variable_definitions(*fn, ll_fn);
    std::map<std::string, Variable> tmp;
    fn->content->generate(fn, ll_fn, tmp, {});

    auto retc = fn->content->returns();
    if (ll_fn->getReturnType()->isVoidTy() && (retc == ReturnChance::NEVER || retc == ReturnChance::MAYBE))
        builder()->CreateRetVoid();
}
void generate_module(Container& mod, CompilationUnit& ast) {
    set_active_container(mod);
    inject_module_definitions(mod);
    if (mod.type != CT_MODULE)
        except(E_INTERNAL, "cannot generate non-module container");
    // Write hoisted definitions
    for (auto hoist : ast.get_hoists()) {
        if (auto method = dynamic_cast<Method*>(hoist)) {
            make_fn(*method, &mod.get_mod());
        }
    }

    // Pass one: concrete functions
    for (auto method : mod.get_methods()) {
        if (method->generic_params.size())
            continue;
        if (!method->content || !method->content->content.len())
            continue;
        Logger::debug("Generating concrete function: " + method->signature());
        generate_method(method, ast, &mod.get_mod());
    }
}
llvm::Module* Codegen::codegen(CompilationUnit& ast) {
    auto rootmod = new llvm::Module("Quinoa Program", *llctx());
    std::vector<TopLevelEntity*> definitions;

    // Generate all the modules, and link them into the root module

    for (auto container : ast.get_containers()) {
        if (container->type != CT_MODULE)
            continue;
        Logger::debug("Generating module: " + container->full_name().str());
        generate_module(*container, ast);
    }

    // Generic function generation loop
    // we need to loop until all implementations are handled
    // generics may generate more generics
    while (auto impl = ast.get_next_impl()) {

        impl->target->apply_generic_substitution(impl->substituted_method_type_args,
                                                 impl->substituted_target_type_args);

        Logger::debug("Generating function: " + impl->target->signature());
        make_fn(*impl->target, &impl->target->parent->get_mod(), llvm::GlobalValue::ExternalLinkage, true);
        generate_method(impl->target, ast, &impl->target->parent->get_mod(), true);

        impl->target->undo_generic_substitution();
        impl->has_impl = true;
    }

    for (auto c : ast.get_containers()) {
        if (auto mod = c->take_mod()) {
            llvm::Linker::linkModules(*rootmod, std::move(mod));
        }
    }
    llvm::verifyModule(*rootmod);
    return rootmod;
}
