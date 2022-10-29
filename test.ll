; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

%0 = type { i64 }

@str = private unnamed_addr constant [4 x i8] c"Hi\0A\00", align 1

define i32 @fn.Test.main.PR_int32.PR_int8-p-p(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %stdout = alloca %0, align 8
  %0 = call %0 @fn._LcM5nSMwX9_.File.create.PR_int32(i32 1)
  store %0 %0, %0* %stdout, align 4
  %1 = getelementptr inbounds %0, %0* %stdout, i32 0, i32 0
  store i64 2, i64* %1, align 4
  %res = alloca i64, align 8
  %2 = call i64 @fn._LcM5nSMwX9_.File.write.ins_File-p.PR_int64.PR_int64(%0* %stdout, i64 ptrtoint ([4 x i8]* @str to i64), i64 3)
  store i64 %2, i64* %res, align 4
  %3 = load i64, i64* %res, align 4
  %4 = trunc i64 %3 to i32
  ret i32 %4
}

define i64 @fn._LcM5nSMwX9_.File.write.ins_File-p.PR_int64.PR_int64(%0* %this, i64 %ptr, i64 %len) {
entry_block:
  %"param this" = alloca %0*, align 8
  store %0* %this, %0** %"param this", align 8
  %"param ptr" = alloca i64, align 8
  store i64 %ptr, i64* %"param ptr", align 4
  %"param len" = alloca i64, align 8
  store i64 %len, i64* %"param len", align 4
  %0 = load %0*, %0** %"param this", align 8
  %1 = getelementptr inbounds %0, %0* %0, i32 0, i32 0
  %2 = load i64, i64* %1, align 4
  %3 = load i64, i64* %"param ptr", align 4
  %4 = load i64, i64* %"param len", align 4
  %5 = call i64 @write(i64 %2, i64 %3, i64 %4)
  ret i64 0
}

declare i64 @write(i64, i64, i64)

define %0 @fn._LcM5nSMwX9_.File.create.PR_int32(i32 %fd) {
entry_block:
  %"param fd" = alloca i32, align 4
  store i32 %fd, i32* %"param fd", align 4
  %f = alloca %0, align 8
  %0 = load i32, i32* %"param fd", align 4
  %1 = getelementptr inbounds %0, %0* %f, i32 0, i32 0
  %2 = sext i32 %0 to i64
  store i64 %2, i64* %1, align 4
  %3 = load %0, %0* %f, align 4
  ret %0 %3
}

define i32 @main(i32 %argc, i8** %argv) {
main_entry:
  %0 = call i32 @fn.Test.main.PR_int32.PR_int8-p-p(i32 %argc, i8** %argv)
  ret i32 %0
}
