const type_import = require("./type.js")
const expr_import = require("./expr.js")
const stmn_import = require("./statement.js")
module.exports = grammar({
  name : "Quinoa",

  extras : $ => [
    " ",
    "\t",
    "\n",
    "\r",
    /\/\/.*\n/
  ],

  word : $ => $.name,
  rules : {
    source_file : $ => repeat($._definition),

    _definition : $ => choice(
      $.import_definition,
      $.module_definition,
    ),

    import_definition : $ => seq(
      "import",
      optional( "@" ), // stdlib
      $.long_name,
      optional(
        seq(
          "as",
          $.name
        )
      ),
      ";"
    ),

    module_definition: $ => seq(
      "module",
      $.name,
      optional(
        seq(
          "is",
          $.long_name,
          optional(
            repeat(
              seq(
                ",",
                $.long_name
              )
            )
          )
        )
      ),

      "{",
      optional( $.module_content ),
      "}"
    ),
    name : $ => /[_a-zA-Z][_a-zA-Z0-9]{0,30}/,
    
    long_name: $ => prec( 200, seq(
      $.name,
        repeat(
          seq(
            "::",
            $.name
          )
        )

    )),


    // TYPES
    ...type_import,

    // EXPRESSIONS
    ...expr_import,

    // STATEMENTS
    ...stmn_import,

    module_member : $ => choice(
      $.module_field_definiton,
      $.module_type_definition,
      $.module_function_definition
    ),

    module_content : $ => seq(
      $.module_member,
      repeat( $.module_member )
    ),


    module_type_definition: $ => seq(
      "type",
      $.name,
      optional(
        seq(
          "<",
          $.type,
          repeat(
            seq(
            ",",
            $.type
            )
          ),
          ">"
        )
      ),
      "=",
      $.type,
      ";"
    ),
    module_field_definiton: $ => seq(
      optional("const"),
      $.name,
      ":",
      $.type,
      "=",
      $.expr,
      ";"
    ),

    function_parameter : $ => seq(
      $.name,
      ":",
      $.type
    ),

    function_signature: $ => seq(
      "func",
      $.name,
      optional(
        seq(
          ".",
          $.type,
        )
      ),
      "(",
      optional(
        seq(
          $.function_parameter,
          repeat(
            seq(
            ",",
            $.function_parameter
          ))
        )
      ),
      ")",
      optional(
        seq(
          "->",
          $.type,
        )
      )
    ),

    module_function_definition: $ => choice(
      seq($.function_signature, ";"),
      seq(
        $.function_signature,
        "{",
        repeat($.statement),
        "}"
      )
    )


  }
})
