# Stark development roadmap
## Stark subset in C
- [ ] Source to tokens
- [ ] Tokens to AST
  - [ ] Constant definition
  - [ ] Variable definition
  - [ ] Function literal with variable arguments only
  - [ ] Support normal function calls (no infix or one-parem calls)
  - [ ] Integer literal
  - [ ] C String literal
  - [ ] Integer supported: `u64`
  - [ ] String supported: `cstr`
  - [ ] Expression blocks
  - [ ] `brk` and `ret`
  - [ ] If expressions
  - [ ] Structs
- [ ] AST to assembly (fasm)

Notes about the subset:
- No compile-time execution
- No type inference for variables or constants
- Function type inference is allowed because functions don't have types in the subset
- Functions don't have an error return type
- Only types supported: `u64`, `cstr` and custom structs
- Only standard string literals, no raw stuff
- No immutability for variables
- No struct constructor
- No constant members for structs
- No unnamed fields for structs
- No unnamed structs or functions
- No default values for function arguments
- Structs still must have padding
- If expressions are allowed
- All pointers are mutable and 
- Only supported operators are the four basic operations (`+`, `-`, `*`, `/`), bitwise ones (`<<`, `>>`, `|`, `&`), boolean logic (`!`, `==`, `>=`, `<=`, `>`, `<`, `&&`, `||`) and assignment (`=`)
- Supports an intrinsic called `__syscall__`, this intrinsic takes 7 arguments, the first one is the syscall number and the rest is the syscall arguments. This intrinsic will be removed in the future.
- Any features not explicitly stated here is simply not allowed

Stark compiler on C rules:
- No libc
- No external libraries
- Only structs, strings and `uint64_t` is allowed as types
- No preprocessor
- C89

This subset is called _Starc_ because it's Stark written in C.

To facilitate future stark self hosting:
no libc or external libraries are allowed.

## Start self hosting
Start implementing the full Stark spec (not the Starc subset) using Starc. When the compiler start to rival Starc ditch it and start to use Stark itself, the switch should be trivial.

