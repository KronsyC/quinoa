; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

define i32 @"HelloWorld.fn_main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %"var c" = alloca i8, align 1
  br label %while_eval

while_eval:                                       ; preds = %while_exec, %entry_block
  br i1 true, label %while_exec, label %while_cont

while_exec:                                       ; preds = %while_eval
  %0 = call i64 @"[EhAqhPCRkM].io.fn_read(PR_int8*,PR_int64)"(i8* %"var c", i64 1)
  call void @"[EhAqhPCRkM].io.fn_putc(PR_int8)"(i8 10)
  %1 = load i8, i8* %"var c", align 1
  call void @"[EhAqhPCRkM].io.fn_putc(PR_int8)"(i8 %1)
  br label %while_eval

while_cont:                                       ; preds = %while_eval
  ret i32 0
}

define i64 @"[EhAqhPCRkM].io.fn_println(PR_int8*)"(i8* %message) {
entry_block:
  %"param message" = alloca i8*, align 8
  store i8* %message, i8** %"param message", align 8
  %"var len" = alloca i64, align 8
  %0 = load i8*, i8** %"param message", align 8
  %1 = load i8*, i8** %"param message", align 8
  %2 = call i64 @"[EhAqhPCRkM].io.fn_strlen(PR_int8*)"(i8* %1)
  %3 = call i64 @"[CvcrrkVhYE].syscall.fn_write(PR_int64,PR_int8*,PR_int64)"(i64 1, i8* %0, i64 %2)
  store i64 %3, i64* %"var len", align 4
  call void @"[EhAqhPCRkM].io.fn_putc(PR_int8)"(i8 10)
  %4 = load i64, i64* %"var len", align 4
  %5 = add i64 %4, 1
  ret i64 %5
}

define i64 @"[EhAqhPCRkM].io.fn_strlen(PR_int8*)"(i8* %str) {
entry_block:
  %"param str" = alloca i8*, align 8
  store i8* %str, i8** %"param str", align 8
  %"var i" = alloca i64, align 8
  store i64 0, i64* %"var i", align 4
  br label %while_eval

while_eval:                                       ; preds = %while_exec, %entry_block
  %0 = load i8*, i8** %"param str", align 8
  %1 = load i64, i64* %"var i", align 4
  %subscript-ptr = getelementptr i8, i8* %0, i64 %1
  %2 = load i8, i8* %subscript-ptr, align 1
  %3 = icmp ne i8 %2, 0
  br i1 %3, label %while_exec, label %while_cont

while_exec:                                       ; preds = %while_eval
  %4 = load i64, i64* %"var i", align 4
  %5 = add i64 %4, 1
  store i64 %5, i64* %"var i", align 4
  br label %while_eval

while_cont:                                       ; preds = %while_eval
  %6 = load i64, i64* %"var i", align 4
  ret i64 %6
}

define void @"[EhAqhPCRkM].io.fn_putc(PR_int8)"(i8 %ch) {
entry_block:
  %"param ch" = alloca i8, align 1
  store i8 %ch, i8* %"param ch", align 1
  %"var ref" = alloca i8*, align 8
  store i8* %"param ch", i8** %"var ref", align 8
  %0 = load i8*, i8** %"var ref", align 8
  %1 = call i64 @"[CvcrrkVhYE].syscall.fn_write(PR_int64,PR_int8*,PR_int64)"(i64 1, i8* %0, i64 1)
  ret void
}

define i64 @"[EhAqhPCRkM].io.fn_read(PR_int8*,PR_int64)"(i8* %str, i64 %len) {
entry_block:
  %"param str" = alloca i8*, align 8
  store i8* %str, i8** %"param str", align 8
  %"param len" = alloca i64, align 8
  store i64 %len, i64* %"param len", align 4
  %"var readLen" = alloca i64, align 8
  %0 = load i8*, i8** %"param str", align 8
  %1 = load i64, i64* %"param len", align 4
  %2 = call i64 @"[CvcrrkVhYE].syscall.fn_read(PR_int64,PR_int8*,PR_int64)"(i64 0, i8* %0, i64 %1)
  store i64 %2, i64* %"var readLen", align 4
  %3 = load i64, i64* %"var readLen", align 4
  ret i64 %3
}

define i64 @"[CvcrrkVhYE].syscall.fn_close(PR_int64)"(i64 %fd) {
entry_block:
  %"param fd" = alloca i64, align 8
  store i64 %fd, i64* %"param fd", align 4
  %0 = load i64, i64* %"param fd", align 4
  %1 = call i64 (i64, ...) @syscall(i64 3, i64 %0)
  ret i64 %1
}

declare i64 @syscall(i64, ...)

define i64 @"[CvcrrkVhYE].syscall.fn_open(PR_int8*,PR_int32,PR_int32)"(i8* %filename, i32 %flags, i32 %mode) {
entry_block:
  %"param filename" = alloca i8*, align 8
  store i8* %filename, i8** %"param filename", align 8
  %"param flags" = alloca i32, align 4
  store i32 %flags, i32* %"param flags", align 4
  %"param mode" = alloca i32, align 4
  store i32 %mode, i32* %"param mode", align 4
  %0 = load i8*, i8** %"param filename", align 8
  %1 = ptrtoint i8* %0 to i64
  %2 = load i32, i32* %"param flags", align 4
  %3 = sext i32 %2 to i64
  %4 = load i32, i32* %"param mode", align 4
  %5 = sext i32 %4 to i64
  %6 = call i64 (i64, ...) @syscall(i64 2, i64 %1, i64 %3, i64 %5)
  ret i64 %6
}

define i64 @"[CvcrrkVhYE].syscall.fn_write(PR_int64,PR_int8*,PR_int64)"(i64 %fd, i8* %ptr, i64 %len) {
entry_block:
  %"param fd" = alloca i64, align 8
  store i64 %fd, i64* %"param fd", align 4
  %"param ptr" = alloca i8*, align 8
  store i8* %ptr, i8** %"param ptr", align 8
  %"param len" = alloca i64, align 8
  store i64 %len, i64* %"param len", align 4
  %0 = load i64, i64* %"param fd", align 4
  %1 = load i8*, i8** %"param ptr", align 8
  %2 = ptrtoint i8* %1 to i64
  %3 = load i64, i64* %"param len", align 4
  %4 = call i64 (i64, ...) @syscall(i64 1, i64 %0, i64 %2, i64 %3)
  ret i64 %4
}

define i64 @"[CvcrrkVhYE].syscall.fn_read(PR_int64,PR_int8*,PR_int64)"(i64 %fd, i8* %ptr, i64 %len) {
entry_block:
  %"param fd" = alloca i64, align 8
  store i64 %fd, i64* %"param fd", align 4
  %"param ptr" = alloca i8*, align 8
  store i8* %ptr, i8** %"param ptr", align 8
  %"param len" = alloca i64, align 8
  store i64 %len, i64* %"param len", align 4
  %0 = load i64, i64* %"param fd", align 4
  %1 = load i8*, i8** %"param ptr", align 8
  %2 = ptrtoint i8* %1 to i64
  %3 = load i64, i64* %"param len", align 4
  %4 = call i64 (i64, ...) @syscall(i64 0, i64 %0, i64 %2, i64 %3)
  ret i64 %4
}

define i32 @main(i32 %argc, i8** %argv) {
main_entry:
  %0 = call i32 @"HelloWorld.fn_main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv)
  ret i32 %0
}
