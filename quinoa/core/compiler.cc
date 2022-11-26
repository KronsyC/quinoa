#include "./compiler.h"
#include "./Lexer/lexer.h"
#include "./Parser/parser.h"
#include "./Preprocessor/preprocessor.hh"
#include "./Codegen/codegen.hh"
#include <fstream>
#include <sys/stat.h>
#define TMP_DIR (std::string(QUINOA_DIR) + "/tmp")


std::unique_ptr<CompilationUnit> make_ast(std::string sourceCode, std::string path, bool process)
{
    auto toks = Lexer::lexify(sourceCode, path);
    auto ast = Parser::make_ast(toks);
    if(process)Preprocessor::process_ast(*ast, false);
    return ast;
}
std::string read_file(std::string path)
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

enum OutputType{
    LL_IR,
    ASM,
    OBJ,
    EXE,
};

//
// Infer the compiler output format based on the output file
//
OutputType get_output_type(std::string output_name){

    if(output_name.ends_with(".o"))return OutputType::OBJ;
    if(output_name.ends_with(".ll"))return OutputType::LL_IR;
    if(output_name.ends_with(".s"))return OutputType::ASM;
    return OutputType::EXE;
}

std::string llmod_to_str(llvm::Module* llmod){
    std::string Str;
    llvm::raw_string_ostream OS(Str);
    llmod->print(OS, nullptr);
    return Str;
}

void emit_llir(llvm::Module* mod, std::string path){
    std::ofstream file(path);
    auto ll_ir = llmod_to_str(mod);
    file << ll_ir;
    file.close();
}
void emit_asm(llvm::Module* mod, std::string path){

    auto tmp_file = TMP_DIR+"/tmp_ir.ll";
    emit_llir(mod, tmp_file);
    std::string command = "clang -S " + tmp_file + " -o " + path;
    system(command.c_str());

}
void emit_obj(llvm::Module* mod, std::string path){
    auto tmp_file = TMP_DIR+"/tmp_ir.ll";
    emit_llir(mod, tmp_file);
    std::string command = "clang -c " + tmp_file + " -o " + path;
    system(command.c_str());
}
void emit_exe(llvm::Module* mod, std::string path){
    auto tmp_file = TMP_DIR+"/tmp_ir.ll";
    emit_llir(mod, tmp_file);
    std::string command = "clang " + tmp_file + " -o " + + "\"" + path + "\"";
    system(command.c_str());
}

bool is_dir(std::string path){
    struct stat file_stats;


    if(stat(path.c_str(), &file_stats) == 0){

        return file_stats.st_mode & S_IFDIR;

    }
    else except(E_ERR, "Failed to stat() " + path);
}
void compile(std::string path, ClargParser& cp)
{


    auto source = read_file(path);
    auto toks = Lexer::lexify(source, path);

    auto ast = Parser::make_ast(toks);

    Preprocessor::process_ast(*ast, true);

    Logger::log("Preprocessed the tree");

    auto ll_mod = Codegen::codegen(*ast);

    Logger::log("Generated the LLIR");

    auto output_path = cp.get_clarg<std::string>("o");
    auto output_type = get_output_type(output_path);

    switch(output_type){
        case OutputType::LL_IR:{
            emit_llir(ll_mod, output_path);
            break;
        }
        case OutputType::ASM:{
            emit_asm(ll_mod, output_path);
            break;
        }
        case OutputType::OBJ:{
            emit_obj(ll_mod, output_path);
            break;
        }
        case OutputType::EXE: {
            emit_exe(ll_mod, output_path);
            break;
        }
        default: except(E_INTERNAL, "Bad Output Type");
    }
    Logger::debug("Output to: " + output_path);





}
