; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

@constant_array = internal constant [3 x i32] [i32 21, i32 2, i32 3]

define i32 @"Test.fn_main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %"var test" = alloca i32*, align 8
  %0 = alloca [3 x i32], align 4
  %1 = bitcast [3 x i32]* %0 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %1, i8* align 4 bitcast ([3 x i32]* @constant_array to i8*), i64 12, i1 false)
  %2 = bitcast [3 x i32]* %0 to i32*
  store i32* %2, i32** %"var test", align 8
  %3 = load i32*, i32** %"var test", align 8
  %4 = load i32, i32* %3, align 4
  ret i32 %4
}

; Function Attrs: argmemonly nofree nounwind willreturn
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* noalias nocapture writeonly, i8* noalias nocapture readonly, i64, i1 immarg) #0

define i32 @main(i32 %argc, i8** %argv) {
main_entry:
  %0 = call i32 @"Test.fn_main(PR_int32,PR_int8**)"(i32 %argc, i8** %argv)
  ret i32 %0
}

attributes #0 = { argmemonly nofree nounwind willreturn }
