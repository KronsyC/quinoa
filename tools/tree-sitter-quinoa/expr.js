module.exports = {

  expr : $ => choice(
    seq( "(" , $.expr, ")" ),
    $.long_name,
    $._literal,
    $._unary_operation,
    $._binary_operation,
    $.method_call,
    $.instance_method_call,
  ),

  _number : $ => /[0-9]/,
  _char   : $ => choice(
    /[A-Za-z]/,
    "\\\"" // Prevent early escape from strings
  ),
  _literal: $ => choice(
    $.literal_int,
    $.literal_float,
    $.literal_bool,
    $.literal_str,
  ),

  literal_int: $ => seq($._number, repeat($._number)),
  literal_float: $ => prec(100, seq(
    $._number,
    repeat($._number),
    ".",
    $._number,
    repeat($._number)
  )),
  literal_bool: $ => choice(
    "true", "false"
  ),
  literal_str: $ => /\"(\\.|[^"\\])*\"/,


 _prefix_operation : $ => prec(1, choice(
    seq("++", $.expr),
    seq("--", $.expr),
    seq("!",  $.expr),
    seq("~",  $.expr),
    seq("&",  $.expr),
    seq("*",  $.expr),
  )),
  _postfix_operation : $ => prec(2, choice(
    seq($.expr, "++"),
    seq($.expr, "--"),
  )),
  _unary_operation : $ => choice(
    $._prefix_operation,
    $._postfix_operation
  ),

  _binary_operation : $ => choice(
   BINRULE($, "+", 6),
   BINRULE($, "-", 6),
   BINRULE($, "*", 5),
   BINRULE($, "/", 5),
   BINRULE($, "%", 5),
   prec.left(2, seq($.expr, ".", $.name)),
   BINRULE($, "&&", 14),
   BINRULE($, "||", 15),
   BINRULE($, "&", 11),
   BINRULE($, "|", 13),
   BINRULE($, "^", 12),
   BINRULE($, "<<", 7),
   BINRULE($, ">>", 7),
  ),

  method_parameters: $ => seq(
    "(",
    optional(
      seq(
        $.expr,
        repeat(
          seq(
            ",",
            $.expr
          )
        )
      )
    ),
    ")",
  ),
  
  generic_arguments : $ => seq(
    "::<",
    optional(
      seq(
        $.type,
        repeat(
          seq(
            ",",
            $.type,
          )
        )
      )
    ),
    ">"
  ),

  method_call : $ => seq(
    $.long_name,
    optional( $.generic_arguments ),
    $.method_parameters,
  ),
  instance_method_call : $ => prec(100, seq(
    $.expr,
    ".",
    $.name,
    optional( $.generic_arguments ),
    $.method_parameters,
  )),
  struct_initialization : $ => seq(
    $._basic_type,
    optional( $.generic_arguments ),
    "{",
    repeat(
      seq(
        $.name,
        ":",
        $.expr,
        ";",
      )
    ),
    "}"
  ) 
}


function BINRULE($, symbol, precedence){
  return prec.left(precedence, seq($.expr, symbol, $.expr))
}

