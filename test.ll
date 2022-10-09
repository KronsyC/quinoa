; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

@str = private unnamed_addr constant [3 x i8] c"Hi\00", align 1

define i32 @"HelloWorld.fn_main(PR_int32,PR_string*)"(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %"var message" = alloca i8*, align 8
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @str, i32 0, i32 0), i8** %"var message", align 8
  %"loaded var 'message'" = load i8*, i8** %"var message", align 8
  %0 = ptrtoint i8* %"loaded var 'message'" to i64
  %1 = call i64 (i64, i32, ...) @"[VbIDx9BknR].primitives.fn___syscall__(PR_int64,...PR_int64[])"(i64 2, i32 3, i64 1, i64 %0, i64 2)
  ret i32 0
}

define i64 @"[t8YWQlndZS].io.fn_println(PR_string)"(i8* %message) {
entry_block:
  %"param message" = alloca i8*, align 8
  store i8* %message, i8** %"param message", align 8
  ret i64 69
}

define i64 @"[VbIDx9BknR].primitives.fn___syscall__(PR_int64,...PR_int64[])"(i64 %callNo, i32 %"+vararg_count", ...) {
entry_block:
  %"param callNo" = alloca i64, align 8
  store i64 %callNo, i64* %"param callNo", align 4
  %"param +vararg_count" = alloca i32, align 4
  store i32 %"+vararg_count", i32* %"param +vararg_count", align 4
  %0 = call i64 (i64, ...) @syscall()
  ret i64 11
}

declare i64 @syscall(i64, ...)

define i32 @main(i32 %argc, i8** %argv) {
main_entry:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %"loaded var 'argc'" = load i32, i32* %"param argc", align 4
  %"loaded var 'argv'" = load i8**, i8*** %"param argv", align 8
  %0 = call i32 @"HelloWorld.fn_main(PR_int32,PR_string*)"(i32 %"loaded var 'argc'", i8** %"loaded var 'argv'")
  ret i32 %0
}
