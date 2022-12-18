module.exports = {

  statement: $ => choice(
    seq($.expr, ";"),
    seq($.return, ";"),
    seq($.declaration, ";"),
    $.if_loop,
    $.while_loop,
    $.control_flow

  ),

  control_flow: $ => choice("break", "continue"),
  return: $ => seq(
    "return",
    optional($.expr),
  ),

  declaration: $ => seq(
    choice("let", "const"),
    $.name,
    optional(
      seq(
        ":",
        $.type,
      )
    ),
    optional(
      seq(
        "=",
        $.expr
      )
    )
  ),

  if_loop: $ => seq(
    "if",
    "(",
    $.expr,
    ")",
    "{",
    repeat($.statement),
    "}",
    optional(
      seq(
        "else",
        "{",
        repeat($.statement),
        "}"
      )
    )
  ),
  while_loop: $ => seq(
    "while",
    "(",
    $.expr,
    ")",
    "{",
    repeat($.statement),
    "}"
  )

}
