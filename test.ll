; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

define i32 @main(i32 %argc) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %0 = alloca i32, align 4
  store i32 0, i32* %0, align 4
  br label %while_eval

while_eval:                                       ; preds = %while_exec, %entry_block
  %1 = load i32, i32* %0, align 4
  %2 = icmp slt i32 %1, 10
  br i1 %2, label %while_exec, label %while_cont

while_exec:                                       ; preds = %while_eval
  %3 = load i32, i32* %0, align 4
  %4 = add i32 %3, 1
  store i32 %4, i32* %0, align 4
  br label %while_eval

while_cont:                                       ; preds = %while_eval
  %5 = load i32, i32* %0, align 4
  ret i32 %5
}
