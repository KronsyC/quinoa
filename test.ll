; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

define i32 @"Test::fn_main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %"var smt" = alloca i8, align 1
  store i8 1, i8* %"var smt", align 1
  %0 = load i8, i8* %"var smt", align 1
  %1 = sext i8 %0 to i32
  ret i32 %1
}

define i32 @main(i32 %argc, i8** %argv) {
main_entry:
  %0 = call i32 @"Test::fn_main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv)
  ret i32 %0
}
