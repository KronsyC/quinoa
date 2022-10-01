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
            //TODO: Implement config-file based imports
            if (import->isStdLib)
            {
                string rpath = regex_replace(import->target->str(), regex("\\."), "/");
                rpath = libq_dir + "/" + rpath + ".qn";

                auto file = readFile(rpath);
                auto ast = makeAst(file);

                // TODO:
                //  locate the exported module and change it's name to the alias
                //  rename the rest of the modules to illegal names (i.e hiding so they arent accidentally accessed)

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
    vector<TopLevelExpression *> items;
    for (auto child : unit.items)
    {
        if (instanceof <Module>(child))
        {
            auto mod = (Module *)child;
            for (auto d : mod->items)
            {
                if (instanceof <Method>(d))
                {
                    auto method = (Method*)d;
                    auto dec = new MethodPredeclaration;
                    dec->sig = method->sig;
                    items.push_back(dec);
                }
            }
        }
    }
    for (auto i : items)
    {
        pushf(unit.items, i);
    }
}


// Flattens a source tree as a single list of all of its members
// and their children bubbled up to the top-level
// useful for iterating over every single node of a given type
vector<Statement*> flatten(Block<Statement> stmnt){
    vector<Statement*> retval;
    for(auto item:stmnt.items){
        if(instanceof<Block<Statement>>(item)){
            auto nested = (Block<Statement>*)item;
            auto sub = flatten(*nested);
            for(auto c:sub)
                retval.push_back(c);
            
        }
        else if(instanceof<Statement>(item)){
            auto stmnt = (Statement*)item;
            auto flat = stmnt->flatten();
            for(auto i:flat)retval.push_back(i);
        }
    }
    return retval;
}

void resolveSelfReferences(Block<Statement> *content, Module *mod)
{
    auto flat = flatten(*content);

    for (auto m : flat)
    {
        if(instanceof<MethodCall>(m)){
            auto call = (MethodCall*)m;
            call->name->flatify();
            if(call->name->parts.size() == 1){
                Logger::log("Injecting Namespace for " + call->name->str());
                auto space = mod->name;
                pushf(call->name->parts, (Identifier*)space);
                call->name->flatify();
                Logger::log("Call is now "+call->name->str());
            }
        }
    }
}
void resolveSelfReferences(CompilationUnit &unit)
{
    for (auto tli : unit.items)
    {
        if (instanceof <Module>(tli))
        {
            auto mod = (Module *)tli;
            for (auto child : mod->items)
            {
                if (instanceof <Method>(child))
                {
                    auto fn = (Method *)child;
                    resolveSelfReferences(fn, mod);
                }
            }
        }
    }
}
void genEntryPoint(CompilationUnit &unit)
{
    vector<Module *> entryPointCandidates;
    for (auto member : unit.items)
    {
        if (instanceof <Module>(member))
        {
            auto mod = (Module *)member;
            if (mod->is("Entry"))
                entryPointCandidates.push_back(mod);
        }
    }
    if (entryPointCandidates.size() == 0)
        error("Failed to locate a suitable entrypoint");
    else if (entryPointCandidates.size() > 1)
    {
        Logger::warn("Multiple Entry-Points were found, this may cause Unexpected Behavior");
    }
    auto entry = entryPointCandidates[0];
    string entryName = "main";
    if (entry->hasMethod(entryName))
    {
        auto main = entry->getMethod(entryName);
        auto name = main->fullname();
        for (auto p : name->parts)
        {
            auto pname = p->str();
        }

        Entrypoint e(main->sig);
        e.sig = new MethodSignature();
        vector<Param *> params;
        params.push_back(new Param(new Primitive(PR_int32), new Ident("argc")));
        params.push_back(new Param(new TPtr(new Primitive(PR_string)), new Ident("argv")));
        e.sig->name = new Ident("main");
        e.sig->params = params;
        e.sig->returnType = new Primitive(PR_int32);
        auto call = new MethodCall();
        call->target = main->sig;
        auto ret = (Primitive *)main->sig->returnType;
        if (ret->type == PR_void)
        {
            call->params = {};
            e.items.push_back(new Return(new Integer(0)));
        }
        else
        {

            call->params = {new Ident("argc"), new Ident("argv")};
            e.items.push_back(new Return(call));
        }
        unit.items.push_back(new Entrypoint(e));
    }
    else
        error("The Entrypoint '" + entry->name->str() + "' does not contain a main method");
}

