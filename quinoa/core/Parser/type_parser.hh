#include "../AST/primary.hh"
#include "../AST/container.hh"


std::vector<std::shared_ptr<Type>> parse_type_args(std::vector<Token>& toks, bool is_fish = true);
std::vector<std::shared_ptr<Generic>> parse_generics(std::vector<Token>& toks);
std::shared_ptr<Type> parse_type(std::vector<Token>& toks, Container* container = nullptr);
