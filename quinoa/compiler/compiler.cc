#include "./compiler.h"
#include "./Lexer/lexer.h"
#include "./Parser/parser.h"
#include "./Codegen/codegen.hh"
#include "./Processor/processor.h"
#include "../lib/logger.h"
#include<fstream>

CompilationUnit makeAst(std::string sourceCode){
    auto toks = Lexer::lexify(sourceCode);
    auto ast = Parser::makeAst(toks);
    Processor::process(ast);
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

llvm::Module* createModule(std::string sourceCode, bool log){
    // Lex the file into a token vector
    auto tokens = Lexer::lexify(sourceCode);
    if(log)Logger::log("Lexed the Source File");
    // Assemble an AST from the tokens
    auto ast = Parser::makeAst(tokens);
    if(log)Logger::log("Generated the AST");
    Processor::process(ast);
    if(log)Logger::log("Processed the AST");
    auto mod = Codegen::codegen(ast);
    if(log)Logger::log("Generated LLVM IR Code");
    return mod;
}
std::string compile(std::string sourceCode){
    auto mod = createModule(sourceCode, true);
    std::string output;
    llvm::raw_string_ostream rso(output);
    mod->print(rso, nullptr);
    return output;

}
