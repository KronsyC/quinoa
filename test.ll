; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

define i32 @main(i32 %argc) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %0 = alloca i32, align 4
  store i32 16, i32* %0, align 4
  %1 = load i32, i32* %0, align 4
  %2 = icmp eq i32 %1, 16
  br i1 %2, label %if_true, label %if_false

if_true:                                          ; preds = %entry_block
  %3 = load i32, i32* %0, align 4
  %4 = load i32, i32* %"param argc", align 4
  %5 = add i32 %3, %4
  store i32 %5, i32* %0, align 4
  br label %if_cont

if_false:                                         ; preds = %entry_block
  br label %if_cont

if_cont:                                          ; preds = %if_false, %if_true
  %6 = load i32, i32* %0, align 4
  ret i32 %6
}
