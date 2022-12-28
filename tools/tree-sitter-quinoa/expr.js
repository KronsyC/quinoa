module.exports = {

  expr: $ => choice(
    $._literal,
    $._unary_operation,
    $._binary_operation,
    $.method_call,
    $.struct_initialization,
    $.cast,
    $.entity,
    $.array_initialization,
  ),



  _literal: $ => choice(
    $.literal_int,
    $.literal_float,
    $.literal_bool,
    $.literal_str,
  ),

  literal_int: $ => /[0-9]+/,
  literal_float: $ => prec(100, seq(
    /[0-9]+/,
    token.immediate("."),
    token.immediate(/[0-9]+/)
  )),
  literal_bool: $ => choice(
    "true", "false"
  ),
  literal_str: $ => /\"(\\.|[^"\\])*\"/,

  entity: $ => choice(
    seq("(", $.expr, ")"),
    seq($.entity, ".", $.name),
    $.identifier,
    seq($.entity, "[", $.expr, "]"),
  ),

  _prefix_operation: $ => prec(1, choice(
    seq("++", $.expr),
    seq("--", $.expr),
    seq("!", $.expr),
    seq("~", $.expr),
    seq("&", $.expr),
    seq("*", $.expr),
  )),
  _postfix_operation: $ => prec(2, choice(
    seq($.expr, "++"),
    seq($.expr, "--"),
  )),
  _unary_operation: $ => choice(
    $._prefix_operation,
    $._postfix_operation
  ),

  _binary_operation: $ => choice(
    BINRULE($, "+", 6),
    BINRULE($, "-", 6),
    BINRULE($, "*", 5),
    BINRULE($, "/", 5),
    BINRULE($, "%", 5),
    BINRULE($, "&&", 14),
    BINRULE($, "||", 15),
    BINRULE($, "&", 11),
    BINRULE($, "|", 13),
    BINRULE($, "=", 16),
    BINRULE($, "==", 10),
    BINRULE($, "!=", 10),
    BINRULE($, "^", 12),
    BINRULE($, "<<", 7),
    BINRULE($, ">>", 7),
    BINRULE($, ">=", 9),
    BINRULE($, ">", 9),
    BINRULE($, "<=", 9),
    BINRULE($, "<", 9),

  ),

  method_parameters: $ => seq(
    $.expr,
    repeat(
      seq(
        ",",
        $.expr
      )
    )


  ),

  generic_arguments: $ => seq(
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

  method_call: $ => seq(
    field("function_name", $.entity),
    field("generic_args", optional($.generic_arguments)),
    "(",
    field("arguments", optional($.method_parameters)),
    ")"
  ),
  struct_initialization: $ => prec(10, seq(
    field("target", $.identifier),
    field("generic_args", optional($.generic_arguments)),
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
  )),
  array_initialization: $ => seq(
    "[",
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
    "]"
  ),
  cast: $ => seq(
    $.expr,
    "as",
    $.type
  ),
  intrinsic: $ => seq(
    "@",
    $.name,
    optional($.generic_arguments),
    "(",
    optional($.method_parameters),
    ")"
  ),

}


function BINRULE($, symbol, precedence) {
  return prec.left(precedence, seq($.expr, symbol, $.expr))
}

