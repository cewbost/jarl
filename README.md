# Jarl

A simple functional, reactive scripting language, born out of a frustration with more
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

## Testing

The unit tests are run by GNU make and bash scripts, and depend on valgrind.

To run the tests under linux, make sure you have GCC, GNU Make, CMake and Valgrind
installed, go to the test directory and run make.

## Documentation

Todo.

For examples of how the language works, check the unit tests.