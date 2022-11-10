; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

define i32 @main(i32 %argc) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %0 = load i32, i32* %"param argc", align 4
  %1 = icmp eq i32 %0, 1
  br i1 %1, label %if_true, label %if_false

if_true:                                          ; preds = %entry_block
  ret i32 100

if_false:                                         ; preds = %entry_block
  ret i32 69
}
