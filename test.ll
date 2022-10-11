; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

define i32 @"HelloWorld.fn_main(PR_int32,PR_string*)"(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %"var i" = alloca i32, align 4
  %"var num" = alloca i32, align 4
  %0 = load i32, i32* %"var num", align 4
  store i32 0, i32* %"var num", align 4
  %1 = load i32, i32* %"var i", align 4
  store i32 0, i32* %"var i", align 4
  br label %for_eval

for_eval:                                         ; preds = %for_inc, %entry_block
  %2 = load i32, i32* %"var i", align 4
  %3 = icmp slt i32 %2, 100
  br i1 %3, label %for_exec, label %while_cont

for_inc:                                          ; preds = %for_exec
  %4 = load i32, i32* %"var i", align 4
  %5 = load i32, i32* %"var i", align 4
  %6 = add i32 %5, 1
  %7 = load i32, i32* %"var i", align 4
  %8 = add i32 %7, 1
  store i32 %8, i32* %"var i", align 4
  br label %for_eval

for_exec:                                         ; preds = %for_eval
  %9 = load i32, i32* %"var num", align 4
  %10 = load i32, i32* %"var num", align 4
  %11 = add i32 %10, 1
  %12 = load i32, i32* %"var num", align 4
  %13 = add i32 %12, 1
  store i32 %13, i32* %"var num", align 4
  br label %for_inc

while_cont:                                       ; preds = %for_eval
  %14 = load i32, i32* %"var num", align 4
  ret i32 %14
}

define i64 @"[Sd9eUfbAD4].io.fn_println(PR_string)"(i8* %message) {
entry_block:
  %"param message" = alloca i8*, align 8
  store i8* %message, i8** %"param message", align 8
  ret i64 69
}

define i32 @main(i32 %argc, i8** %argv) {
main_entry:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %0 = load i32, i32* %"param argc", align 4
  %1 = load i8**, i8*** %"param argv", align 8
  %2 = call i32 @"HelloWorld.fn_main(PR_int32,PR_string*)"(i32 %0, i8** %1)
  ret i32 %2
}
