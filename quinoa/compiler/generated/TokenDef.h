
#pragma once

#include<string>
#include<vector>

enum TokenType{
    
		TT_eof,
		TT_comment,
		TT_identifier,
		TT_void,
		TT_string,
		TT_int8,
		TT_int16,
		TT_int32,
		TT_int64,
		TT_uint8,
		TT_uint16,
		TT_uint32,
		TT_uint64,
		TT_float16,
		TT_float32,
		TT_float64,
		TT_double_quote,
		TT_hashtag,
		TT_quote,
		TT_l_paren,
		TT_r_paren,
		TT_l_brace,
		TT_r_brace,
		TT_l_square_bracket,
		TT_r_square_bracket,
		TT_comma,
		TT_colon,
		TT_semicolon,
		TT_question_mark,
		TT_at_symbol,
		TT_backslash,
		TT_percent,
		TT_postfix_inc,
		TT_postfix_dec,
		TT_subscript,
		TT_prefix_inc,
		TT_prefix_dec,
		TT_instantiate_object,
		TT_bang,
		TT_star,
		TT_plus,
		TT_minus,
		TT_bool_or,
		TT_bool_and,
		TT_dot,
		TT_slash,
		TT_lesser,
		TT_greater,
		TT_lesser_eq,
		TT_greater_eq,
		TT_assignment,
		TT_equals,
		TT_not_equals,
		TT_bitwise_and,
		TT_bitiwse_or,
		TT_bitwise_not,
		TT_bitwise_xor,
		TT_bitwise_shl,
		TT_bitwise_shr,
		TT_underscore,
};

class TokenDefinition{
    TokenType type;
    
	std::vector<std::string> alias;
	bool type = false;
	bool ind = false;
	bool dind = false;
	int infix = 0;
	int postfix = 0;
	int prefix = 0;
};
