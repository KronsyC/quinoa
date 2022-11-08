; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

define i32 @main(i32 %argc) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %0 = call i32 @"Hello::nice.PR_int32"(i32 11)
  ret i32 %0
}

define i32 @"Hello::nice.PR_int32"(i32 %offset) {
entry_block:
  %"param offset" = alloca i32, align 4
  store i32 %offset, i32* %"param offset", align 4
  %0 = load i32, i32* %"param offset", align 4
  %1 = add i32 69, %0
  ret i32 %1
}
