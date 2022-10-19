; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

@str = private unnamed_addr constant [33 x i8] c"Error: Equality Assertion Failed\00", align 1

define i32 @"Test::fn_main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  call void @"[L54Z6GSQTa].assertions::fn_equals(PR_int32,PR_int32)"(i32 1, i32 2)
  ret i32 0
}

define void @"[L54Z6GSQTa].assertions::fn_equals(PR_int32,PR_int32)"(i32 %v1, i32 %v2) {
entry_block:
  %"param v1" = alloca i32, align 4
  store i32 %v1, i32* %"param v1", align 4
  %"param v2" = alloca i32, align 4
  store i32 %v2, i32* %"param v2", align 4
  %0 = load i32, i32* %"param v1", align 4
  %1 = load i32, i32* %"param v2", align 4
  %2 = icmp ne i32 %0, %1
  br i1 %2, label %if_exec, label %if_cont

if_exec:                                          ; preds = %entry_block
  %3 = call i64 @"[FcDkJv5MnT].io::fn_println(PR_int8*)"(i8* getelementptr inbounds ([33 x i8], [33 x i8]* @str, i32 0, i32 0))
  br label %if_cont

if_cont:                                          ; preds = %if_exec, %entry_block
  ret void
}

define i64 @"[FcDkJv5MnT].io::fn_println(PR_int8*)"(i8* %message) {
entry_block:
  %"param message" = alloca i8*, align 8
  store i8* %message, i8** %"param message", align 8
  %"var len" = alloca i64, align 8
  %0 = load i8*, i8** %"param message", align 8
  %1 = ptrtoint i8* %0 to i64
  %2 = load i8*, i8** %"param message", align 8
  %3 = call i64 @"[FcDkJv5MnT].io::fn_strlen(PR_int8*)"(i8* %2)
  %4 = call i64 @write(i64 1, i64 %1, i64 %3)
  store i64 %4, i64* %"var len", align 4
  call void @"[FcDkJv5MnT].io::fn_putc(PR_int8)"(i8 10)
  %5 = load i64, i64* %"var len", align 4
  %6 = add i64 %5, 1
  ret i64 %6
}

define i64 @"[FcDkJv5MnT].io::fn_strlen(PR_int8*)"(i8* %str) {
entry_block:
  %"param str" = alloca i8*, align 8
  store i8* %str, i8** %"param str", align 8
  %"var i" = alloca i64, align 8
  store i64 0, i64* %"var i", align 4
  br label %while_eval

while_eval:                                       ; preds = %while_exec, %entry_block
  %0 = load i64, i64* %"var i", align 4
  %1 = load i8*, i8** %"param str", align 8
  %2 = getelementptr i8, i8* %1, i64 %0
  %3 = load i8, i8* %2, align 1
  %4 = icmp ne i8 %3, 0
  br i1 %4, label %while_exec, label %while_cont

while_exec:                                       ; preds = %while_eval
  %5 = load i64, i64* %"var i", align 4
  %6 = add i64 %5, 1
  store i64 %6, i64* %"var i", align 4
  br label %while_eval

while_cont:                                       ; preds = %while_eval
  %7 = load i64, i64* %"var i", align 4
  ret i64 %7
}

declare i64 @write(i64, i64, i64)

define void @"[FcDkJv5MnT].io::fn_putc(PR_int8)"(i8 %ch) {
entry_block:
  %"param ch" = alloca i8, align 1
  store i8 %ch, i8* %"param ch", align 1
  %0 = ptrtoint i8* %"param ch" to i64
  %1 = call i64 @write(i64 1, i64 %0, i64 1)
  ret void
}

define i64 @"[FcDkJv5MnT].io::fn_read(PR_int8*,PR_int64)"(i8* %str, i64 %len) {
entry_block:
  %"param str" = alloca i8*, align 8
  store i8* %str, i8** %"param str", align 8
  %"param len" = alloca i64, align 8
  store i64 %len, i64* %"param len", align 4
  %0 = load i8*, i8** %"param str", align 8
  %1 = ptrtoint i8* %0 to i64
  %2 = load i64, i64* %"param len", align 4
  %3 = call i64 @"[INlk6XdK7Y].syscall::fn_read(PR_int64,PR_int64,PR_int64)"(i64 0, i64 %1, i64 %2)
  ret i64 %3
}

define i64 @"[INlk6XdK7Y].syscall::fn_read(PR_int64,PR_int64,PR_int64)"(i64 %fd, i64 %ptr, i64 %len) {
entry_block:
  %"param fd" = alloca i64, align 8
  store i64 %fd, i64* %"param fd", align 4
  %"param ptr" = alloca i64, align 8
  store i64 %ptr, i64* %"param ptr", align 4
  %"param len" = alloca i64, align 8
  store i64 %len, i64* %"param len", align 4
  %0 = load i64, i64* %"param fd", align 4
  %1 = load i64, i64* %"param ptr", align 4
  %2 = load i64, i64* %"param len", align 4
  %3 = call i64 (i64, ...) @syscall(i64 0, i64 %0, i64 %1, i64 %2)
  ret i64 %3
}

declare i64 @syscall(i64, ...)

define i32 @main(i32 %argc, i8** %argv) {
main_entry:
  %0 = call i32 @"Test::fn_main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv)
  ret i32 %0
}
