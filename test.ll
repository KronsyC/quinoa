; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

define i32 @main(i32 %argc) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %0 = alloca i32, align 4
  store i32 16, i32* %0, align 4
  %1 = load i32, i32* %0, align 4
  ret i32 %1
}
