; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

@str = private unnamed_addr constant [14 x i8] c"Hello, World!\00", align 1

define i32 @"HelloWorld.fn_main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %0 = call i64 @"[fVfwUcNBpn].io.fn_println(PR_int8*)"(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @str, i32 0, i32 0))
  ret i32 0
}

define i64 @"[fVfwUcNBpn].io.fn_println(PR_int8*)"(i8* %message) {
entry_block:
  %"param message" = alloca i8*, align 8
  store i8* %message, i8** %"param message", align 8
  %0 = load i8*, i8** %"param message", align 8
  %1 = ptrtoint i8* %0 to i64
  %2 = load i8*, i8** %"param message", align 8
  %3 = call i64 @"[fVfwUcNBpn].io.fn_strlen(PR_int8*)"(i8* %2)
  %4 = call i64 (i64, i32, ...) @"[MchfJJdUAX].primitives.fn___syscall__(PR_int64,...PR_int64[])"(i64 1, i32 3, i64 1, i64 %1, i64 %3)
  ret i64 0
}

define i64 @"[fVfwUcNBpn].io.fn_strlen(PR_int8*)"(i8* %str) {
entry_block:
  %"param str" = alloca i8*, align 8
  store i8* %str, i8** %"param str", align 8
  %"var i" = alloca i64, align 8
  %0 = load i64, i64* %"var i", align 4
  store i64 0, i64* %"var i", align 4
  br label %while_eval

while_eval:                                       ; preds = %while_exec, %entry_block
  %1 = load i64, i64* %"var i", align 4
  %subscript-ptr = getelementptr i8*, i8** %"param str", i64 %1
  %2 = load i8*, i8** %subscript-ptr, align 8
  %3 = ptrtoint i8* %2 to i8
  %4 = icmp ne i8 %3, 0
  br i1 %4, label %while_exec, label %while_cont

while_exec:                                       ; preds = %while_eval
  %5 = load i64, i64* %"var i", align 4
  %6 = load i64, i64* %"var i", align 4
  %7 = add i64 %6, 1
  %8 = load i64, i64* %"var i", align 4
  %9 = add i64 %8, 1
  store i64 %9, i64* %"var i", align 4
  br label %while_eval

while_cont:                                       ; preds = %while_eval
  %10 = load i64, i64* %"var i", align 4
  ret i64 %10
}

declare i64 @"[MchfJJdUAX].primitives.fn___syscall__(PR_int64,...PR_int64[])"(i64, i32, ...)

define i32 @main(i32 %argc, i8** %argv) {
main_entry:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %0 = load i32, i32* %"param argc", align 4
  %1 = load i8**, i8*** %"param argv", align 8
  %2 = call i32 @"HelloWorld.fn_main(PR_int32,PR_int8**)"(i32 %0, i8** %1)
  ret i32 %2
}
