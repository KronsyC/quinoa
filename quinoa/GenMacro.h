#define DEFINITIONS_STR \
	std::vector<std::string> alias;\
	std::string type = "";\
	bool ind = false;\
	bool dind = false;\
	int infix = 0;\
	bool postfix = false;\
	bool prefix = false;\
	std::string builtin = "";\
	

#define DEFINITIONS_ARGS \
	\
	, std::vector<std::string> alias\
	, std::string type = ""\
	, bool ind = false\
	, bool dind = false\
	, int infix = 0\
	, bool postfix = false\
	, bool prefix = false\
	, std::string builtin = ""

#define DEFINITIONS_DEFAULT_ASSIGNMENTS \
	this->alias = alias;\
	this->type = type;\
	this->ind = ind;\
	this->dind = dind;\
	this->infix = infix;\
	this->postfix = postfix;\
	this->prefix = prefix;\
	this->builtin = builtin;\
	

#define DEFINITIONS_INITIALIZERS \
	new TokenDefinition(TT_notok, "__notok", {}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_eof, "__eof", {}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_comment, "__comment", {}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_identifier, "__identifier", {}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_literal_str, "__literal_str", {}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_literal_true, "__literal_true", {"true"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_literal_false, "__literal_false", {"false"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_literal_int, "__literal_int", {}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_literal_float, "__literal_float", {}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_literal_char, "__literal_char", {}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_void, "__void", {"void"}, {"void"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_boolean, "__boolean", {"bool"}, {"integer"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_string, "__string", {"c_str"}, {"pointer"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_int8, "__int8", {"i8", "char"}, {"integer"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_int16, "__int16", {"i16"}, {"integer"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_int32, "__int32", {"i32", "int"}, {"integer"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_int64, "__int64", {"i64"}, {"integer"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_uint8, "__uint8", {"u8", "byte"}, {"integer"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_uint16, "__uint16", {"u16"}, {"integer"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_uint32, "__uint32", {"u32", "uint"}, {"integer"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_uint64, "__uint64", {"u64"}, {"integer"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_float16, "__float16", {"f16"}, {"floating_point"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_float32, "__float32", {"f32"}, {"floating_point"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_float64, "__float64", {"f64", "float"}, {"floating_point"}, false, false, 0, false, false, ""),\
	new TokenDefinition(TT_double_quote, "__double_quote", {"\""}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_hashtag, "__hashtag", {"#"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_quote, "__quote", {"'"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_l_paren, "__l_paren", {"("}, "", true, false, 0, false, false, ""),\
	new TokenDefinition(TT_r_paren, "__r_paren", {")"}, "", false, true, 0, false, false, ""),\
	new TokenDefinition(TT_l_brace, "__l_brace", {"{"}, "", true, false, 0, false, false, ""),\
	new TokenDefinition(TT_r_brace, "__r_brace", {"}"}, "", false, true, 0, false, false, ""),\
	new TokenDefinition(TT_l_square_bracket, "__l_square_bracket", {"["}, "", true, false, 0, false, false, ""),\
	new TokenDefinition(TT_r_square_bracket, "__r_square_bracket", {"]"}, "", false, true, 0, false, false, ""),\
	new TokenDefinition(TT_comma, "__comma", {","}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_colon, "__colon", {":"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_double_colon, "__double_colon", {"::"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_semicolon, "__semicolon", {";"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_arrow, "__arrow", {"->"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_question_mark, "__question_mark", {"?"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_at_symbol, "__at_symbol", {"@"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_backslash, "__backslash", {"\\"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_percent, "__percent", {"%"}, "", false, false, 5, false, false, ""),\
	new TokenDefinition(TT_increment, "__increment", {"++"}, "", false, false, 0, true, true, ""),\
	new TokenDefinition(TT_bitwise_not, "__bitwise_not", {"~"}, "", false, false, 0, false, true, ""),\
	new TokenDefinition(TT_decrement, "__decrement", {"--"}, "", false, false, 0, true, true, ""),\
	new TokenDefinition(TT_bang, "__bang", {"!"}, "", false, false, 0, false, true, ""),\
	new TokenDefinition(TT_amperand, "__amperand", {"&"}, "", false, false, 0, false, true, ""),\
	new TokenDefinition(TT_star, "__star", {"*"}, "", false, false, 5, false, true, ""),\
	new TokenDefinition(TT_plus, "__plus", {"+"}, "", false, false, 6, false, false, ""),\
	new TokenDefinition(TT_minus, "__minus", {"-"}, "", false, false, 6, false, true, ""),\
	new TokenDefinition(TT_bool_or, "__bool_or", {"||"}, "", false, false, 15, false, false, ""),\
	new TokenDefinition(TT_bool_and, "__bool_and", {"&&"}, "", false, false, 14, false, false, ""),\
	new TokenDefinition(TT_dot, "__dot", {"."}, "", false, false, 4, false, false, ""),\
	new TokenDefinition(TT_ellipsis, "__ellipsis", {"..."}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_slash, "__slash", {"/"}, "", false, false, 5, false, false, ""),\
	new TokenDefinition(TT_lesser, "__lesser", {"<"}, "", false, false, 9, false, false, ""),\
	new TokenDefinition(TT_greater, "__greater", {">"}, "", false, false, 9, false, false, ""),\
	new TokenDefinition(TT_lesser_eq, "__lesser_eq", {"<="}, "", false, false, 9, false, false, ""),\
	new TokenDefinition(TT_greater_eq, "__greater_eq", {">="}, "", false, false, 9, false, false, ""),\
	new TokenDefinition(TT_assignment, "__assignment", {"="}, "", false, false, 16, false, false, ""),\
	new TokenDefinition(TT_equals, "__equals", {"=="}, "", false, false, 10, false, false, ""),\
	new TokenDefinition(TT_not_equals, "__not_equals", {"!="}, "", false, false, 10, false, false, ""),\
	new TokenDefinition(TT_bitwise_and, "__bitwise_and", {"&"}, "", false, false, 11, false, false, ""),\
	new TokenDefinition(TT_bitiwse_or, "__bitiwse_or", {"|"}, "", false, false, 13, false, false, ""),\
	new TokenDefinition(TT_bitwise_xor, "__bitwise_xor", {"^"}, "", false, false, 13, false, false, ""),\
	new TokenDefinition(TT_bitwise_shl, "__bitwise_shl", {"<<"}, "", false, false, 7, false, false, ""),\
	new TokenDefinition(TT_bitwise_shr, "__bitwise_shr", {">>"}, "", false, false, 7, false, false, ""),\
	new TokenDefinition(TT_if, "__if", {"if"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_else, "__else", {"else"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_while, "__while", {"while"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_for, "__for", {"for"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_break, "__break", {"break"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_continue, "__continue", {"continue"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_switch, "__switch", {"switch"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_case, "__case", {"case"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_fallthrough, "__fallthrough", {"fallthrough"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_import, "__import", {"import"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_module, "__module", {"module"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_seed, "__seed", {"seed"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_func, "__func", {"func"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_is, "__is", {"is"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_as, "__as", {"as"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_return, "__return", {"return"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_let, "__let", {"let"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_const, "__const", {"const"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_public_access, "__public_access", {"pub"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_instance_access, "__instance_access", {"inst"}, "", false, false, 0, false, false, ""),\
	new TokenDefinition(TT_cast, "__cast", {}, "", false, false, 0, false, false, {"cast"}),\
	new TokenDefinition(TT_length, "__length", {}, "", false, false, 0, false, false, {"len"}),\
	

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
	TT_amperand,\
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
	TT_bitiwse_or,\
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
	TT_func,\
	TT_is,\
	TT_as,\
	TT_return,\
	TT_let,\
	TT_const,\
	TT_public_access,\
	TT_instance_access,\
	TT_cast,\
	TT_length,\
	

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
	BIN_bitiwse_or, \
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
	{ TT_bitiwse_or, BIN_bitiwse_or}, \
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
	PRE_amperand, \
	PRE_star, \
	PRE_minus, \
	

#define PREFIX_ENUM_MAPPINGS \
	{ TT_increment, PRE_increment}, \
	{ TT_bitwise_not, PRE_bitwise_not}, \
	{ TT_decrement, PRE_decrement}, \
	{ TT_bang, PRE_bang}, \
	{ TT_amperand, PRE_amperand}, \
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
	{PR_void, "PR_void"},\
	{PR_boolean, "PR_boolean"},\
	{PR_string, "PR_string"},\
	{PR_int8, "PR_int8"},\
	{PR_int16, "PR_int16"},\
	{PR_int32, "PR_int32"},\
	{PR_int64, "PR_int64"},\
	{PR_uint8, "PR_uint8"},\
	{PR_uint16, "PR_uint16"},\
	{PR_uint32, "PR_uint32"},\
	{PR_uint64, "PR_uint64"},\
	{PR_float16, "PR_float16"},\
	{PR_float32, "PR_float32"},\
	{PR_float64, "PR_float64"},\
	

#define PRIMITIVES_ENUM_GROUPS \
	{PR_void, "void"},\
	{PR_boolean, "integer"},\
	{PR_string, "pointer"},\
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
	

