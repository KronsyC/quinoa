# ***Compiler Regression Test Suite***

### This directory consists of tests for the quinoa compiler written in the `.qnt` file format. 

### The `quinoa test` format is similar to refular quinoa source files but contains some metadata at the top level of the file.

---

## QNT Metadata Entries:
- `name`:   
    The name of test, must not contain whitespace

- `template`:   
    The template to wrap the test file with,   
    the following are valid templates:   
    - `main`:   
        All text is put inside the body of a main function
        `func main() -> i32`

    - `mod`:   
        All text is put inside a module with the same name as the test

    - `raw`:   
        A full, valid quinoa program is expected

    The default template is `main`

- `error`:   
    The error with which the compiler is expected to exit,   
    can be any of the errors mentioned in `error.hh`
    

## QNT DSL

The `qnt` file format exposes directives which are not usually available
in quinoa source files, prefixed by `$`

- `equals` : Assert that two pieces of data are equal
- `not_equals` : Assert that two pieces of data are inequal
- `errors` : Assert that the enclosed statement causes a compilation error
- `hoist` : Hoist a piece of text to the top level of the file



    
    