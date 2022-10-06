; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

declare void @puts(i8*);


define i32 @"HelloWorld.fn_main(PR_int32,PR_string*)"(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %"var message" = alloca i8*, align 8
  %"temp load for subscript" = load i8**, i8*** %"param argv", align 8
  %"subscript-ptr of 'argv'" = getelementptr i8*, i8** %"temp load for subscript", i32 1
  %"element of argv" = load i8*, i8** %"subscript-ptr of 'argv'", align 8
  store i8* %"element of argv", i8** %"var message", align 8
  %"loaded var 'message'" = load i8*, i8** %"var message", align 8
  call void @"[17KGJw2NkK].io.fn_println(PR_string)"(i8* %"loaded var 'message'")
  ret i32 0
}

define void @"[17KGJw2NkK].io.fn_println(PR_string)"(i8* %message) {
entry_block:
  %"param message" = alloca i8*, align 8
  store i8* %message, i8** %"param message", align 8
  call void @puts(i8* %message);
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
