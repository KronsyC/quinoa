#include "../AST/container.hh"
#include "../AST/primary.hh"

TypeVec parse_type_args(std::vector<Token>& toks, bool is_fish = true);
std::vector<std::shared_ptr<Generic>> parse_generics(std::vector<Token>& toks);
_Type parse_type(std::vector<Token>& toks, Container* container = nullptr);
