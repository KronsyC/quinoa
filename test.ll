; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

@str = private unnamed_addr constant [19 x i8] c"\0AAssertion Error: \00", align 1
@str.1 = private unnamed_addr constant [26 x i8] c"Equality Assertion Failed\00", align 1

define i32 @"fn_Test::main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %0 = load i32, i32* %"param argc", align 4
  call void @"fn_$azftmosOiG::Assert::equals(PR_int32,PR_int32)"(i32 %0, i32 2)
  call void @"fn_$azftmosOiG::Assert::truthy(PR_int8)"(i8 1)
  ret i32 0
}

define i64 @"fn_$Y3MIh36uGD::IO::println(PR_int8*)"(i8* %message) {
entry_block:
  %"param message" = alloca i8*, align 8
  store i8* %message, i8** %"param message", align 8
  %"var len" = alloca i64, align 8
  %0 = load i8*, i8** %"param message", align 8
  %1 = call i64 @"fn_$Y3MIh36uGD::IO::print(PR_int8*)"(i8* %0)
  store i64 %1, i64* %"var len", align 4
  call void @"fn_$Y3MIh36uGD::IO::putc(PR_int8)"(i8 10)
  %2 = load i64, i64* %"var len", align 4
  %3 = add i64 %2, 1
  ret i64 %3
}

define i64 @"fn_$Y3MIh36uGD::IO::print(PR_int8*)"(i8* %message) {
entry_block:
  %"param message" = alloca i8*, align 8
  store i8* %message, i8** %"param message", align 8
  %"var len" = alloca i64, align 8
  %0 = load i8*, i8** %"param message", align 8
  %1 = ptrtoint i8* %0 to i64
  %2 = load i8*, i8** %"param message", align 8
  %3 = call i64 @"fn_$Y3MIh36uGD::IO::strlen(PR_int8*)"(i8* %2)
  %4 = call i64 @write(i32 1, i64 %1, i64 %3)
  store i64 %4, i64* %"var len", align 4
  %5 = load i64, i64* %"var len", align 4
  ret i64 %5
}

define void @"fn_$Y3MIh36uGD::IO::putc(PR_int8)"(i8 %ch) {
entry_block:
  %"param ch" = alloca i8, align 1
  store i8 %ch, i8* %"param ch", align 1
  %0 = ptrtoint i8* %"param ch" to i64
  %1 = call i64 @write(i32 1, i64 %0, i64 1)
  ret void
}

declare i64 @write(i32, i64, i64)

define i64 @"fn_$Y3MIh36uGD::IO::strlen(PR_int8*)"(i8* %str) {
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

define i64 @"fn_$Y3MIh36uGD::IO::read(PR_int8*,PR_int64)"(i8* %str, i64 %len) {
entry_block:
  %"param str" = alloca i8*, align 8
  store i8* %str, i8** %"param str", align 8
  %"param len" = alloca i64, align 8
  store i64 %len, i64* %"param len", align 4
  %0 = load i8*, i8** %"param str", align 8
  %1 = ptrtoint i8* %0 to i64
  %2 = load i64, i64* %"param len", align 4
  %3 = call i64 @read(i64 0, i64 %1, i64 %2)
  ret i64 %3
}

declare i64 @read(i64, i64, i64)

define void @"fn_$azftmosOiG::Assert::truthy(PR_int8)"(i8 %value) {
entry_block:
  %"param value" = alloca i8, align 1
  store i8 %value, i8* %"param value", align 1
  %0 = call i64 @exit(i64 0)
  ret void
}

declare i64 @exit(i64)

define void @"fn_$azftmosOiG::Assert::equals(PR_int32,PR_int32)"(i32 %v1, i32 %v2) {
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
  call void @"fn_$azftmosOiG::Assert::assert_error(PR_int8*)"(i8* getelementptr inbounds ([26 x i8], [26 x i8]* @str.1, i32 0, i32 0))
  br label %if_cont

if_cont:                                          ; preds = %if_exec, %entry_block
  ret void
}

define void @"fn_$azftmosOiG::Assert::assert_error(PR_int8*)"(i8* %message) {
entry_block:
  %"param message" = alloca i8*, align 8
  store i8* %message, i8** %"param message", align 8
  %0 = call i64 @"fn_$Y3MIh36uGD::IO::print(PR_int8*)"(i8* getelementptr inbounds ([19 x i8], [19 x i8]* @str, i32 0, i32 0))
  %1 = load i8*, i8** %"param message", align 8
  %2 = call i64 @"fn_$Y3MIh36uGD::IO::println(PR_int8*)"(i8* %1)
  %3 = call i64 @exit(i64 21)
  ret void
}

define i32 @main(i32 %argc, i8** %argv) {
main_entry:
  %0 = call i32 @"fn_Test::main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv)
  ret i32 %0
}
