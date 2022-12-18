"func"   @keyword
"import" @keyword
"type"   @keyword
"return" @keyword
"while"  @keyword
"let"    @keyword
"const"  @keyword
"if"     @keyword
"else"   @keyword
"as"     @keyword
"is"     @keyword
"module" @keyword
"break"  @keyword
"continue" @keyword
"pvt"     @keyword
"struct"  @keyword
"enum"    @keyword


"{" @punctuation.bracket
"}" @punctuation.bracket
"(" @punctuation.bracket
")" @punctuation.bracket
"[" @punctuation.bracket
"]" @punctuation.bracket

"->" @symbol
"@" @symbol
"--" @operator
"-" @operator
"=" @operator
"!=" @operator
"*" @operator
"&" @operator
"&&" @operator
"+" @operator
"++" @operator
"<" @operator
"==" @operator
">" @operator
"||" @operator

"." @delimiter
";" @delimiter
"," @delimeter
":" @delimeter

( literal_int )   @number
( literal_float ) @float
( literal_str )   @string
( literal_bool )  @boolean

(primitive_type) @type.builtin
(id_type) @type

(entity) @identifier
( name ) @variable


( long_name root: (name) @constructor )
( long_name inner : (name) @label)


( comment ) @comment
( multiline_comment ) @comment
(enum_member) @label

(function_signature name : ( name ) @function)
(function_parameter (name) @parameter)

(import_definition (identifier) @label)
(module_definition mod_name: (name) @label)
(struct_type struct_entry_name: (name) @label)
; (module_type_definition type_name: (name) @label)
(module_field_definition field_name : (name) @label)
