; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

@str = private unnamed_addr constant [14 x i8] c"Hello, World!\00", align 1

define i32 @"HelloWorld.fn_main(PR_int32,PR_string*)"(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  call void @"[5hiGNADmE0].io.fn_println(PR_string)"(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @str, i32 0, i32 0))
  ret i32 0
}

define void @"[5hiGNADmE0].io.fn_println(PR_string)"(i8* %message) {
entry_block:
  %"param message" = alloca i8*, align 8
  store i8* %message, i8** %"param message", align 8
  ret void
}

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
