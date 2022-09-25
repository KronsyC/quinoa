#include "./processor.h"
#include "../AST/ast.hh"
#include "../../lib/error.h"
#include "../compiler.h"
#include "../../lib/list.h"
#include "../../lib/logger.h"
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

                //TODO:
                // locate the exported module and change it's name to the alias
                // rename the rest of the modules to illegal names (i.e hiding so they arent accidentally accessed)

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

void genEntryPoint(CompilationUnit& unit)
{
    vector<Module*> entryPointCandidates;
    for(auto member:unit.items){
        if(instanceof<Module>(member)){
            auto mod = (Module*)member;
            if(mod->is("Entry"))entryPointCandidates.push_back(mod);
            
        }
    }
    if(entryPointCandidates.size() == 0)error("Failed to locate a suitable entrypoint");
    else if(entryPointCandidates.size() > 1){
        Logger::warn("Multiple Entry-Points were found, this may cause Unexpected Behavior");
    }
    auto entry = entryPointCandidates[0];
    if(entry->hasMethod("main")){
        auto main = entry->getMethod("main");
        if(!instanceof<Primitive>(main->returnType))error("The main method must return a primitive type");
        auto ret = (Primitive*)main->returnType;
        if(main->params.size() !=0 && main->params.size() !=2 )error("The main method must contain either 0 or 2 parameters");
        Logger::log("Using entry point '"+main->fullname->str()+"'");
        
        Entrypoint e(main->fullname);
        vector<Param*> params;
        params.push_back(new Param(new Primitive(PR_int32), new Ident("argc")));
        params.push_back(new Param(new TPtr(new Primitive(PR_string)), new Ident("argv")));
        e.name = new Ident("main");
        e.params = params;
        e.returnType = new Primitive(PR_int32);

        if(ret->type == PR_void){
            e.items.push_back(new Return(new Integer(0)));
        }
        else{
            auto call = new MethodCall();
            call->target = main->fullname;
            if(main->params.size() == 0)call->params = {};
            else call->params = {new Ident("argc"), new Ident("argv")};
            e.items.push_back(new Return(call));
        }
        unit.items.push_back(new Entrypoint(e));
    }
    else error("The Entrypoint '"+entry->name->str()+ "' does not contain a main method");
}

void Processor::process(CompilationUnit &unit, bool finalize)
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
     * Entrypoint location
     */
    resolveImports(unit);
    hoistDefinitions(unit);
    if(finalize)genEntryPoint(unit);
};
