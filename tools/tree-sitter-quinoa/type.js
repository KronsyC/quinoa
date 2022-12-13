module.exports = {
    primitive_type: $ => choice(
      "int", "float", "byte", "char", "str",
      "i8", "i16", "i32", "i64",
      "u8", "u16", "u32", "u64",
      "f16", "f32", "f64",
      "bool",
    ),

    parameterized_type_ref: $ => seq(
      $.long_name,
      "<",
      $.type,
      repeat(
        seq(
          ",",
          $.type
        )
      ),
      ">",
    ),
    struct_type: $ => seq(
      "struct",
      "{",
      repeat(
        seq(
          $.name,
          ":",
          $.type,
          ";",
        )
      ),
      "}"
    ),
    enum_type : $ => seq(
      "enum",
      "{",
      repeat(
        seq(
          $.name,
          ";"
        )
      ),
      "}"
    ),
    id_type: $ => $.name,
    modified_type : $ => choice(
      seq( $.type, "*" ),
      seq( $.type, "&" ),
      seq( $.type, "[", "]" ),
      seq( $.type, "[", $.expr, "]" ),
    ),

    
    _basic_type: $ => choice(
      $.primitive_type,
      $.long_name,
      $.struct_type,
      $.enum_type,
      $.id_type,
      $.modified_type,
    ),
    type : $ => choice(
      $._basic_type,
      $.parameterized_type_ref
    )
}
