#pragma once
#include "../AST/compilation_unit.hh"
#include "../AST/advanced_operators.hh"
#include "../compiler.h"
#include "./passes/syntactic_sugar.hh"

#define ExportTable std::map<std::string, Container*>

/**
 *
 * Handle Imports and prevent duplication
 *
*/

std::string gen_rand_unique_str(size_t size){
    static std::vector<std::string> used_strings;
    static const char choices[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static int len = sizeof(choices);
    std::string ret;
    ret.reserve(size);
    for (unsigned int i = 0; i < size; ++i)
    {
        ret += choices[rand() % (len - 1)];
    }

    if(includes(used_strings, ret))return gen_rand_unique_str(size);
    return ret;
}

void add_prefixes(CompilationUnit& unit, std::string str_prefix){
    static std::map<std::string, std::shared_ptr<Name>> prefix_cache;
    auto& prefix = prefix_cache[str_prefix];
    if(!prefix){
        prefix = std::make_shared<Name>(str_prefix);
    }
    for(auto container : unit.get_containers()){
        container->name_space = prefix;
        for(auto member : container->members){
            member->name_space = prefix;
        }
    }
}

void resolve_aliased_symbols(CompilationUnit& unit, LongName& alias, LongName& replace_with, bool change_state = true){

    Logger::debug("Switch out "  + alias.str() + " for " + replace_with.str());
    for(auto fn : unit.get_methods()){
        if(!fn->content || fn->aliases_resolved)continue;

        for(auto code : fn->content->flatten()){
            if(auto call = dynamic_cast<MethodCall*>(code)){
                Logger::debug("-- " + call->name->container->name->str() + " / " + alias.str());
                if(call->name->container->name->str() == alias.str()){
                    call->name->container->name = std::make_unique<LongName>(replace_with);
                }
            }
        }
//        if(change_state)fn->aliases_resolved = true;
    }

}

void handle_imports(CompilationUnit& unit);

ExportTable generate_export_table(CompilationUnit& unit){
    ExportTable exports;
    for(auto mod:unit.get_containers()){
        if(mod->is_imported)continue;
        if(mod->has_compositor("Exported")){
            exports["__default__"] = mod;
        }
    }
    return exports;
}

static std::map<std::string, ExportTable> all_exports;

CompilationUnit* construct_ast_from_path(std::string path){
    Logger::debug("Construct ast: " + path);
    static std::map<std::string, std::unique_ptr<CompilationUnit>> import_cache;

    auto& cached = import_cache[path];

    if(!cached){
        Logger::debug("Cache miss");
        auto file_content = read_file(path);
        auto imported_ast = make_ast(file_content, path, false);
        cached = std::move(imported_ast);
        apply_syntactic_sugar(*cached);

        auto prefix_hash = gen_rand_unique_str(4);
        add_prefixes(*cached, prefix_hash);

        for(auto container : cached->get_containers()){
            // Fix self-references to refer to the new symbol
            auto alias = LongName();
            alias.parts.push(*container->name);
            auto new_name = container->full_name();
            Logger::debug("Fixing self-refs for container: " + alias.str() + " to refer to " + new_name.str());
            resolve_aliased_symbols(*cached, alias, new_name, false);
        }

        auto exports = generate_export_table(*cached);
        all_exports[path] = exports;

        handle_imports(*cached);

    }



    return cached.get();
}

void merge_units(CompilationUnit& tgt, CompilationUnit donor)
{
    auto transferred = donor.transfer();
    for(size_t i = 0; i < transferred.size(); i++){
        auto member = std::move(transferred[i]);
        member->is_imported = true;
        member->parent = &tgt;
        tgt.members.push(std::move(member));
    }
}


void handle_imports(CompilationUnit& unit){

    static const std::string libq_dir = std::string(QUINOA_DIR) + "/libq";


    int removals = 0;
    for (unsigned int i = 0; i < unit.members.len(); i++)
    {
        auto& item = unit.members[i - removals];
        if(auto import = dynamic_cast<Import*>(&item))
        {

            //
            // 1. Constructs an AST from the import                     <-
            // 2. Attaches a unique hash to each member                  | construct_ast_from_path
            // 3. Update the export table                                |
            // 4. Recursively resolves imports for the child AST ...... <-
            // 5. Resolve the import alias to the true (hashed) name for the parent
            //      This process also singles out the exported module
            // 6. Inlines the AST into the parent
            //

            if (import->is_stdlib)
            {
                string rpath = std::regex_replace(import->target.str(), std::regex("::"), "/");
                rpath = libq_dir + "/" + rpath + ".qn";

                auto ast = construct_ast_from_path(rpath);

                auto alias = import->alias;

                // Get the name of the target module (it is in the global export table)
                Container* target_module = all_exports[rpath]["__default__"];

                if(!target_module)except(E_BAD_IMPORT, "Failed to locate default export from " + rpath);

                auto full_name = target_module->full_name();

                resolve_aliased_symbols(unit, alias, full_name);

                merge_units(unit, std::move(*ast));

            }
            else except(E_INTERNAL, "Non-stdlib imports are an unsupported feature");
        }
    }

}



