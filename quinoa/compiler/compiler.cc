#include "./compiler.h"
#include "./Lexer/lexer.h"
#include "./Parser/parser.h"
#include "./Codegen/codegen.hh"
#include "./Processor/processor.h"
#include "../lib/logger.h"
#include<fstream>

CompilationUnit* makeAst(std::string sourceCode, std::string path, bool process){
    auto toks = Lexer::lexify(sourceCode, path);
    auto ast = Parser::makeAst(toks);
    if(process)Processor::process(ast, false);
    return ast;
}
std::string readFile(std::string path){
        // Read the file into a string
        std::ifstream file(path);
        if(!file.good())error("Failed to read file '"+path+"'");
        file.seekg(0, std::ios::end);
        size_t filesize = file.tellg();
        std::string fileContent(filesize, ' ');
        file.seekg(0);
        file.read(&fileContent[0], filesize);
        return fileContent;
}

llvm::Module* createModule(std::string sourceCode, std::string path, bool log){
    // Lex the file into a token vector
    auto ast = makeAst(sourceCode, path, false);
    Logger::debug("Parsed");
    Processor::process(ast, true);
    if(log)Logger::log("Generated the AST");
    auto mod = Codegen::codegen(*ast);
    if(log)Logger::log("Generated LLVM IR Code");
    return mod;
}
std::string compile(std::string sourceCode, std::string path){
    auto mod = createModule(sourceCode, path, true);
    std::string output;
    llvm::raw_string_ostream rso(output);
    mod->print(rso, nullptr);
    return output;
    // return sourceCode;

}
