# Stark
A simple language for systems programming.

Current version: 0.1.0

## Hello, World!

```
def io : @import "std.io";

def main : () => io.println("Hello, World!");
```

## Functions
### Function definition and calling conventions
Function are defined using the `def` keyword, followed by an identifier, followed by the constant assignment operator `:`, then the argument list `(arg0 = type0, arg1 = type1, ...)`, next the return type, followed by the body assignment operator `=>` and finally an expression, that expression is the function body:
```
def func_name : (arg0 = type0, arg1 = type1, ...) return_type => func_body_expr;
```

Functions always returns the expression at the function body result.

Identifiers have to start with an underscore or any letter, followed by an n amount of underscores, letters or numbers:
```
a # valid identifier
_a # valid identifier
abc__0123 # valid identifier
0a # invalid identifier
```

[The `_` identifier is reserved](#Unused-variables-and-the-'_'-identifier)

A basic function that returns a sum of two numbers would be defined as:
```
def sum : (a = i32, b = i32) i32 => a + b;
```

To call a regular function:
```
def foo : (_a = i32, _b = i32) =>;
foo(10, 12);
```

But if it takes exactly one argument the parenthesis are optional:
```
def foo : (_x = i32) =>;
foo 10;
```

And if it takes exactly two arguments it can be called as an 'infix call' `x foo y`, where `x` and `y` are arguments and `foo` is the function:
```
def foo : (_a = i32, _b = i32) =>;
10 foo 12;
```

Chaining infix function calls is possible (see the [precedence table](#Precedence-table)):
```
def foo : (_a = i32, _b = i32) =>;
2 foo 4 foo 6; # equivalent to (2 foo 4) foo 6 
```

### Unused arguments
Unused arguments causes a compile-time error:
```
def sum : (a = i32, b = i32, c = i32) i32 => a + b; # error: 'c' is unused
```

To supress the error put an underscore as a prefix on the argument name:
```
def sum : (a = i32, b = i32, _c = i32) i32 => a + b; # valid
```

### Void return type
For functions with no return value use [void](#The-void-type) as the return type:
```
def foo : () void =>;
```

`void` is infered if no return type is provided:
```
def foo : () =>;
```

### Expresion blocks
For functions with more then one expression use an expression block.
```
def foo : () => {
  expr0;
  expr1;
}
```

If an early break from the expression block is required use the `brk` keyword:
```
def do_more_stuff : () => {
  do_stuff();
  do_stuff();
  brk;
  do_stuff(); # will not be executed
}
```

By default expression blocks do not return any value. Use the `brk` keyword followed by one expression to break from the expression block with a return value:
```
def do_stuff_and_sum : (x = i32, y = i32) i32 => {
  do_stuff();
  do_stuff();
  brk x + y;
}
```

Because expression blocks are also just expressions they can be nested:
```
def foo : () => {
  { }
  { }
}
```

### Break and return from expression blocks
Keep in mind that the `brk` keyword only breaks from the current expression block scope:
```
def foo : () => {
  {
    brk;
    do_stuff(); # will not be executed
  }
  do_stuff(); # will be executed
}
```

If you want to return from all nested expression blocks at once use the `ret` keyword:
```
def foo : () => {
  {
    ret;
    do_stuff(); # will not be executed
  }
  do_stuff(); # will not be executed
}
```

And of course, `ret` also can have an expression as a return value:
```
def five : () i32 => {
  {
    ret 5; # 5 is returned from the function
    do_stuff(); # will not be executed
  }
  do_stuff(); # will not be executed
}
```

The `brk` statement can be nested:
```
def foo : () => {
  {
    {
      brk brk;
      do_stuff(); # not executed
    }
    do_something(); # not executed
  }
  do_something_else(); # executed
}
```

It can be nested how many times you want infact:
```
def foo : () => {
  {
    {
      {
        {
          brk brk brk brk;
          do_stuff(); # not executed
        }
        do_something(); # not executed
      }
      do_something_else(); # not executed
    }
    do_something_extra(); # not executed;
  }
  do_real_work(); # executed;
}
```

But this is not recomended, try to keep your functions without much nested expression blocks.

### Variable and constant arguments
Most functions arguments are variables, so they can be defined in the same way (see more on [the variables section](#Variable-definitions)):
```
def fn0 : (_x = i32) =>;
def fn1 : (_x, _y = i32, _c = f32) =>;
def fn2 : (_x = i32 1) =>; # default values have to be compile-time constants
def fn3 : (_x, _y = i32 1) =>;
def fn4 : (_x = 0) =>;
def fn5 : (_x, _y = 0, _c, _d = 0.0) =>;
```

Because function arguments are variables they also are [immutable](#Variable-mutability) by default. Make them mutable with the `mut` attribute:
```
def foo : (_x = i32) => _x = 10; # invalid, '_x' is immutable
def foo : (mut _x = i32) => _x = 10; # valid, '_x' is mutable
```

Functions can also have constant arguments:
```
def foo : (num : u64 0) u64 => num;
```

That means that you can only pass constant values for those arguments:
```
def foo : (num : u64 0) u64 => num;
def x = foo(10); # valid
def y = foo(x); # invalid, x isn't a constant
```

Constant arguments always have to be the first ones in a function:
```
def foo : (var = i32, const : u64) =>; # invalid
def foo : (const : u64, var = i32) =>; # valid
```

Constant arguments are the only place where you don't need to specify the constant value on definition (see more on [the constants section](#Constants)):
```
def foo : (num : u64) u64 => num;
```
So the code above doesn't actually make a type aliases, instead it makes an i64 constant.

Keep in mind that non-compile-time functions with constant arguments, generates a new function every time they are used with a different constant value through out your program. This can lead to more 'bloated' executables, but the performance is not affected. 

### Compile-time functions
If a function only takes constant arguments (or no arguments at all), it don't access external non-constant data, it's private to the current [module](#Modules) and no function pointers are taken from it through out the module; it becomes a compile-time function. I.e. it'll be run at compile-time:
```
def comp_sum : (a, b : i32) i32 => a + b;
def x = comp_sum(12, 4); # 16 will be computed at compile-time
```

It's actually possible to run any function, that don't access external non-constant data, at compile time if you pass compile-time known values as its arguments and use the `@` operator:
```
def sum : (a, b = i32) i32 => a + b;
def x = @sum(12, 4); # 16 will be computed at compile-time
```

If the intended behaviour for a function is to be ran at compile-time, it's good practice to always call it using the `@` operator instead of relaying on compile-time functions. That is because breaking the compile-time function requirements is very easy.

If a non-compile-time function is always used as such they'll be optimised out of the program and will not have memory.

You generally can't run functions at module scope, using the `@` operator you can:
```
def foo : () => do_something();

foo(); # invalid
@foo(); # valid

def main : () => {
}
```

It's also possible to ensure that a function have to be a compile-time one using `@` in the function definition:
```
def comp_sum : @(a, b = i32) i32 => a + b;
def x = @comp_sum(12, 4); # 16 will be computed at compile-time
def y = comp_sum(12, 4); # same behaviour, but the above one is best-practice
```

This generates an error if any of the compile-time function rules are broken for this specific function.

### Generic functions
With constant arguments you can make generic functions using the `type` type:
```
def sum : (T : type, a, b = T) T => a + b;
```

It's also possible to use the `is` (see more on [the type families section](#Type-Families-and-'is'-operator)):
```
def sum : (T : type is number, a, b = T) T => a + b;
```

### Immutable arguments are references
Immutable arguments that have a value bigger than 8 bytes are passed as references (implicit pointer). That way copies are not made.

So you don't need to explicitly pass immutable pointers around for big structs, like you would do it in C. It is actually more performant if you don't use immutable pointers to just pass values because inlining becomes a possibility:
```
def foo(x = *Big_Struct) Something => x.something;
def bar(x = Big_Struct) Something => x.something;
def big = Big_Struct;
foo(&big)
bar(big) # performance for this is the same as bar or better if inlined
```

You can't take the address of or edit member values of arguments passed by reference. If you want that functionality you have to explicitly tell the compiler and the user using the reference type:
```
def foo(x = Big_Struct) => x.something = something_else(); # invalid
def bar(x = &Big_Struct) => x.something = something_else(); # valid
def big = Big_Struct;
foo(big) # passed by implicit reference
bar(big) # passed by reference
```

### Functions are just values
Functions are just values assigned to [constants](#Constants) with function types. So you can create an anonymous function using a function literal and call it directly:
```
def num = (x, y = i32) i32 => x + y;(10, 20); # num is assigned to 30
```

The type of a function looks something like this:
```
fn(arg0 : type0, arg1 : type1, arg2 = type2, arg3 = type3) return_type
```

Here are some examples with functions and what the type will look like:
```
def square_i32 : (x = i32) i32            => x*x;
def square_any : (T : number, x = T) T    => x*x;
def sum_i32    : (x, y = i32) i32         => x+y;
def sum_any    : (T : number, x, y = T) T => x+y;
def set_any_x  : (T : type, foo = T)      => foo.x = something();
# square_i32 type: fn(x = i32) i32
# square_any type: fn(T : number, x = T) T
# sum_i32 type:    fn(x = i32, y = i32) i32
# sum_any type:    fn(T : number, x = T, y = T) T
# set_any_x type:  fn(T : number, x = T, y = T) void
```

With that in mind you can define a function without [type inferrency](#Type-inferrency):
```
def sum_any : fn(T : number, x = T, = T) T (T : number, x, y = T) T => x+y;
```

But this is extremely verbose.

Because of the 'functions are just values' mindset nested functions are simple:
```
def foo : () => {
  def bar : (x, y = i32) i32 => x + y;
}
```

Function literals assigned to constants are considered 'named functions'. This is basicaly taking adventage of [constant scoping](#Constant-scoping) and not actually a seperate feature. I.e. recursion is possible. 

### Function pointers
Variables can't have the function types, only constants. For variables use function pointers instead.

To assign a function pointer to a variable you take its address:
```
def foo : () =>;
def var = &foo;
```

This would be an error:
```
def foo : () =>;
def var = foo; # error: trying to assign function type to variable
```

Function literals can be assigned directly to variables if you take its address:
```
def var = &(x = i32) i32 => x*x;
```

The code above is basically creating an anonymous function and assigning its address to the variable.

You can't use recursion on functions assigned directly to variables because they're anonymous:
```
def var = &(x = i32) i32 => var(x)*x; # error: var is undefined
```

Function pointers can't have constant arguments. To get a function pointer from a function with constant arguments you have to do the following:
```
def square : (T : number, x = T) T => x*x;
def ptr_to_square_i32 = &square(i32);
def ptr_to_square_f32 = &square(f32); # different pointer from 'ptr_to_square_i32'
```

The type of a function pointer looks like this:
```
*fn(type0, type1, ...) return_type # function pointer
```

Obviously is not possible to take the mutable address of a function, so mutable function pointer aren't a thing.

### Passing functions as arguments
There are basically three ways of passing functions as arguments:
1. Function pointers
2. Constant arguments with function type
3. Constant arguments with [overload set](#Overload-sets) type
```
def some_overload : ol|a is number|(a, a) a;
def foo : (fn = *(i32, i32) i32, x, y = i32) i32 => fn(x, y);
def bar : (fn : function, x, y = i32) i32 => fn(x, y);
def bar : (fn : some_overload, x, y = i32) i32 => fn(x, y);
```

The function pointer one will have memory, so it can be assigned to things.

The overload set one will accept all functions that are part of that overload_set.

And the function type one will accept any function.

## Constants
Constants are immutable compile-time known values.

### Constant definitions
To define a constant use the `def` keyword followed by an identifier and the constant assignment operator `:`. After that a compile-time known value must be provided:
```
def thirty : 30;
```

### Type inferrency
You can optionally provide a type explicitly to a constant definition:
```
def thirty : i32 30;
```

For more info into what type a literal infer check the [inferrency table](#Type-inferrency-by-value-table).

### Type aliases
Differently from [variables](#Assignment-after-definition), constants can't be assigned after definition. If you define a constant to be equal to a type, you'll instead create a type aliases:
```
def int : i32; # int is a type alias of i32
def foo (x = int) int => x*x+x;
def my_i32 = i32 foo 10; # valid
```

the type of a type alias is... `type`. So you can define it explicitly:
```
def int : type i32; # int is a type alias of i32
```

You can also create distinct type aliases using the `type()` constructor: 
```
def int : type(i32); # int is its own distinct type
def foo (x = int) int => x*x+x;
def my_i32 = i32 foo 10; # invalid, int != i32
```

### Multiple constants definitions at once
It's also possible to define multiple constants at once separating them by commas:
```
def FOO, BAR : 10; # both will have anyi type with 10 as the value
```

With this a function can have multiple names:
```
def foo, foo_alternative : (x = i32) i32 => x*x+x; # Both foo and foo_alternative will have the same function address
def foo_alternative2 : foo; # This also generates a constant with the same function address
```

### Constant scoping
When a constant is defined it'll become available to all the current scope and all scopes inside the current scope. So forward declaring is possible:
```
def bar_squared : () i32 => BAR*BAR;
def BAR : 10;
```

This is particularly good for functions:
```
def square_and_sum : (x, y = i32) i32 => square x + square y;
def square : (x = i32) i32 => x*x;
```

### Constant shadowing
It's possible to shadow a constant in nested scopes:
```
def TEN : 10;
def foo : () => {
  def TEN : 20; # for this scope only 'TEN' will be 20
}
def TEN_AGAIN : TEN; # 'TEN_AGAIN' will be 10
```

You can't shadow a constant in the same scope:
```
def TEN : 10;
def TEN : 20; # invalid, 'TEN' is already defined
```

### Unused constants
Similarly to [unused arguments](#Unused-arguments), unused constants also throw compile errors:
```
def FOO : 10; # error: 'FOO' is never used
```

You solve this in the same way unused arguments did: add an underscore at the start of the identifier:
```
def _FOO : 10; # valid
```

## Variables
Variables are very similar to [constants](#Constants). The main difference is that variables are runtime values.

### Variable definitions
To define a constant use the `def` keyword followed by an identifier and the variable assignment operator `=`. It's similar to constants, infact you can define it in the same way constants can be defined:
```
def x = 1; # inferred integer
def y = i32 2; # explicit type
def a, b, c = 345; # Multiple variables inferred as the same type
def e, f, g = i32 678; # Multiple variables defined with an explicit type
```

The first main difference between variable and constant definitions is type aliasing. You can't make a variable type alias.

This:
```
def x = i32;
```
Will actually create a variable 'x' with type 'i32' initialized to 0.

This syntax also works for multiple variable definitions at once:
```
def x, y = i32; # both x and y are of type 'i32' and initialized to 0
```

### Default initialization
When you define a variable without giving it a default value it'll always default to 0 ([or whatever the default value is](Default-member-values)).

If you want to garbage initialize a variable, for performance or whatever reasons, you can do so explicitly:
```
def x = i32 ---; # defaults to stack garbage
```

### Variable mutability
All variables are immutable by default and cannot be assigned to after its definition.
```
def x = 10;
x = 20; # invalid, x is immutable
def y = i32;
y = 10; # invalid, y is immutable
```

If mutability is a needed factor for a variable use the `mut` attribute on its definition:
```
def x = mut 10;
x = 20; # valid, x is mutable
def y = mut i32;
y = 10; # valid, y is mutable
```

### Unused variables and the '_' identifier
It's exactly the same situation as [unused constants](#Unused-constants) or [unused arguments](#Unused-arguments):
```
def x = 10; # error: 'x' is never used
def _x = 10; # valid
```

The `_` identifier is a special one. It can be defined numerous times inside the same scope and it's not possible to read its value:
```
def _ = 10;
def _ = 20;
def x = _; # error, '_' is not defined
```

The value inside `_` is immediataly thrown way and not stored in memory.

This is particularly useful for functions. Function return values _have_ to be used, so this is a way to signal that the value does not matter:
```
def five : () i32 => 5;
five(); # error: unhandled return value
def _ = five(); # result is thrown away
```

It's also possible to define `_` as a constant as well. This is useful for getting rid of compile-time functions return values at module scope:
```
def foo : () i32 => return_some_i32();

@foo(); # invalid, unused return value
def _ : @foo(); # valid

def main : () => {
}
```

Technically you can do the same thing using just the variable `_` instead of the constant one. But this is good if in the future you want this value to be a constant:
```
def _ : @foo(); # before
def easy_change : @foo(); # after
```

### Variable scoping
it works differently than [constant scoping](#Constant-scoping). A variable cannot be forward declared.

### Variable shadowing
Exactly the same as [constant shadowing](Constant-shadowing).

## Primitive types
There are two types of primitive types, the ones that can be assigned to variables and the ones that are constant assignable only.

### Normal primitive types
- __Integers:__
  - _Signed:_ `i8`, `i16`, `i32`, `i64`
  - _Unsigned:_ `u8`, `u16`, `u32`, `u64`
  - _Pointer sized (generally 8 or 4 bytes):_ `isize`, `usize`
- __Boolean (1 byte):__ `bool`
- __Floating points:__ `f32`, `f64`
- __Characters:__
  - _UTF-32 (4 bytes):_ `uchar`
  - _ASCII (1 byte):_ `achar`
- __Strings:__
  - _Regular string (generally 16 or 12 bytes):_ `str`
  - _C string (generally 16 or 8 bytes):_ `cstr`

### Constant-only primitive types
None of the constant-only primitives have size
- __Any integer:__ `anyi`
- __Any floating point:__ `anyf`
- __Any string:__ `anys`
- __Variable type:__ `type`

### Signed integers
Signed integers are represented using [two's complement](https://en.wikipedia.org/wiki/Two%27s_complement).

### The void type
The `void` type is a special one. It's basically a type that doesn't hold any value.

It's useful for returning nothing from a function or [making a pointer that points to anything](#Void-pointer).

### Strings
Strings are UTF-8 enconded non-null-terminated.

The layout of a string in memory (illustrated as a struct) looks like this:
```
def str : struct(
  data        = *u8,
  length      = u32,
  bytes_count = u32,
);
```
Where `length` is the amount of characters on the string and `bytes_count` is the actual amount of bytes needed by the string.

Strings don't actually hold any data. They're equivalent to string views in other languages.

C strings are similar, but they don't have a `bytes_length` field, they're ASCII encoded and null-terminated.

### Booleans
Booleans are really simple, they just can have two states: `true` or `false`
- `true`: Equivalent to 1 in integers
- `false`: Equivalent to 0 in integers

### Type inferrency by value table
| Value            | Decription       | Constant inferred type              | Variable inferred type                                            |
|------------------|------------------|-------------------------------------|-------------------------------------------------------------------|
| 1234             | Integer literal  | anyi (can be passed to any integer) | Inferred by first usage (u8, i16, etc). If needed defaults to i32 |
| 0x123abc         | Hex literal      | anyi (can be passed to any integer) | Inferred by first usage (u8, i16, etc). If needed defaults to u64 |
| 0b010101         | Bin literal      | anyi (can be passed to any integer) | Inferred by first usage (u8, i16, etc). If needed defaults to u32 |
| 0567             | Oct literal      | anyi (can be passed to any integer) | Inferred by first usage (u8, i16, etc). If needed defaults to u32 |
| 123usize         | usize literal    | usize                               | usize                                                             |
| 123isize         | isize literal    | isize                               | isize                                                             |
| 123u8            | u8 literal       | u8                                  | u8                                                                |
| 123u16           | u16 literal      | u16                                 | u16                                                               |
| 123u32           | u32 literal      | u32                                 | u32                                                               |
| 123u64           | u64 literal      | u64                                 | u64                                                               |
| 0x123abcu8       | Hex u8 literal   | u8                                  | u8                                                                |
| 0x123abcu16      | Hex u16 literal  | u16                                 | u16                                                               |
| 0x123abcu32      | Hex u32 literal  | u32                                 | u32                                                               |
| 0x123abcu64      | Hex u64 literal  | u64                                 | u64                                                               |
| 0b010101u8       | Bin u8 literal   | u8                                  | u8                                                                |
| 0b010101u16      | Bin u16 literal  | u16                                 | u16                                                               |
| 0b010101u32      | Bin u32 literal  | u32                                 | u32                                                               |
| 0b010101u64      | Bin u64 literal  | u64                                 | u64                                                               |
| 0b010101i8       | Bin i8 literal   | i8                                  | i8                                                                |
| 0b010101i16      | Bin i16 literal  | i16                                 | i16                                                               |
| 0b010101i32      | Bin i32 literal  | i32                                 | i32                                                               |
| 0b010101i64      | Bin i64 literal  | i64                                 | i64                                                               |
| 0567u8           | Oct u8 literal   | u8                                  | u8                                                                |
| 0567u16          | Oct u16 literal  | u16                                 | u16                                                               |
| 0567u32          | Oct u32 literal  | u32                                 | u32                                                               |
| 0567u64          | Oct u64 literal  | u64                                 | u64                                                               |
| 0567i8           | Oct i8 literal   | i8                                  | i8                                                                |
| 0567i16          | Oct i16 literal  | i16                                 | i16                                                               |
| 0567i32          | Oct i32 literal  | i32                                 | i32                                                               |
| 0567i64          | Oct i64 literal  | i64                                 | i64                                                               |
| 12.34            | Float literal    | anyf (can be passed to any float)   | Inferred by first usage (f32 or f64). If needed defaults to f32   |
| 123.0f32         | f32 literal      | f32                                 | f32                                                               |
| 123.0f64         | f64 literal      | f64                                 | f64                                                               |
| true/false       | Boolean literal  | bool                                | bool                                                              |
| null             | Null constant    | option                              | option                                                            |
| "text"           | String literal   | anys (can be passed to str or cstr) | str                                                               |
| c"text"          | C string literal | cstr                                | cstr                                                              |
| i32, f32, Struct | Types directly   | type                                | The typed 'type' (i32, f32, etc) with a default value             |
| \[1, 2, 3\]      | Array literal    | \[amount-of-elements\]inferred      | \[amount-of-elements\]inferred                                    |

__Caveats:__
* Passing constants with `anyi` type into mismatched integers will cause an error. E.g. constant with value 1234 passed to an `u8`
* Passing constants with `anys` type with non-ascii characters into a `cstr` will cause an error.
* Binary, octal and hexdecimal literals can't be negative. But they can be assigned to signed integers.
* An array type will always be based on the inferred type of the first element. E.g. `array = [1, 2, 3]` == `array = [3]inferred-type-of-array[0]`

## Pointers
You can grab a pointer to a variable using the `&` operator:
```
def a = 0;
def ptr = &a;
```

The type of a pointer is represented by a `*` followed by the type that the pointer points to:
```
*i32 # pointer to an i32
*str # pointer to a str
**u8 # pointer to a pointer to an u8
```

### Derreferencing
To derreference a pointer use the `*` operator, similarly to C:
```
def a = 12;
def ptr = &a;
def b = *ptr; # b = 12
```

Accessing a member from a pointer to a [struct](#Structs) causes an implicit derreference:
```
def a = Some_Struct(.some_i32 = 12);
def ptr = &a;
def b = ptr.some_i32; # equivalent to 'def b = (*ptr).some_i32;'
```

### Pointer mutability
Similarly to [variables](#Variable-mutability), pointers are immutable by default.

If you want to assign to the variable the pointers points to a mutable pointer is needed.

Use the `&mut` operator on a variable to take the mutable address from it. You can only make mutable pointers from mutable variables:
```
def a = 0;
def ptr = &mut a; # invalid, 'a' isn't mutable
def b = mut 0;
def ptr_mut = &mut a; # valid, 'b' is mutable
```

Now assigning to derreferenced values is possible:
```
def a = mut 0;
def ptr = &a;
*ptr = 10; # error: 'ptr' is not a mutable pointer
def mut_ptr = &mut a;
*mut_ptr = 10; # valid, now 'a' = 10
```

Do not confuse `*mut` with `mut *`. `*mut` is a pointer that can change the value inside the address it's pointing to. `mut *` is a pointer that can change the actual address it points to:
```
def a = mut i32;
def b = mut i32;
def p0 = *mut i32 &a;
*p0 = 10; # valid, it's a mutable pointer
p0 = &b; # invalid, 'p0' is immutable
def p1 = mut *i32 &a;
*p1 = 10; # invalid, it's an immutable pointer
p1 = &b; # valid, 'p1' is mutable
```

### Pointer arithimatic
It's not possible to use the `[]` operator to index through a pointer.

But you can achieve something similar with pointer arithimatic:
```
def nums = [1, 2, 3];
def ptr = &mut something;
def x = *(ptr + 1); # equivalent to nums[1]
```

The only operations possible with pointers are: `+` and `-`

### Pointer definition
Differently from other variable types, pointers cannot be initialized to nothing. This is because pointers don't have a default value and must always point to memory:
```
def ptr = *i32; # error: uninitialized pointer
```

This actually enables several advantages compared to pointers that permit something like `NULL` or `nullptr`:
- No need to check for invalid pointers
- Less branching
- Safer, every pointer is garanteed to be valid

Because of that reason it's not possible to initialize pointers to garbage:
```
def ptr = *i32 ---; # error: uninitialized pointer
```

Often you actually don't need a pointer that points to nothing. Though, sometimes when interacting with the OS, C APIs or something of that sort a nullable pointer are needed. Use an [optinal pointer](#Options) on that occasions:
```
def ptr = ?*i32; # valid, initialized to 'null'
```

Optional pointers can be initialized to garbage:
```
def ptr = ?*i32 ---; # valid: initialized to garbage
```

Do not abuse of optional pointers. Only use when strictly necessary.

### Void pointer
[The void type](#The-void-type) is a type that holds no value. So a pointer to a `void` is basically a pointer to nothing, but because always point must point to something, they're a pointer to anything:
```
def a = i32;
def b = f64;
def c = Some_Struct;
def ptr = mut *void &a;
ptr = &b; # valid
ptr = &c; # valid
def ptr_to_some_struct = *Some_Struct ptr; # valid
```

It's not possible to derreference void pointers.

### Pointer comparison
Pointer comparison is possible between pointers of same tipe or void:
```
def a = i8;
def b = i8;
def c = u8;
def p0 = *i8 &a;
def p1 = *u8 &c;
def p2 = *i8 &b;
def p3 = *void &a;
def _ = p0 == p1; # error
def _ = p0 == p2; # valid, _ = false
def _ = p0 == p3; # valid, _ = true
```

## Casting
Stark is strongly typed. You can't pass an u8 to a f32 or even an i8 to an i16. But sometimes casting is needed, on those cases use the `->` operator:
```
def a = 1.5;
def b = a -> i32; # now 'b' is an i32 with a value of 1
```

Chained casting is valid:
```
def a = 1.5;
def b = a -> i32 -> u8; # 'b' is an i8 with value 1
```

The `->` operator can also be used on pointers:
```
def a = i32;
def p0 = &a;
def p1 = p0 -> *i8; # p1 points to 'a' as if it was a '*i8'
```

It's not possible to cast an immutable pointer into a mutable one:
```
def a = i32;
def p0 = &a;
def p1 = p0 -> *mut i8; # error
```

Casting between pointers and integers is possible:
```
def a = i32;
def p0 = &a;
def i0 = p0 -> u64; # valid
def i1 = p0 -> isize; # valid
def i2 = p0 -> i8; # valid
def p1 = i0 -> *i32; # valid
```

Keep in mind that casting between pointers, and specially pointer-integer casting, can be extremely unsafe:
```
def a = 0xdeadbeef;
def p1 = i0 -> *i32;
def b = *p1; # program will crash
```

It's not possible to cast an integer into a mutable pointer:
```
def a = 0xdeadbeef;
def p1 = i0 -> *mut i32; # error
```

Sometimes you don't want to explicitly cast to a type every time. Use the `!` operator as an automatic `-> type` cast to the expected type:
```
def sum : (a, b = i32) i32 => a+b;
def x = sum(12, 1.6!); # equivalent to 'sum(12, 1.6 -> i32)'
```

`!` operator is not valid for pointers:
```
def foo : (ptr = *i32) => ...;
def x = i32 10;
def y = *u8;
foo(x!); # invalid
foo(y!); # invalid
```

## Arrays
Arrays are a buffer of objects of the same time with a compile-time known length.

To define an array open and close square brackets `[]`, in between the square brackets put a positive integer literal or positive integer constant. Follow the brackets by the desired type of the array elements:
```
def arr = [10]i32;
```

All values inside of the array are defaulted if defined that way.

It's possible to use array literals for non-default values:
```
def arr = [1, 2, 3]; # arr[0] == 1, arr[1] == 2, arr[2] == 3. Length of the array is 3
def arr_explicit = [3]i32 [4, 5, 6]; # same thing, but the array type is explicit
```

Another way of writing an array literal is `[amount; default-value]`:
```
def arr = [3; 1.5]; # arr[0] == 1.5, arr[1] == 1.5, arr[2] == 1.5. Length of the array is 3
def arr = [3]f32 [3; 1.5]; # same thing, but the array type is explicit
```

You can also define an array of implicit length based on the literal:
```
def arr = []u64 [1, 2, 3]; # equivalent to [3]u64
```

This is good for explicit typing the elements.

You can also initialize arrays to garbage:
```
def arr = [3]u64 ---;
```

### builtin 'len'
If an array length is needed you can use the builtin `len()`:
```
def arr = [1, 2, 3, 4, 5];
def arr_len = @len(arr);
```

`len()` is actually an [overload set](#Overload-sets), not a function. But specifically the array function overload is a compile-time one, so it's good practice to use the `@` operator.

### Passing arrays as arguments
Work in progress...

## Slices
Work in progress...

## Structs
Work in progress...

Constant members without a value have to be at the top
```
def Int_List = struct(
  T : type is integer,
  buffer = *T,
  capacity, length = u32
);
def numbers = Int_List(u32);
```

## Control flow (if, else and loops)
Work in progress...
### if def
Work in progress...
### Compile-time control flows
Work in progress...

## Options
Work in progress...
### 'or', 'or_brk' and 'or_ret' keywords on options
Work in progress...

## Tuples and error handling
Work in progress...
### 'or', 'or_brk' and 'or_ret' keywords on tuples
Work in progress...

## Type families
Work in progress...

## Precedence table
| Precedence | Operator | Description                                | Associativity |
|------------|----------|--------------------------------------------|---------------|
| 0          | @        | At compile-time operator                   | Left to Right |
| 1          | func()   | Regular function call                      | Left to Right |
| 1          | func x   | One argument function call                 | Left to Right |
| 1          | x func y | Infix function call                        | Left to Right |
| 1          | ++       | Suffix incremeant                          | Left to Right |
| 1          | --       | Suffix Decremeant                          | Left to Right |
| 1          | []       | Indexing                                   | Left to Right |
| 1          | .        | Member access                              | Left to Right |
| 1          | Struct() | Struct literal                             | Left to Right |
| 2          | ++       | Prefix incremeant                          | Right to Left |
| 2          | --       | Prefix incremeant                          | Right to Left |
| 2          | +        | Unary plus                                 | Right to Left |
| 2          | -        | Unary minus                                | Right to Left |
| 2          | !        | Logical not                                | Right to Left |
| 2          | ~        | Bitwise not                                | Right to Left |
| 2          | *        | Pointer derreferencing                     | Right to Left |
| 2          | &        | Address of                                 | Right to Left |
| 2          | &mut     | Mutable address of                         | Right to Left |
| 2          | sizeof   | Size of type                               | Right to Left |
| 2          | alignof  | Alignment of type                          | Right to Left |
| 2          | ->       | Safe cast between types                    | Left to Right |
| 2          | !        | Safe automatic cast between types          | Left to Right |
| 3          | *        | Multiplication                             | Left to Right |
| 3          | /        | Division                                   | Left to Right |
| 3          | %        | Modular                                    | Left to Right |
| 4          | +        | Addition                                   | Left to Right |
| 4          | -        | Subtraction                                | Left to Right |
| 5          | <<       | Bitwise shift left                         | Left to Right |
| 5          | >>       | Bitwise shift right                        | Left to Right |
| 6          | >        | Greater than                               | Left to Right |
| 6          | <        | Less than                                  | Left to Right |
| 6          | >=       | Greater or equals than                     | Left to Right |
| 6          | <=       | Less or equals than                        | Left to Right |
| 7          | ==       | Equals                                     | Left to Right |
| 7          | !=       | Not equal                                  | Left to Right |
| 8          | &        | Bitwise and                                | Left to Right |
| 9          | \|       | Bitwise or                                 | Left to Right |
| 10         | ^        | Bitwise xor                                | Left to Right |
| 11         | &&       | Logical and                                | Left to Right |
| 12         | \|\|     | Logical or                                 | Left to Right |
| 13         | ?:       | Ternary conditional                        | Right to Left |
| 14         | :        | Constant assignment                        | Right to Left |
| 14         | =        | Variable assignment                        | Right to Left |
| 14         | +=       | Variable assignment by sum                 | Right to Left |
| 14         | -=       | Variable assignment by difference          | Right to Left |
| 14         | *=       | Variable assignment by product             | Right to Left |
| 14         | /=       | Variable assignment by quotient            | Right to Left |
| 14         | %=       | Variable assignment by modular             | Right to Left |
| 14         | <<=      | Variable assignment by bitwise left shift  | Right to Left |
| 14         | >>=      | Variable assignment by bitwise right shift | Right to Left |
| 14         | &=       | Variable assignment by bitwise and         | Right to Left |
| 14         | \|=      | Variable assignment by bitwise or          | Right to Left |
| 14         | ^=       | Variable assignment by bitwise xor         | Right to Left |
| 15         | ,        | Comma                                      | Right to Left |

## Overload sets
Work in progress...

## Modules, build system and linking
Work in progress...

## Linking with C libraries
Work in progress...

## Allocator, arenas and Arena_Array
Work in progress...

