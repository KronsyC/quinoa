; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

@0 = private unnamed_addr constant [14 x i8] c"Hello, World!\00", align 1
@"IO::stdout" = linkonce global i32 1

define i32 @main(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %0 = call i64 @"IO::println.PR_int8*"(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @0, i32 0, i32 0))
  ret i32 0
}

define void @"IO::putc.PR_int8"(i8 %ch) {
entry_block:
  %"param ch" = alloca i8, align 1
  store i8 %ch, i8* %"param ch", align 1
  %0 = load i32, i32* @"IO::stdout", align 4
  %1 = ptrtoint i8* %"param ch" to i64
  %2 = load i32, i32* @"IO::stdout", align 4
  %3 = sext i32 %2 to i64
  %4 = call i64 @write(i32 %0, i64 %1, i64 %3)
  ret void
}

declare i64 @write(i32, i64, i64)

define i64 @"IO::print.PR_int8*"(i8* %message) {
entry_block:
  %"param message" = alloca i8*, align 8
  store i8* %message, i8** %"param message", align 8
  %0 = alloca i64, align 8
  %1 = load i32, i32* @"IO::stdout", align 4
  %2 = load i8*, i8** %"param message", align 8
  %3 = ptrtoint i8* %2 to i64
  %4 = load i8*, i8** %"param message", align 8
  %5 = call i64 @"CString::len.PR_int8*"(i8* %4)
  %6 = call i64 @write(i32 %1, i64 %3, i64 %5)
  store i64 %6, i64* %0, align 4
  %7 = load i64, i64* %0, align 4
  ret i64 %7
}

define i64 @"IO::println.PR_int8*"(i8* %message) {
entry_block:
  %"param message" = alloca i8*, align 8
  store i8* %message, i8** %"param message", align 8
  %0 = alloca i64, align 8
  %1 = load i8*, i8** %"param message", align 8
  %2 = call i64 @"IO::print.PR_int8*"(i8* %1)
  store i64 %2, i64* %0, align 4
  call void @"IO::putc.PR_int8"(i8 10)
  %3 = load i64, i64* %0, align 4
  %4 = add i64 %3, 1
  ret i64 %4
}

define i64 @"CString::len.PR_int8*"(i8* %str) {
entry_block:
  %"param str" = alloca i8*, align 8
  store i8* %str, i8** %"param str", align 8
  %0 = alloca i64, align 8
  store i64 0, i64* %0, align 4
  br label %while_eval

while_eval:                                       ; preds = %while_exec, %entry_block
  %1 = load i8*, i8** %"param str", align 8
  %2 = load i64, i64* %0, align 4
  %3 = getelementptr i8, i8* %1, i64 %2
  %4 = load i8, i8* %3, align 1
  %5 = icmp ne i8 %4, 0
  br i1 %5, label %while_exec, label %while_cont

while_exec:                                       ; preds = %while_eval
  %6 = load i64, i64* %0, align 4
  %7 = add i64 %6, 1
  %8 = load i64, i64* %0, align 4
  store i64 %7, i64* %0, align 4
  br label %while_eval

while_cont:                                       ; preds = %while_eval
  %9 = load i64, i64* %0, align 4
  ret i64 %9
}
