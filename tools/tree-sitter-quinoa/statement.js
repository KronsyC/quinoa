module.exports = {

  statement: $ => choice(
    seq($.expr, ";"),
    seq($.return, ";"),
    seq($.declaration, ";"),
    $.if_loop,
    $.while_loop
    
  ),


  return : $ => seq(
    "return",
    optional( $.expr ),
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

  if_loop : $ => seq(
    "if",
    "(",
    $.expr,
    ")",
    "{",
    repeat($.statement),
    "}"
  ),
  while_loop : $ => seq(
    "while",
    "(",
    $.expr,
    ")",
    "{",
    repeat($.statement),
    "}"
  )

}
