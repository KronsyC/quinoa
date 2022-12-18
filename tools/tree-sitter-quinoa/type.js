module.exports = {
  primitive_type: $ => choice(
    "int", "float", "byte", "char", "str",
    "i8", "i16", "i32", "i64",
    "u8", "u16", "u32", "u64",
    "f16", "f32", "f64",
    "bool",
  ),

  parameterized_type_ref: $ => prec(10, seq(
    $.id_type,
    "<",
    $.type,
    repeat(
      seq(
        ",",
        $.type
      )
    ),
    ">",
  )),
  struct_type: $ => seq(
    "struct",
    "{",
    repeat(
      seq(
        field("struct_entry_name", $.name),
        ":",
        $.type,
        ";",
      )
    ),
    "}"
  ),
  enum_member: $ => seq(
    $.name,
    ";"
  ),
  enum_type: $ => seq(
    "enum",
    "{",
    repeat(
      $.enum_member
    ),
    "}"
  ),
  id_type: $ => $.identifier,
  modified_type: $ => choice(
    seq("*", $.type),
    seq("&", $.type),
    seq("[", "]", $.type),
    seq("[", $._literal, "]", $.type),
  ),


  _basic_type: $ => choice(
    $.primitive_type,
    $.id_type,
    $.struct_type,
    $.enum_type,
    $.modified_type,
  ),
  type: $ => choice(
    $._basic_type,
    $.parameterized_type_ref
  )
}