static std::map<PrimitiveType, std::string> primitive_group_mappings{
    PRIMITIVES_ENUM_GROUPS  
};
int getCompatabilityScore(MethodSigStr base, MethodSigStr target){
    Logger::debug("Comparing " + base.str() + " against "+ target.str());
    // if(base.)
    if(base.name->str() != target.name->str())return -1;
    // compare param lengths TODO: Reimplement this once varargs are implemented
    if(base.params.size() != target.params.size())return -1;
    // Start with a base score, each infraction has a cost based on how different it is
    int score = 0;
    for(int i = 0;i<base.params.size();i++){
        auto baram = base.params[i]->type;
        auto taram = target.params[i]->type;
        if(baram->equals(taram))continue;
        if(instanceof<Primitive>(baram) && instanceof<Primitive>(taram)){
            // same group, different type is +1, otherwise no match

            auto bprim = (Primitive*)baram;
            auto tprim = (Primitive*)taram;
            auto bg = primitive_group_mappings[bprim->type];
            auto tg = primitive_group_mappings[tprim->type];
            if(bg == tg)score++;
            else score = -1;

        }

    }

    //TODO: Implement Type Reference Inheritance Tree Crawling
    return score;
}   

// Takes a function and returns the best matching signature that it fits into to be called
MethodSignature* qualify(MethodCall* call, std::map<std::string, MethodSignature*> sigs){

    auto params = call->params;
    vector<Param*> testparams;
    for(auto p:params)testparams.push_back(new Param(p->getType(), nullptr));
    auto callsig = new MethodSignature;
    callsig->name = call->name->last();
    callsig->params = testparams;
    auto sigstr = callsig->sigstr();

    CompoundIdentifier callname(call->name->parts);
    callname.parts.pop_back();
    callname.parts.push_back((Identifier*)new Ident(sigstr.str()));
    auto fn = sigs[callname.str()];
    if(fn == nullptr){
        sigs.erase(callname.str());
        // Run Compatibility Checks on each sigstr pair to find
        // the most compatible function to match to
        vector<int> compatabilityScores;
        for(auto sigpair:sigs){
            auto sig = sigpair.second;
            auto name = sigpair.first;
            int compat = getCompatabilityScore(sig->sigstr(), callsig->sigstr());
            compatabilityScores.push_back(compat);
        }
        int max = -1;
        int prev = -1;
        int idx = -1;
        for(int i = 0; i<compatabilityScores.size();i++){
            auto s = compatabilityScores[i];
            if(s==-1)continue;
            if(s<=max || max == -1){
                prev = max;
                max = s;
                idx = i;
            }
        }
        if(idx == -1)return nullptr;
        int ind = 0;
        for(auto pair:sigs){
            if(ind == idx)return pair.second;
            ind++;
        }
        return nullptr;
    }
    return fn;
}
void qualifyCalls(Block<Statement>& code, std::map<std::string, MethodSignature*> sigs){
    auto flat = flatten(code);
    for(auto item:flat){
        if(instanceof<MethodCall>(item)){
            auto call = (MethodCall*)item;
            auto tgtsig = qualify(call, sigs);
            if(tgtsig==nullptr)error("Failed to locate appropriate function call");
            call->target = tgtsig;
        }
    }

}
void qualifyCalls(CompilationUnit& unit){
    // Construct a table of all call names -> their signatures
    std::map<std::string, MethodSignature*> sigs;
    for(auto item:unit.items){
        if(instanceof<Module>(item)){
            auto mod = (Module*)item;
            for(auto item:mod->items){
                if(instanceof<Method>(item)){
                    auto method = (Method*)item;
                    auto name = method->sig->sourcename();
                    Logger::debug("Found Signature "+ name);
                    sigs[name] = method->sig;
                }
            }
        }
    }

    // Attempt to Qualify all Calls
    for(auto item:unit.items){
        if(instanceof<Module>(item)){
            auto mod = (Module*)item;
            for(auto item:mod->items){
                if(instanceof<Method>(item)){
                    auto method = (Method*)item;
                    qualifyCalls(*method, sigs);
                }
            }
        }
    }
}
void Processor::process(CompilationUnit &unit, bool finalize)
{
    /**
     * Preprocess the tree via various different processes:
     * ----------------------------------------------------
     * ✅ Import resolution
     * ✅ Method Shorthand Self-Referencing (foo() -> method.foo())
     * Generic Implementation (Type Substitution and method gen)
     * ✅ Method Mangling (For Overloads)
     * ✅ Function Call Qualification (Signature Reference Injection)
     * Duplicate Name Detection
     * ✅ Function/Property Hoisting (at the module level)
     * Type Checking
     * Unused Variable Warning / Removal
     * Unreachable Code Warning / Removal
     * Static Statement Resolution ( 11 + 4 -> 15 )
     * Local Initializer Hoisting (optimization) (may require renames for block-scoped overrides)
     * ✅ Entrypoint Generation
     */

    resolveImports(unit);
    resolveSelfReferences(unit);
    Logger::debug("Resolved Self Refs");
    if (finalize)
    {
        qualifyCalls(unit);
        Logger::debug("Successfully Qualified Calls");
        hoistDefinitions(unit);
        Logger::debug("Hoisted Definitions");
        genEntryPoint(unit);
        Logger::debug("Generated Entry Point");
    }

};
