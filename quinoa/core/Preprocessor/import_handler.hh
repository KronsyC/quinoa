#pragma once

#include "../AST/advanced_operators.hh"
#include "../AST/compilation_unit.hh"
#include "../compiler.h"
#include "./passes/syntactic_sugar.hh"
#include <cstring>
#include <llvm/ADT/StringRef.h>

#define ExportTable std::map<std::string, Container*>

/**
 *
 * Handle Imports and prevent duplication
 *
 */

std::string gen_rand_unique_str(size_t size) {
    static std::vector<std::string> used_strings;
    static const char choices[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static int len = sizeof(choices);
    std::string ret;
    ret.reserve(size);
    for (unsigned int i = 0; i < size; ++i) {
        ret += choices[rand() % (len - 1)];
    }

    if (includes(used_strings, ret))
        return gen_rand_unique_str(size);
    return ret;
}

std::string to_encodedstr(std::uint64_t num) {
    // Convert the number to a string
    static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const unsigned chars_len = sizeof chars / sizeof chars[0];
    std::uint8_t sections[8];
    std::memcpy(&sections, &num, sizeof num);
    std::string ret;
    for (auto m : sections) {
        auto idx = m % (chars_len - 1);
        ret.push_back(chars[idx]);
    }
    return ret.substr(0, 5);
}

std::string hash_path(std::string path, int char_count = 4) {
    auto hash = llvm::hash_value(path);
    auto str_hash = to_encodedstr(hash);
    return str_hash;
}

void add_prefixes(CompilationUnit& unit, std::string str_prefix) {
    static std::map<std::string, std::shared_ptr<Name>> prefix_cache;
    auto& prefix = prefix_cache[str_prefix];
    if (!prefix) {
        prefix = std::make_shared<Name>(str_prefix);
    }

    for (auto container : unit.get_containers()) {
        if (container->name_space)
            continue;
        container->name_space = prefix;
    }
}

void resolve_aliased_symbols(CompilationUnit& unit, LongName& alias, LongName& replace_with) {

    for (const auto& cont : unit.get_containers()) {
        if (cont->is_imported)
            continue;

        cont->aliases[alias.str()] = LongName(replace_with);
    }
}

ExportTable generate_export_table(CompilationUnit& unit) {
    ExportTable exports;
    for (auto mod : unit.get_containers()) {
        if (mod->is_imported)
            continue;
        if (mod->has_compositor("Exported")) {
            exports["__default__"] = mod;
        }
    }
    return exports;
}

void handle_imports(CompilationUnit& unit, std::map<std::string, std::string> includes);
static std::map<std::string, ExportTable> all_exports;

CompilationUnit* construct_ast_from_path(std::string path, std::map<std::string, std::string> includes) {
    static std::map<std::string, std::unique_ptr<CompilationUnit>> import_cache;

    auto& cached = import_cache[path];

    if (!cached) {
        auto file_content = read_file(path);
        auto imported_ast = make_ast(file_content, path, false);
        cached = std::move(imported_ast);
        apply_syntactic_sugar(*cached);

        std::string prefix_hash = hash_path(path);
        add_prefixes(*cached, prefix_hash);

        auto exports = generate_export_table(*cached);
        all_exports[path] = exports;

        handle_imports(*cached, includes);
    }

    return cached.get();
}

void merge_units(CompilationUnit& tgt, CompilationUnit donor) {
    auto transferred = donor.transfer();
    for (size_t i = 0; i < transferred.size(); i++) {
        auto member = std::move(transferred[i]);
        member->is_imported = true;
        member->parent = &tgt;
        if (member->scope)
            member->scope->set_parent(tgt.scope.get());
        tgt.members.push(std::move(member));
    }
}

void handle_imports(CompilationUnit& unit, std::map<std::string, std::string> includes) {

    // Hack for local modules in the entrypoint file
    for (auto cont : unit.get_containers()) {
        for (auto icont : unit.get_containers()) {
            icont->aliases[cont->name->str()] = LongName(cont->full_name());
        }
    }
    int removals = 0;

    for (unsigned int i = 0; i < unit.members.len(); i++) {
        auto& item = unit.members[i - removals];
        if (auto import = dynamic_cast<Import*>(&item)) {

            //
            // 1. Constructs an AST from the import                     <-
            // 2. Attaches a unique hash to each member                  | construct_ast_from_path
            // 3. Update the export table                                |
            // 4. Recursively resolves imports for the child AST ...... <-
            // 5. Resolve the import alias to the true (hashed) name for the parent
            //      This process also singles out the exported module
            // 6. Inlines the AST into the parent
            //
            std::string import_name = import->target.str();
            if (import->is_stdlib) {
                import_name = "__std__::" + import_name;
            }
            auto import_path_ref = includes.find(import_name);

            if(import_path_ref == includes.end()){
                Logger::error("Failed to resolve a path for the library: " + import_name);
                exit(1);
            }

            auto import_path = import_path_ref->second;



            auto ast = construct_ast_from_path(import_path, includes);
            auto target_mod = all_exports[import_path]["__default__"];
            if (!target_mod)
                except(E_BAD_IMPORT, "Failed to locate default exported module from file: " + import_path);
            auto fullname = target_mod->full_name();
            resolve_aliased_symbols(unit, import->alias, fullname);
            merge_units(unit, std::move(*ast));
        }
    }
}
