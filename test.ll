; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

define i32 @main(i32 %argc) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %0 = call i32 @"Util::nice"()
  ret i32 %0
}

define i32 @"Util::nice"() {
entry_block:
  ret i32 69
}
