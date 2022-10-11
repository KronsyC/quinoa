; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

%0 = type { i32, i32, i8*, i8* }

define i32 @"HelloWorld.fn_main(PR_int32,PR_string*)"(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %0 = call i32 (i32, ...) @"HelloWorld.fn_sum(...PR_int32[])"(i32 3, i32 1, i32 2, i32 3)
  ret i32 %0
}

define i32 @"HelloWorld.fn_sum(...PR_int32[])"(i32 %"+vararg_count", ...) {
entry_block:
  %"param +vararg_count" = alloca i32, align 4
  store i32 %"+vararg_count", i32* %"param +vararg_count", align 4
  %varargs_len = load i32, i32* %"param +vararg_count", align 4
  %varargs_list = alloca i32, i32 %varargs_len, align 4
  %var_args_obj = alloca %0, align 8
  %i = alloca i32, align 4
  store i32 0, i32* %i, align 4
  %0 = bitcast %0* %var_args_obj to i8*
  call void @llvm.va_start(i8* %0)
  br label %while_eval

while_eval:                                       ; preds = %while_body, %entry_block
  %1 = load i32, i32* %i, align 4
  %2 = icmp slt i32 %1, %varargs_len
  br i1 %2, label %while_body, label %while_cont

while_body:                                       ; preds = %while_eval
  %3 = va_arg %0* %var_args_obj, i32
  %4 = getelementptr i32, i32* %varargs_list, i32 %1
  store i32 %3, i32* %4, align 4
  %5 = add i32 %1, 1
  store i32 %5, i32* %i, align 4
  br label %while_eval

while_cont:                                       ; preds = %while_eval
  %"var i" = alloca i32, align 4
  %"var sum" = alloca i32, align 4
  %6 = load i32, i32* %"var sum", align 4
  store i32 0, i32* %"var sum", align 4
  %7 = load i32, i32* %"var i", align 4
  store i32 0, i32* %"var i", align 4
  br label %for_eval

for_eval:                                         ; preds = %for_inc, %while_cont
  %8 = load i32, i32* %"var i", align 4
  %9 = icmp slt i32 %8, %varargs_len
  br i1 %9, label %for_exec, label %while_cont1

for_inc:                                          ; preds = %for_exec
  %10 = load i32, i32* %"var i", align 4
  %11 = load i32, i32* %"var i", align 4
  %12 = add i32 %11, 1
  %13 = load i32, i32* %"var i", align 4
  %14 = add i32 %13, 1
  store i32 %14, i32* %"var i", align 4
  br label %for_eval

for_exec:                                         ; preds = %for_eval
  %15 = load i32, i32* %"var sum", align 4
  %16 = load i32, i32* %"var sum", align 4
  %17 = load i32, i32* %"var i", align 4
  %subscript-ptr = getelementptr i32, i32* %varargs_list, i32 %17
  %18 = load i32, i32* %subscript-ptr, align 4
  %19 = add i32 %16, %18
  %20 = load i32, i32* %"var sum", align 4
  %21 = load i32, i32* %"var i", align 4
  %subscript-ptr2 = getelementptr i32, i32* %varargs_list, i32 %21
  %22 = load i32, i32* %subscript-ptr2, align 4
  %23 = add i32 %20, %22
  store i32 %23, i32* %"var sum", align 4
  br label %for_inc

while_cont1:                                      ; preds = %for_eval
  %24 = load i32, i32* %"var sum", align 4
  ret i32 %24
}

declare void @llvm.va_start(i8*)

define i64 @"[7bYxoXyXxS].io.fn_println(PR_string)"(i8* %message) {
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
