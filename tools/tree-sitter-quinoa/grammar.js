const type_import = require("./type.js")
const expr_import = require("./expr.js")
const stmn_import = require("./statement.js")
module.exports = grammar({
  name: "quinoa",

  extras: $ => [
    " ",
    "\t",
    "\n",
    "\r",
    $.comment,
  ],

  word: $ => $.name,
  rules: {
    source_file: $ => repeat($._definition),

    comment: $ => choice($.oneline_comment, $.multiline_comment),
    oneline_comment: $ => /\/\/.*\n/,
    multiline_comment: $ => /\/\*(.|\n)*\*\//,


    _definition: $ => choice(
      $.import_definition,
      $.module_definition,
    ),

    import_definition: $ => seq(
      "import",
      optional("@"), // stdlib
      $.identifier,
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
      field("mod_name", $.name),
      optional(
        seq(
          "is",
          $.identifier,
          optional(
            repeat(
              seq(
                ",",
                $.identifier
              )
            )
          )
        )
      ),

      "{",
      optional($.module_content),
      "}"
    ),
    name: $ => /[_a-zA-Z][_a-zA-Z0-9]*/,

    long_name: $ => prec(200, seq(
      field("root", $.name),
      repeat1(
        seq(
          "::",
          field("inner", $.name)
        )
      )

    )),

    identifier: $ => choice($.name, $.long_name),
    // TYPES
    ...type_import,

    // EXPRESSIONS
    ...expr_import,

    // STATEMENTS
    ...stmn_import,

    module_member: $ => seq(
      repeat($.metadata),
      optional("pvt"),
      choice(
        $.module_field_definition,
        $.module_type_definition,
        $.module_function_definition

      )
    ),

    module_content: $ => seq(
      $.module_member,
      repeat($.module_member)
    ),


    metadata: $ => seq(
      "#[",
      $.name,
      repeat(
        $._literal
      ),
      "]"
    ),

    module_type_definition: $ => seq(
      "type",
      field("type_name", $.name),
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
    module_field_definition: $ => seq(
      optional("const"),
      field("field_name", $.name),
      ":",
      $.type,
      "=",
      $.expr,
      ";"
    ),

    function_parameter: $ => seq(
      $.name,
      ":",
      $.type
    ),

    function_signature: $ => seq(
      "func",
      field("name", $.name),
      optional(
        seq(
          "<",
          optional(
            seq(
              $.id_type,
              repeat(
                seq(
                  ",",
                  $.id_type,)
              )
            )),
          ">",)
      ),
      optional(
        seq(
          ".",
          $.type,
        )
      ),
      "(",
      optional(
        seq(
          field("parameter", $.function_parameter),
          repeat(
            seq(
              ",",
              field("parameter", $.function_parameter)
            ))
        )
      ),
      ")",
      field("return_type", optional(
        seq(
          "->",
          $.type,
        )
      ))
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
