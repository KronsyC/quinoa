---
name: Exit_Status
---

let foo = 16;
let bar = foo;

$equals(foo, bar)

foo = 17;

$nequals(foo, bar)