; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

define i32 @main(i32 %argc) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %0 = alloca i64, align 8
  %1 = load i32, i32* %"param argc", align 4
  %2 = add i32 11, %1
  %3 = sext i32 %2 to i64
  store i64 %3, i64* %0, align 4
  store i64 13, i64* %0, align 4
  %4 = load i64, i64* %0, align 4
  %5 = trunc i64 %4 to i32
  ret i32 %5
}
