; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

define i32 @"Test.fn_main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %"list list" = alloca i64, i32 10, align 8
  %"var list" = alloca i64*, align 8
  store i64* %"list list", i64** %"var list", align 8
  %0 = load i64*, i64** %"var list", align 8
  %subscript-ptr = getelementptr i64, i64* %0, i8 0
  store i64 1, i64* %subscript-ptr, align 4
  %1 = load i64*, i64** %"var list", align 8
  %subscript-ptr1 = getelementptr i64, i64* %1, i8 0
  %2 = load i64, i64* %subscript-ptr1, align 4
  %3 = trunc i64 %2 to i32
  ret i32 %3
}

define i32 @main(i32 %argc, i8** %argv) {
main_entry:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %0 = load i32, i32* %"param argc", align 4
  %1 = load i8**, i8*** %"param argv", align 8
  %2 = call i32 @"Test.fn_main(PR_int32,PR_int8**)"(i32 %0, i8** %1)
  ret i32 %2
}
