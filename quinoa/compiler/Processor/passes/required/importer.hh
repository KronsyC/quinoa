#pragma once
#include "../include.h"
#include <ctime>
#include <regex>
#include <unistd.h>
#include "../../../compiler.h"
using namespace std;

std::string gen_random_str(int size)
{
  static const char choices[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  int len = sizeof(choices);
  std::string ret;
  ret.reserve(size);
  for (int i = 0; i < size; ++i)
  {
    ret += choices[rand() % (len - 1)];
  }
  return ret;
}
static std::vector<std::string> usedHashes;
// Same as genRandomString, but checks for collisions
std::string gen_random_safe_string(int size)
{
  while (1)
  {
    auto str = gen_random_str(size);
    if (!includes(usedHashes, str))
      return str;
  }
};

std::map<std::string, Module*> gen_export_table(CompilationUnit& unit){
  std::map<std::string, Module*> exports;
  Logger::debug("Building Export Table");
  for(auto mod:unit.getAllModules()){
    if(mod->isImported)continue;
    if(auto comp = mod->comp("Exported")){
      Logger::debug("Found Export " + mod->name->str());
      if(comp->params.size()){
        if(comp->params.size() != 1)error("The `Exported` attribute only takes one parameter");
        auto name = comp->params[0];
        if(!instanceof<String>(name))error("An Exports name must be a string");
        auto nm = (String*)name;
        Logger::debug("Named export");
        exports[nm->value] = mod;
      }
      else{
        Logger::debug("Default Export");
        exports["__default__"] = mod;
      }
    }
  }
  return exports;
}


void prefixify_children(CompilationUnit &unit, std::string prefix)
{
  for (auto mod : unit.getAllModules())
    if (!mod->isImported)
      pushf(mod->name->parts, (Identifier *)Ident::get("$" + prefix));
}
void deAliasify(CompilationUnit &unit, CompoundIdentifier *alias,
                CompoundIdentifier *fullname)
{
  for (auto method : unit.getAllMethods())
  {
    auto content = method->flatten();

    for (auto member : content)
    {

      // Dealiasify both member access and direct references
      if (instanceof <CompoundIdentifier>(member))
      {
        auto ident = (CompoundIdentifier *)member;
        auto ns = ident->all_but_last();
        if (ns->equals(alias))
        {
          auto name = ident->last();
          CompoundIdentifier deAliasedName({fullname, name});
          *ident = deAliasedName;
        }
        delete ns;

        if(ident->equals(alias)){
          *ident = *fullname;
        }
      }
    }
  }
}
void merge_units(CompilationUnit &tgt, CompilationUnit donor)
{
  for (auto e : donor.take())
  {
    e->isImported = true;
    if(includes(tgt,e))continue;
    tgt.push_back(e);
  }
}

static std::map<std::string, std::map<std::string, Module *>> exports;
static std::map<std::string, CompilationUnit *> import_cache;
CompilationUnit get_ast_from_path(std::string path, Ident* filename)
{
  Logger::log("Importing module from " + path);
  auto cached = import_cache[path];
  if (!cached)
  {
    auto file = readFile(path);
    auto ast = makeAst(file, path);

    // add the unique namespace prefix to the module
    auto prefix = gen_random_str(10);
    // prefixify_children(ast, prefix);

    auto export_table = gen_export_table(ast);
    exports[path] = export_table;

    import_cache[path] = new CompilationUnit(ast);
    return ast;
  }
  else{
    return *cached;

  }
}


void resolve_imports(CompilationUnit &unit)
{
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
  for (int i = 0; i < unit.size(); i++)
  {
    auto item = unit[i - removals];
    if (item->isImport())
    {
      auto import = (Import *)item;
      // TODO: Implement config-file based imports (allows custom stdlib and a
      // ton of
      //  other benefits)
      if (import->isStdLib)
      {
        string rpath = regex_replace(import->target->str(), regex("::"), "/");
        rpath = libq_dir + "/" + rpath + ".qn";
        auto filename = import->target->last();
        auto ast = get_ast_from_path(rpath, filename);


        // Injects the cryptic namespace
        // into the unit
        merge_units(unit, ast);

        auto table = exports[rpath];

        auto mod = import->member?table[import->member->str()]:table["__default__"];
        if(!mod)error("Failed to import Module");
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