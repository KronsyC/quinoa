#include "./processor.h"
#include "../AST/ast.hh"
#include "../../lib/error.h"
#include "../compiler.h"
#include "../../lib/list.h"
#include <regex>
using namespace std;

void mergeUnits(CompilationUnit &tgt, CompilationUnit donor)
{
    for (auto e : donor.items)
    {
        tgt.push(e);
    }
}
void resolveImports(CompilationUnit &unit)
{
    string libq_dir = "/home/casey/quinoa-ref/libq";
    /**
     * - Pulls in the relevant Modules
     * - Renames them to their proper aliases
     * - Changes any self-references to the module as the new aliased name
     *
     */
    int removals = 0;
    for (int i = 0; i < unit.items.size(); i++)
    {
        auto item = unit.items[i - removals];
        if (instanceof <Import>(item))
        {
            auto import = (Import *)item;
            if (import->isStdLib)
            {
                string rpath = regex_replace(import->target->str(), regex("\\."), "/");
                rpath = libq_dir + "/" + rpath + ".qn";

                auto file = readFile(rpath);
                auto ast = makeAst(file);

                // locate the exported module and change it's name to the alias
                // rename the rest of the modules to illegal names (i.e hiding so they arent accidentally accessed)
                // TODO:^

                mergeUnits(unit, ast);
            }

            else
                error("Non-stdlib imports are not yet supported");
            unit.items.erase(unit.items.begin() + i - removals);
            removals++;
        }
    }
};
void hoistDefinitions(CompilationUnit &unit)
{
    vector<TopLevelExpression*> items;
    for (auto child : unit.items)
    {
        if (instanceof <Module>(child))
        {
            auto mod = (Module *)child;
            for (auto d : mod->items)
            {
                if (instanceof <Method>(d))
                {
                    auto method = (Method *)d;
                    auto def = new MethodDefinition;
                    def->name = new CompoundIdentifier({mod->name, method->name});
                    def->params = method->params;
                    def->returnType = method->returnType;
                    items.push_back(def);
                }
            }
        }
    }
    for(auto i:items){
        pushf(unit.items, i);
    }
}
void Processor::process(CompilationUnit &unit)
{
    /**
     * Preprocess the tree via various different processes
     * Import resolution
     * Name Collision Detection
     * Function/Property Hoisting (at the module level)
     * Type Checking
     * Unused Variable Warning / Removal
     * Unreachable Code Warning / Removal
     * Static Statement Resolution ( 11 + 4 -> 15 )
     * Local Initializer Hoisting (optimization) (may require renames for block-scoped overrides)
     */
    resolveImports(unit);
    hoistDefinitions(unit);
};
