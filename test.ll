; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

define i32 @main(i32 %argc) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %0 = alloca i32, align 4
  store i32 11, i32* %0, align 4
  %1 = load i32, i32* %0, align 4
  %2 = call i32 @"Hello::nice.PR_int32"(i32 %1)
  ret i32 %2
}

define i32 @"Hello::nice.PR_int32"(i32 %offset) {
entry_block:
  %"param offset" = alloca i32, align 4
  store i32 %offset, i32* %"param offset", align 4
  %0 = load i32, i32* %"param offset", align 4
  %1 = add i32 69, %0
  ret i32 %1
}

define i32 @"Hello::nice.PR_int8"(i8 %offset) {
entry_block:
  %"param offset" = alloca i8, align 1
  store i8 %offset, i8* %"param offset", align 1
  %0 = load i8, i8* %"param offset", align 1
  %1 = add i8 69, %0
  %2 = sext i8 %1 to i32
  ret i32 %2
}
