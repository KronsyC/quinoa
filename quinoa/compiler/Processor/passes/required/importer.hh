#pragma once
#include "../include.h"
#include <ctime>
#include <regex>
#include <unistd.h>
#include "../../../compiler.h"
using namespace std;


std::string gen_random_str(int size) {
  static const char choices[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  int len = sizeof(choices);
  std::string ret;
  ret.reserve(size);
  for (int i = 0; i < size; ++i) {
    ret += choices[rand() % (len - 1)];
  }
  return ret;
}
static std::vector<std::string> usedHashes;
// Same as genRandomString, but checks for collisions
std::string gen_random_safe_string(int size) {
  while (1) {
    auto str = gen_random_str(size);
    if (!includes(usedHashes, str))
      return str;
  }
};
Module *get_primary_export(CompilationUnit &unit) {
  Module *ret = nullptr;
  for (auto mod : unit.getAllModules()) {
    if (mod->is("Exported")) {
      if (ret )
        error("Cannot Export Multiple Modules from a file");
      mod->remove("Exported");
      ret = mod;
    }
  }
  return ret;
};

void prefixify_children(CompilationUnit &unit, std::string prefix) {
  for (auto mod : unit.getAllModules()) 
    if(!mod->isImported)
      pushf(mod->name->parts, (Identifier *)Ident::get("[" + prefix + "]"));
}
void deAliasify(CompilationUnit &unit, CompoundIdentifier *alias,
                CompoundIdentifier *fullname) {
  for (auto method : unit.getAllMethods()) {
    auto content = method->flatten();

    for (auto member : content) {
      if (instanceof<CompoundIdentifier>(member)) {
        auto ident = (CompoundIdentifier*)member;
        auto ns = ident->all_but_last();
        if(ns->equals(alias)){
          delete ns;
          auto name = ident->last();
          CompoundIdentifier deAliasedName({fullname, name});
          *ident = deAliasedName;
        }
      }
    }
  }
}
void merge_units(CompilationUnit &tgt, CompilationUnit donor) {
  for (auto e : donor.take()) {
    e->isImported = true;
    tgt.push_back(e);
  }
}
// Keeps track of the absolute paths of every single file currently imported
static std::vector<std::string> imports;

// Maintain aliases for each file path
// This prevents people from being able to see the directory
// structure of the host machine, which is a potential security
// risk
static std::map<std::string, std::string> path_aliases;

CompilationUnit get_ast_from_path(std::string path) {
  Logger::log("Importing module from " + path);
  auto file = readFile(path);
  auto ast = makeAst(file, path);
  return ast;
}


static std::map<std::string, Module*> exports;

void resolve_imports(CompilationUnit &unit) {
  // TODO: Load this from the project config files, this wont work on any other
  // pc
  string libq_dir = LIBQ_DIR;

  /**
   * - Pulls in the relevant Modules
   * - Skips a module if their path is already imported
   * - Renames them to their proper aliases
   * - Changes any self-references to the module as the new aliased name
   *
   */
  int removals = 0;
  for (int i = 0; i < unit.size(); i++) {
    auto item = unit[i - removals];
    if (item->isImport()) {
      auto import = (Import *)item;
      // TODO: Implement config-file based imports (allows custom stdlib and a
      // ton of
      //  other benefits)
      if (import->isStdLib) {
        string rpath = regex_replace(import->target->str(), regex("\\."), "/");
        rpath = libq_dir + "/" + rpath + ".qn";
        if (!includes(imports, rpath)) {
          imports.push_back(rpath);
          auto ast = get_ast_from_path(rpath);
          auto primary_export = get_primary_export(ast);
          if (primary_export == nullptr)
            error("Failed to Import Module " + import->target->str());
          auto filename = import->target->last();
          primary_export->name->parts.pop_back();
          primary_export->name->parts.push_back(filename);
          auto prefix = gen_random_str(10);
          path_aliases[rpath] = prefix;



          exports[rpath] = primary_export;

          // Injects the cryptic namespace
          // into the unit
          prefixify_children(ast, prefix);  
          merge_units(unit, ast);      
        }

        auto mod = exports[rpath];
        if(!mod)error("Failed to Locate Module " + rpath);

        // Replace all references to the alias with the actual name
        auto name = mod->name;
        auto alias = import->alias;
        deAliasify(unit, alias, name);
      }

      else
        error("Non-stdlib imports are not yet supported");
      unit.erase(unit.begin() + i - removals);
      removals++;
    }
  }
}