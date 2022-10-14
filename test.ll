; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

define i32 @"Test.fn_main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %0 = call i64 @"[YKH0hYd3ld].syscall.fn_exit(PR_int64)"(i64 21)
  ret i32 0
}

define i64 @"[YKH0hYd3ld].syscall.fn_exit(PR_int64)"(i64 %status) {
entry_block:
  %"param status" = alloca i64, align 8
  store i64 %status, i64* %"param status", align 4
  %0 = load i64, i64* %"param status", align 4
  %1 = call i64 (i64, ...) @syscall(i64 60, i64 %0)
  ret i64 %1
}

declare i64 @syscall(i64, ...)

define i64 @"[YKH0hYd3ld].syscall.fn_close(PR_int64)"(i64 %fd) {
entry_block:
  %"param fd" = alloca i64, align 8
  store i64 %fd, i64* %"param fd", align 4
  %0 = load i64, i64* %"param fd", align 4
  %1 = call i64 (i64, ...) @syscall(i64 3, i64 %0)
  ret i64 %1
}

define i64 @"[YKH0hYd3ld].syscall.fn_open(PR_int8*,PR_int32,PR_int32)"(i8* %filename, i32 %flags, i32 %mode) {
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

define i64 @"[YKH0hYd3ld].syscall.fn_write(PR_int64,PR_int8*,PR_int64)"(i64 %fd, i8* %ptr, i64 %len) {
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

define i64 @"[YKH0hYd3ld].syscall.fn_read(PR_int64,PR_int8*,PR_int64)"(i64 %fd, i8* %ptr, i64 %len) {
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
  %0 = call i32 @"Test.fn_main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv)
  ret i32 %0
}
