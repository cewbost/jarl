# Jarl

A functional, reactive scripting language, born out of a frustration with more
conventional languages like Lua and Squirrel.

The main goal is to make it easier and safer to manage persistent state across several
threads of execution. Scripting languages are not generally used to perform heavy
computations, but for configuring other software. ACIDity is an important goal.
Safety, dryness and memory footprint will be prioritized over runtime performance.

This is what's planned:

  - Consistency guarantees for persistant state. Consistence checks for state changes,
    and easy rollback on errors. Transactional updates.
  - Less error prone semantics, arguments passed by immutable reference, unique
    ownership by default.
  - Multithreading support. Safe communication between parallel VMs.
  - A more powerfull type system. Type classes, types as values and pattern matching
    will give stronger safety guaranties and genericity.
  - JIT compilation friendly design. Type annotations and heavy use of functional
    patterns.
  - Embedding api will strive for simplicity while giving control over the internals.

At the moment this is in very early development. If you try this language it will
cause chrashes due to improper error handling. Also it has no persistent state.

## Building

Use CMAKE. Requires C++14.

Currently it depends on the C++ standard library. This will be optional in the future.

The lexer code is generated using re2c. The generated file is included in the src/
folder, but if you want to generate it again, or modify the lexer, make the changes
to re2c/lexer.re and run re2c/Makefile.

## Documentation

Todo.

## Examples

Everything is an expression:

```
//if statements are expressions
var x = if 1 == 2: "Yes" else "No"
print x
//prints "No"

//code blocks are expressions
x = {
  var y = 0
  var z = 5
  while z > 0: {
    y = y * 3 + 7
    z -= 1
  }
  y
}
print x
//prints "847"

print (if {
  var y = 0
  var z = 5
  while z > 0: {
    y = y * 3 + 7
    z -= 1
  }
  y
} % 2 == 0: "Goodby" else "Hello") ++ ", World!"
//prints "Hello, World!"
```

Arrays and slices:

```
var x = [1:6]
print x
//[1, 2, 3, 4, 5]
print x[1:-1]
//[2, 3, 4]
print x[2:] ++ x[:2]
//[3, 4, 5, 1, 2]
```

Lambdas, higher order functions, partial application and currying:

```
var x = 5
var f = func a: x * a
print f 2
//10

f = func a, b, f: f a b
print f 3 4 (func a * b)
//12

f = func a, b: a * b
var g = f 10
var h = f 2
print (g 5) + (h 2)
//54

f = func a: func b: a * b
print f 3 4
//12
```
