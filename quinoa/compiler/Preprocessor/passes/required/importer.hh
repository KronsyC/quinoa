#pragma once
#include "../include.h"
#include <ctime>
#include <regex>
#include <unistd.h>
#include "../../../compiler.h"
using namespace std;

std::string gen_random_str(unsigned int size)
{
  static const char choices[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  int len = sizeof(choices);
  std::string ret;
  ret.reserve(size);
  for (unsigned int i = 0; i < size; ++i)
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

std::map<std::string, Container*> gen_export_table(CompilationUnit& unit){
  std::map<std::string, Container*> exports;
  for(auto mod:unit.get_containers()){
    if(mod->is_imported)continue;
    if(mod->has_compositor("Exported")){
      exports["__default__"] = mod;
    }
  }
  return exports;
}

void prefixify_children(CompilationUnit &unit, std::string prefix)
{
  static std::map<std::string, std::shared_ptr<Name>> prefix_cache;
  std::shared_ptr<Name> pfx;
  if(auto entry = prefix_cache[prefix]){
    pfx = entry;
  }
  else{
    auto obj = std::make_shared<Name>("_"+prefix+"_");
    prefix_cache[prefix] = obj;
    pfx = obj;
  }
  for (auto& mod : unit.get_containers()){
    if (mod->is_imported)continue;
    mod->name_space = pfx;
  }

}
void deAliasify(CompilationUnit &unit, LongName& alias,
                LongName& fullname)
{
  for(auto method: unit.get_methods()){
    if(!method->content)continue;
    for(auto code : method->content->flatten()){
      if(auto call = dynamic_cast<MethodCall*>(code)){
        if(call->name->container->name->str() == alias.str()){
          call->name->container->name = std::make_unique<LongName>(fullname);
        }
      }
    }
  }
  // for (auto method : unit.get_methods())
  // {
  //   auto content = method->flatten();

  //   for (auto member : content)
  //   {

  //     // Dealiasify both member access and direct references
  //     if (instanceof <CompoundIdentifier>(member))
  //     {
  //       auto ident = (CompoundIdentifier *)member;
  //       if(ident->equals(alias)){
  //         *ident = *fullname;
  //         continue;
  //       }

  //       auto ns = ident->all_but_last();
  //       if (ns->equals(alias))
  //       {
  //         auto name = ident->last();
  //         CompoundIdentifier deAliasedName;
  //         for(auto p:*fullname)deAliasedName.push_back(p);
  //         deAliasedName.push_back(name);
  //         *ident = deAliasedName;
  //         continue;
  //       }


  //     }
  //   }
  // }
  for(auto mod:unit.get_containers()){
    for(auto& c:mod->compositors){
      if(c->name->str() == alias.str()){
        c->name = std::make_unique<LongName>(fullname);
      }
    }
  }
}
void merge_units(CompilationUnit *tgt, CompilationUnit donor)
{
  auto transferred = donor.transfer();
  for(size_t i = 0; i < transferred.size(); i++){
    auto member = std::move(transferred[i]);
    member->is_imported = true;
    tgt->members.push(std::move(member));
  }
}

static std::map<std::string, std::map<std::string, Container*>> exports;
static std::map<std::string, std::unique_ptr<CompilationUnit>> import_cache;
CompilationUnit* get_ast_from_path(std::string path, std::string filename)
{
  Logger::log("Importing module from " + path);
  auto cached = import_cache[path].get();
  if (!cached)
  {
    auto file = readFile(path);
    auto ast = make_ast(file, path);

    // add the unique namespace prefix to the module
    auto prefix = gen_random_safe_string(10);
    prefixify_children(*ast, prefix);

    auto export_table = gen_export_table(*ast);
    exports[path] = export_table;

    import_cache[path] = std::move(ast);
    return import_cache[path].get();
  }
  else{
    return cached;

  }
}


void resolve_imports(CompilationUnit *unit)
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
  for (unsigned int i = 0; i < unit->members.len(); i++)
  {
    auto& item = unit->members[i - removals];
    if(auto import = dynamic_cast<Import*>(&item))
    {
      // TODO: Implement config-file based imports (allows custom stdlib and a
      // ton of
      //  other benefits)
      if (import->is_stdlib)
      {
        string rpath = std::regex_replace(import->target.str(), std::regex("::"), "/");
        rpath = libq_dir + "/" + rpath + ".qn";
        auto filename = import->target.last();
        auto ast = get_ast_from_path(rpath, filename.str());


        auto table = exports[rpath];

        auto mod = table["__default__"];
        if(!mod)error("Failed to import Module");
        // Replace all references to the alias with the actual name
        auto alias = import->alias;
        auto full_name = mod->full_name();
        deAliasify(*unit, alias, full_name);

        merge_units(unit, *ast);
      }

      else
        error("Non-stdlib imports are not yet supported");
      unit->members.remove(i - removals);
      removals++;
    }
  }
}