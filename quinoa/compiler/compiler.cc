#include "./compiler.h"
#include "../lib/logger.h"
#include "./Lexer/lexer.h"
#include "./Parser/parser.h"
#include "./Preprocessor/preprocessor.hh"
#include <fstream>

std::unique_ptr<CompilationUnit> make_ast(std::string sourceCode, std::string path, bool process)
{
    auto toks = Lexer::lexify(sourceCode, path);
    auto ast = Parser::make_ast(toks);
    if(process)Preprocessor::process_ast(*ast);
    return ast;
}
std::string readFile(std::string path)
{
    // Read the file into a string
    std::ifstream file(path);
    if(!file.good())
	except(E_ERR, "Bad File Path: " + path);
    file.seekg(0, std::ios::end);
    size_t filesize = file.tellg();
    std::string fileContent(filesize, ' ');
    file.seekg(0);
    file.read(&fileContent[0], filesize);
    return fileContent;
}

// llvm::Module* createModule(std::string sourceCode, std::string path, bool log)
// {
//     // Lex the file into a token vector
//     auto ast = makeAst(sourceCode, path, false);
//     Logger::debug("Parsed");
//     // Processor::process(ast, true);
//     // if(log)
// 	// Logger::log("Generated the AST");
//     // auto mod = Codegen::codegen(*ast);
//     // if(log)
// 	// Logger::log("Generated LLVM IR Code");
//     // return mod;
//     return nullptr;
// }
std::string compile(std::string sourceCode, std::string path)
{
    auto toks = Lexer::lexify(sourceCode, path);
    auto ast = Parser::make_ast(toks);
    Preprocessor::process_ast(*ast);
    return "NOT APPLICABLE";
}
