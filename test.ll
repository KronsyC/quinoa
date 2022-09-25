; ModuleID = 'Quinoa Program'
source_filename = "Quinoa Program"

@str = private unnamed_addr constant [14 x i8] c"Hello, World!\00", align 1

define i32 @HelloWorld.main(i32 %argc, i8** %argv) {
entry_block:
  %"param argc" = alloca i32, align 4
  store i32 %argc, i32* %"param argc", align 4
  %"param argv" = alloca i8**, align 8
  store i8** %argv, i8*** %"param argv", align 8
  %"var execpath" = alloca i8*, align 8
  %"temp load for subscript" = load i8**, i8*** %"param argv", align 8
  %"subscript-ptr of 'argv'" = getelementptr i8*, i8** %"temp load for subscript", i32 0
  %"element of argv" = load i8*, i8** %"subscript-ptr of 'argv'", align 8
  store i8* %"element of argv", i8** %"var execpath", align 8
  call void @io.println(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @str, i32 0, i32 0))
  ret i32 0
}

define void @io.println(i8* %message) {
entry_block:
  %"param message" = alloca i8*, align 8
  store i8* %message, i8** %"param message", align 8
  ret void
}
