#pragma once
#include "../processor.h"
#include "../../compiler.h"
#include <regex>

using namespace std;

void mergeUnits(CompilationUnit &tgt, CompilationUnit donor) {
  for (auto e : donor.items) {
    tgt.push(e);
  }
}
void resolveImports(CompilationUnit &unit) {
  // TODO: Load this from the project config files, this wont work on any other
  // pc
  string libq_dir = "/home/casey/quinoa-ref/libq";
  static std::vector<std::string> imports;
  /**
   * - Pulls in the relevant Modules
   * - Skips a module if their path is already imported
   * - Renames them to their proper aliases
   * - Changes any self-references to the module as the new aliased name
   *
   */
  int removals = 0;
  for (int i = 0; i < unit.items.size(); i++) {
    auto item = unit.items[i - removals];
    if (instanceof <Import>(item)) {
      auto import = (Import *)item;
      // TODO: Implement config-file based imports (allows custom stdlib and a
      // ton of
      //  other benefits)
      if (import->isStdLib) {
        string rpath = regex_replace(import->target->str(), regex("\\."), "/");
        rpath = libq_dir + "/" + rpath + ".qn";
        if (!includes(imports, rpath)) {
          imports.push_back(rpath);
          auto file = readFile(rpath);
          auto ast = makeAst(file, rpath);

          // TODO:
          //  locate the exported module and change it's name to the alias
          //  rename the rest of the modules to illegal names (i.e hiding so
          //  they arent accidentally accessed)

          mergeUnits(unit, ast);
        }

      }

      else
        error("Non-stdlib imports are not yet supported");
      unit.items.erase(unit.items.begin() + i - removals);
      removals++;
    }
  }
}