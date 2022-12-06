#pragma once

#define DEFINITIONS_STR \
	std::vector<std::string> alias = {};\
	std::string type = "";\
	bool ind = false;\
	bool dind = false;\
	int infix = 0;\
	bool postfix = false;\
	bool prefix = false;\
	std::vector<int> intrinsic = {};\
	

#define DEFINITIONS_ARGS \
	\
	, std::vector<std::string> alias = {}\
	, std::string type = ""\
	, bool ind = false\
	, bool dind = false\
	, int infix = 0\
	, bool postfix = false\
	, bool prefix = false\
	, std::vector<int> intrinsic = {}

#define DEFINITIONS_DEFAULT_ASSIGNMENTS \
	this->alias = alias;\
	this->type = type;\
	this->ind = ind;\
	this->dind = dind;\
	this->infix = infix;\
	this->postfix = postfix;\
	this->prefix = prefix;\
	this->intrinsic = intrinsic;\
	

#define DEFINITIONS_INITIALIZERS \
	new TokenDefinition(TT_notok, "notok", {}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_eof, "eof", {}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_comment, "comment", {}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_identifier, "identifier", {}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_literal_str, "literal_str", {}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_literal_true, "literal_true", {"true"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_literal_false, "literal_false", {"false"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_literal_int, "literal_int", {}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_literal_float, "literal_float", {}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_literal_char, "literal_char", {}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_void, "void", {"void"}, {"void"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_boolean, "boolean", {"bool"}, {"integer"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_string, "string", {"str"}, {"str"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_int8, "int8", {"i8", "char"}, {"integer"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_int16, "int16", {"i16"}, {"integer"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_int32, "int32", {"i32", "int"}, {"integer"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_int64, "int64", {"i64"}, {"integer"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_uint8, "uint8", {"u8", "byte"}, {"integer"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_uint16, "uint16", {"u16"}, {"integer"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_uint32, "uint32", {"u32", "uint"}, {"integer"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_uint64, "uint64", {"u64"}, {"integer"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_float16, "float16", {"f16"}, {"floating_point"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_float32, "float32", {"f32"}, {"floating_point"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_float64, "float64", {"f64", "float"}, {"floating_point"}, false, false, 0, false, false, {}),\
	new TokenDefinition(TT_double_quote, "double_quote", {"\""}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_hashtag, "hashtag", {"#"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_quote, "quote", {"'"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_l_paren, "l_paren", {"("}, "", true, false, 0, false, false, {}),\
	new TokenDefinition(TT_r_paren, "r_paren", {")"}, "", false, true, 0, false, false, {}),\
	new TokenDefinition(TT_op_generic, "op_generic", {"::<"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_l_brace, "l_brace", {"{"}, "", true, false, 0, false, false, {}),\
	new TokenDefinition(TT_r_brace, "r_brace", {"}"}, "", false, true, 0, false, false, {}),\
	new TokenDefinition(TT_l_square_bracket, "l_square_bracket", {"["}, "", true, false, 0, false, false, {}),\
	new TokenDefinition(TT_r_square_bracket, "r_square_bracket", {"]"}, "", false, true, 0, false, false, {}),\
	new TokenDefinition(TT_comma, "comma", {","}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_colon, "colon", {":"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_double_colon, "double_colon", {"::"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_semicolon, "semicolon", {";"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_arrow, "arrow", {"->"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_question_mark, "question_mark", {"?"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_at_symbol, "at_symbol", {"@"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_backslash, "backslash", {"\\"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_percent, "percent", {"%"}, "", false, false, 5, false, false, {}),\
	new TokenDefinition(TT_increment, "increment", {"++"}, "", false, false, 0, true, true, {}),\
	new TokenDefinition(TT_bitwise_not, "bitwise_not", {"~"}, "", false, false, 0, false, true, {}),\
	new TokenDefinition(TT_decrement, "decrement", {"--"}, "", false, false, 0, true, true, {}),\
	new TokenDefinition(TT_bang, "bang", {"!"}, "", false, false, 0, false, true, {}),\
	new TokenDefinition(TT_ampersand, "ampersand", {"&"}, "", false, false, 0, false, true, {}),\
	new TokenDefinition(TT_star, "star", {"*"}, "", false, false, 5, false, true, {}),\
	new TokenDefinition(TT_plus, "plus", {"+"}, "", false, false, 6, false, false, {}),\
	new TokenDefinition(TT_minus, "minus", {"-"}, "", false, false, 6, false, true, {}),\
	new TokenDefinition(TT_bool_or, "bool_or", {"||"}, "", false, false, 15, false, false, {}),\
	new TokenDefinition(TT_bool_and, "bool_and", {"&&"}, "", false, false, 14, false, false, {}),\
	new TokenDefinition(TT_dot, "dot", {"."}, "", false, false, 4, false, false, {}),\
	new TokenDefinition(TT_ellipsis, "ellipsis", {"..."}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_slash, "slash", {"/"}, "", false, false, 5, false, false, {}),\
	new TokenDefinition(TT_lesser, "lesser", {"<"}, "", false, false, 9, false, false, {}),\
	new TokenDefinition(TT_greater, "greater", {">"}, "", false, false, 9, false, false, {}),\
	new TokenDefinition(TT_lesser_eq, "lesser_eq", {"<="}, "", false, false, 9, false, false, {}),\
	new TokenDefinition(TT_greater_eq, "greater_eq", {">="}, "", false, false, 9, false, false, {}),\
	new TokenDefinition(TT_assignment, "assignment", {"="}, "", false, false, 16, false, false, {}),\
	new TokenDefinition(TT_equals, "equals", {"=="}, "", false, false, 10, false, false, {}),\
	new TokenDefinition(TT_not_equals, "not_equals", {"!="}, "", false, false, 10, false, false, {}),\
	new TokenDefinition(TT_bitwise_and, "bitwise_and", {"&"}, "", false, false, 11, false, false, {}),\
	new TokenDefinition(TT_bitwise_or, "bitwise_or", {"|"}, "", false, false, 13, false, false, {}),\
	new TokenDefinition(TT_bitwise_xor, "bitwise_xor", {"^"}, "", false, false, 13, false, false, {}),\
	new TokenDefinition(TT_bitwise_shl, "bitwise_shl", {"<<"}, "", false, false, 7, false, false, {}),\
	new TokenDefinition(TT_bitwise_shr, "bitwise_shr", {">>"}, "", false, false, 7, false, false, {}),\
	new TokenDefinition(TT_if, "if", {"if"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_else, "else", {"else"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_while, "while", {"while"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_for, "for", {"for"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_break, "break", {"break"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_continue, "continue", {"continue"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_switch, "switch", {"switch"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_case, "case", {"case"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_fallthrough, "fallthrough", {"fallthrough"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_import, "import", {"import"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_module, "module", {"module"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_seed, "seed", {"seed"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_struct, "struct", {"struct"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_enum, "enum", {"enum"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_func, "func", {"func"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_type, "type", {"type"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_is, "is", {"is"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_as, "as", {"as"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_return, "return", {"return"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_let, "let", {"let"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_const, "const", {"const"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_private, "private", {"pvt"}, "", false, false, 0, false, false, {}),\
	new TokenDefinition(TT_intr_add, "intr_add", {"@add"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_sub, "intr_sub", {"@sub"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_mul, "intr_mul", {"@mul"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_div, "intr_div", {"@div"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_mod, "intr_mod", {"@mod"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_bool_or, "intr_bool_or", {"@bool_or"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_bool_and, "intr_bool_and", {"@bool_and"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_get_member, "intr_get_member", {"@member"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_cmp_lesser, "intr_cmp_lesser", {"@cmp_lt"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_cmp_greater, "intr_cmp_greater", {"@cmp_gt"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_cmp_lesser_eq, "intr_cmp_lesser_eq", {"@cmp_lte"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_cmp_greater_eq, "intr_cmp_greater_eq", {"@cmp_gte"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_assign, "intr_assign", {"@assign"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_cmp_eq, "intr_cmp_eq", {"@cmp_eq"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_cmp_neq, "intr_cmp_neq", {"@cmp_neq"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_bitwise_and, "intr_bitwise_and", {"@bitwise_and"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_bitwise_or, "intr_bitwise_or", {"@bitwise_or"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_bitwise_xor, "intr_bitwise_xor", {"@bitwise_xor"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_bitwise_shl, "intr_bitwise_shl", {"@bitwise_shl"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_bitwise_shr, "intr_bitwise_shr", {"@bitwise_shr"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_power, "intr_power", {"@pow"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_sqrt, "intr_sqrt", {"@sqrt"}, "", false, false, 0, false, false, {1, 0}),\
	new TokenDefinition(TT_intr_nth_root, "intr_nth_root", {"@nth_root"}, "", false, false, 0, false, false, {2, 0}),\
	new TokenDefinition(TT_intr_bitwise_not, "intr_bitwise_not", {"@bitwise_not"}, "", false, false, 0, false, false, {1, 0}),\
	new TokenDefinition(TT_intr_bool_not, "intr_bool_not", {"@bool_not"}, "", false, false, 0, false, false, {1, 0}),\
	new TokenDefinition(TT_intr_pointer_to, "intr_pointer_to", {"@pointer_to"}, "", false, false, 0, false, false, {1, 0}),\
	new TokenDefinition(TT_intr_dereference, "intr_dereference", {"@deref"}, "", false, false, 0, false, false, {1, 0}),\
	new TokenDefinition(TT_intr_negate, "intr_negate", {"@negate"}, "", false, false, 0, false, false, {1, 0}),\
	new TokenDefinition(TT_intr_add_one, "intr_add_one", {"@inc"}, "", false, false, 0, false, false, {1, 0}),\
	new TokenDefinition(TT_intr_sub_one, "intr_sub_one", {"@dec"}, "", false, false, 0, false, false, {1, 0}),\
	new TokenDefinition(TT_intr_size_of, "intr_size_of", {"@size_of"}, "", false, false, 0, false, false, {0, 1}),\
	new TokenDefinition(TT_intr_make_slice, "intr_make_slice", {"@make_slice"}, "", false, false, 0, false, false, {2, 1}),\
	new TokenDefinition(TT_intr_subscript, "intr_subscript", {"@subscript"}, "", false, false, 0, false, false, {2, 0}),\
	

#define DEFINITIONS_ENUM_MEMBERS \
	TT_notok,\
	TT_eof,\
	TT_comment,\
	TT_identifier,\
	TT_literal_str,\
	TT_literal_true,\
	TT_literal_false,\
	TT_literal_int,\
	TT_literal_float,\
	TT_literal_char,\
	TT_void,\
	TT_boolean,\
	TT_string,\
	TT_int8,\
	TT_int16,\
	TT_int32,\
	TT_int64,\
	TT_uint8,\
	TT_uint16,\
	TT_uint32,\
	TT_uint64,\
	TT_float16,\
	TT_float32,\
	TT_float64,\
	TT_double_quote,\
	TT_hashtag,\
	TT_quote,\
	TT_l_paren,\
	TT_r_paren,\
	TT_op_generic,\
	TT_l_brace,\
	TT_r_brace,\
	TT_l_square_bracket,\
	TT_r_square_bracket,\
	TT_comma,\
	TT_colon,\
	TT_double_colon,\
	TT_semicolon,\
	TT_arrow,\
	TT_question_mark,\
	TT_at_symbol,\
	TT_backslash,\
	TT_percent,\
	TT_increment,\
	TT_bitwise_not,\
	TT_decrement,\
	TT_bang,\
	TT_ampersand,\
	TT_star,\
	TT_plus,\
	TT_minus,\
	TT_bool_or,\
	TT_bool_and,\
	TT_dot,\
	TT_ellipsis,\
	TT_slash,\
	TT_lesser,\
	TT_greater,\
	TT_lesser_eq,\
	TT_greater_eq,\
	TT_assignment,\
	TT_equals,\
	TT_not_equals,\
	TT_bitwise_and,\
	TT_bitwise_or,\
	TT_bitwise_xor,\
	TT_bitwise_shl,\
	TT_bitwise_shr,\
	TT_if,\
	TT_else,\
	TT_while,\
	TT_for,\
	TT_break,\
	TT_continue,\
	TT_switch,\
	TT_case,\
	TT_fallthrough,\
	TT_import,\
	TT_module,\
	TT_seed,\
	TT_struct,\
	TT_enum,\
	TT_func,\
	TT_type,\
	TT_is,\
	TT_as,\
	TT_return,\
	TT_let,\
	TT_const,\
	TT_private,\
	TT_intr_add,\
	TT_intr_sub,\
	TT_intr_mul,\
	TT_intr_div,\
	TT_intr_mod,\
	TT_intr_bool_or,\
	TT_intr_bool_and,\
	TT_intr_get_member,\
	TT_intr_cmp_lesser,\
	TT_intr_cmp_greater,\
	TT_intr_cmp_lesser_eq,\
	TT_intr_cmp_greater_eq,\
	TT_intr_assign,\
	TT_intr_cmp_eq,\
	TT_intr_cmp_neq,\
	TT_intr_bitwise_and,\
	TT_intr_bitwise_or,\
	TT_intr_bitwise_xor,\
	TT_intr_bitwise_shl,\
	TT_intr_bitwise_shr,\
	TT_intr_power,\
	TT_intr_sqrt,\
	TT_intr_nth_root,\
	TT_intr_bitwise_not,\
	TT_intr_bool_not,\
	TT_intr_pointer_to,\
	TT_intr_dereference,\
	TT_intr_negate,\
	TT_intr_add_one,\
	TT_intr_sub_one,\
	TT_intr_size_of,\
	TT_intr_make_slice,\
	TT_intr_subscript,\
	

#define INDENTATION_TYPES \
	IND_parens,\
	 IND_braces,\
	 IND_square_brackets,\
	 

#define INDENTATION_MAPPINGS \
	{IND_parens, {TT_l_paren, TT_r_paren}},\
	{IND_braces, {TT_l_brace, TT_r_brace}},\
	{IND_square_brackets, {TT_l_square_bracket, TT_r_square_bracket}},\
	

#define INFIX_ENUM_MEMBERS \
	BIN_percent, \
	BIN_star, \
	BIN_plus, \
	BIN_minus, \
	BIN_bool_or, \
	BIN_bool_and, \
	BIN_dot, \
	BIN_slash, \
	BIN_lesser, \
	BIN_greater, \
	BIN_lesser_eq, \
	BIN_greater_eq, \
	BIN_assignment, \
	BIN_equals, \
	BIN_not_equals, \
	BIN_bitwise_and, \
	BIN_bitwise_or, \
	BIN_bitwise_xor, \
	BIN_bitwise_shl, \
	BIN_bitwise_shr, \
	

#define INFIX_ENUM_MAPPINGS \
	{ TT_percent, BIN_percent}, \
	{ TT_star, BIN_star}, \
	{ TT_plus, BIN_plus}, \
	{ TT_minus, BIN_minus}, \
	{ TT_bool_or, BIN_bool_or}, \
	{ TT_bool_and, BIN_bool_and}, \
	{ TT_dot, BIN_dot}, \
	{ TT_slash, BIN_slash}, \
	{ TT_lesser, BIN_lesser}, \
	{ TT_greater, BIN_greater}, \
	{ TT_lesser_eq, BIN_lesser_eq}, \
	{ TT_greater_eq, BIN_greater_eq}, \
	{ TT_assignment, BIN_assignment}, \
	{ TT_equals, BIN_equals}, \
	{ TT_not_equals, BIN_not_equals}, \
	{ TT_bitwise_and, BIN_bitwise_and}, \
	{ TT_bitwise_or, BIN_bitwise_or}, \
	{ TT_bitwise_xor, BIN_bitwise_xor}, \
	{ TT_bitwise_shl, BIN_bitwise_shl}, \
	{ TT_bitwise_shr, BIN_bitwise_shr}, \
	

#define UNARY_ENUM_MEMBERS \
	PRE_increment, \
	POST_increment, \
	PRE_bitwise_not, \
	PRE_decrement, \
	POST_decrement, \
	PRE_bang, \
	PRE_ampersand, \
	PRE_star, \
	PRE_minus, \
	

#define PREFIX_ENUM_MAPPINGS \
	{ TT_increment, PRE_increment}, \
	{ TT_bitwise_not, PRE_bitwise_not}, \
	{ TT_decrement, PRE_decrement}, \
	{ TT_bang, PRE_bang}, \
	{ TT_ampersand, PRE_ampersand}, \
	{ TT_star, PRE_star}, \
	{ TT_minus, PRE_minus}, \
	

#define POSTFIX_ENUM_MAPPINGS \
	{ TT_increment, POST_increment}, \
	{ TT_decrement, POST_decrement}, \
	

#define PRIMITIVES_ENUM_MEMBERS \
	PR_void,\
	PR_boolean,\
	PR_string,\
	PR_int8,\
	PR_int16,\
	PR_int32,\
	PR_int64,\
	PR_uint8,\
	PR_uint16,\
	PR_uint32,\
	PR_uint64,\
	PR_float16,\
	PR_float32,\
	PR_float64,\
	

#define PRIMITIVES_ENUM_MAPPINGS \
	{ TT_void, PR_void},\
	{ TT_boolean, PR_boolean},\
	{ TT_string, PR_string},\
	{ TT_int8, PR_int8},\
	{ TT_int16, PR_int16},\
	{ TT_int32, PR_int32},\
	{ TT_int64, PR_int64},\
	{ TT_uint8, PR_uint8},\
	{ TT_uint16, PR_uint16},\
	{ TT_uint32, PR_uint32},\
	{ TT_uint64, PR_uint64},\
	{ TT_float16, PR_float16},\
	{ TT_float32, PR_float32},\
	{ TT_float64, PR_float64},\
	

#define PRIMITIVES_ENUM_NAMES \
	{PR_void, "void"},\
	{PR_boolean, "bool"},\
	{PR_string, "str"},\
	{PR_int8, "i8"},\
	{PR_int16, "i16"},\
	{PR_int32, "i32"},\
	{PR_int64, "i64"},\
	{PR_uint8, "u8"},\
	{PR_uint16, "u16"},\
	{PR_uint32, "u32"},\
	{PR_uint64, "u64"},\
	{PR_float16, "f16"},\
	{PR_float32, "f32"},\
	{PR_float64, "f64"},\
	

#define PRIMITIVES_ENUM_GROUPS \
	{PR_void, "void"},\
	{PR_boolean, "integer"},\
	{PR_string, "str"},\
	{PR_int8, "integer"},\
	{PR_int16, "integer"},\
	{PR_int32, "integer"},\
	{PR_int64, "integer"},\
	{PR_uint8, "integer"},\
	{PR_uint16, "integer"},\
	{PR_uint32, "integer"},\
	{PR_uint64, "integer"},\
	{PR_float16, "floating_point"},\
	{PR_float32, "floating_point"},\
	{PR_float64, "floating_point"},\
	

#define INTRINSICS_ENUM_MEMBERS \
	intr_add,\
	intr_sub,\
	intr_mul,\
	intr_div,\
	intr_mod,\
	intr_bool_or,\
	intr_bool_and,\
	intr_get_member,\
	intr_cmp_lesser,\
	intr_cmp_greater,\
	intr_cmp_lesser_eq,\
	intr_cmp_greater_eq,\
	intr_assign,\
	intr_cmp_eq,\
	intr_cmp_neq,\
	intr_bitwise_and,\
	intr_bitwise_or,\
	intr_bitwise_xor,\
	intr_bitwise_shl,\
	intr_bitwise_shr,\
	intr_power,\
	intr_sqrt,\
	intr_nth_root,\
	intr_bitwise_not,\
	intr_bool_not,\
	intr_pointer_to,\
	intr_dereference,\
	intr_negate,\
	intr_add_one,\
	intr_sub_one,\
	intr_size_of,\
	intr_make_slice,\
	intr_subscript,\
	

#define INTRINSICS_ENUM_NAMES \
	{ intr_add, "@add"},\
	{ intr_sub, "@sub"},\
	{ intr_mul, "@mul"},\
	{ intr_div, "@div"},\
	{ intr_mod, "@mod"},\
	{ intr_bool_or, "@bool_or"},\
	{ intr_bool_and, "@bool_and"},\
	{ intr_get_member, "@member"},\
	{ intr_cmp_lesser, "@cmp_lt"},\
	{ intr_cmp_greater, "@cmp_gt"},\
	{ intr_cmp_lesser_eq, "@cmp_lte"},\
	{ intr_cmp_greater_eq, "@cmp_gte"},\
	{ intr_assign, "@assign"},\
	{ intr_cmp_eq, "@cmp_eq"},\
	{ intr_cmp_neq, "@cmp_neq"},\
	{ intr_bitwise_and, "@bitwise_and"},\
	{ intr_bitwise_or, "@bitwise_or"},\
	{ intr_bitwise_xor, "@bitwise_xor"},\
	{ intr_bitwise_shl, "@bitwise_shl"},\
	{ intr_bitwise_shr, "@bitwise_shr"},\
	{ intr_power, "@pow"},\
	{ intr_sqrt, "@sqrt"},\
	{ intr_nth_root, "@nth_root"},\
	{ intr_bitwise_not, "@bitwise_not"},\
	{ intr_bool_not, "@bool_not"},\
	{ intr_pointer_to, "@pointer_to"},\
	{ intr_dereference, "@deref"},\
	{ intr_negate, "@negate"},\
	{ intr_add_one, "@inc"},\
	{ intr_sub_one, "@dec"},\
	{ intr_size_of, "@size_of"},\
	{ intr_make_slice, "@make_slice"},\
	{ intr_subscript, "@subscript"},\
	

#define INTRINSICS_ENUM_MAPPINGS \
	{ "@add", intr_add},\
	{ "@sub", intr_sub},\
	{ "@mul", intr_mul},\
	{ "@div", intr_div},\
	{ "@mod", intr_mod},\
	{ "@bool_or", intr_bool_or},\
	{ "@bool_and", intr_bool_and},\
	{ "@member", intr_get_member},\
	{ "@cmp_lt", intr_cmp_lesser},\
	{ "@cmp_gt", intr_cmp_greater},\
	{ "@cmp_lte", intr_cmp_lesser_eq},\
	{ "@cmp_gte", intr_cmp_greater_eq},\
	{ "@assign", intr_assign},\
	{ "@cmp_eq", intr_cmp_eq},\
	{ "@cmp_neq", intr_cmp_neq},\
	{ "@bitwise_and", intr_bitwise_and},\
	{ "@bitwise_or", intr_bitwise_or},\
	{ "@bitwise_xor", intr_bitwise_xor},\
	{ "@bitwise_shl", intr_bitwise_shl},\
	{ "@bitwise_shr", intr_bitwise_shr},\
	{ "@pow", intr_power},\
	{ "@sqrt", intr_sqrt},\
	{ "@nth_root", intr_nth_root},\
	{ "@bitwise_not", intr_bitwise_not},\
	{ "@bool_not", intr_bool_not},\
	{ "@pointer_to", intr_pointer_to},\
	{ "@deref", intr_dereference},\
	{ "@negate", intr_negate},\
	{ "@inc", intr_add_one},\
	{ "@dec", intr_sub_one},\
	{ "@size_of", intr_size_of},\
	{ "@make_slice", intr_make_slice},\
	{ "@subscript", intr_subscript},\
	

