# Stark
A language for systems programming.

Current version: 0.1.0

## Hello, World!
```py
def main : () => ext std.io.write("Hello, World!\n");
```

## Philosophy
__Stark__, as it's name suggests, is a language that focus on:
- Robustness
- Explicitness
- No-magic behavior
- Zero overhead

The main focus of the language is performance and type safety.

## Functions
### Function definition and calling conventions
Functions are defined using the `def` keyword, followed by an identifier, followed by the constant assignment operator `:`, then the argument list `(arg0 = type0, arg1 = type1, ...)`, next the return type, followed by the body assignment operator `=>` and finally an expression, that expression is the function body:
```py
def func_name : (arg0 = type0, arg1 = type1, ...) return_type => func_body_expr;
```

Functions always returns the expression at the function body result.

A basic function that returns a sum of two numbers would be defined as:
```py
def sum : (a = i32, b = i32) i32 => a + b;
```

To call a regular function:
```py
def foo : (_a = i32, _b = i32) =>;
foo(10, 12);
```

But if it takes exactly one parameter the parenthesis are optional:
```py
def foo : (_x = i32) =>;
foo 10;
```

And if it takes exactly two parameters it can be called as an 'infix call' `x foo y`, where `x` and `y` are parameters and `foo` is the function:
```py
def foo : (_a = i32, _b = i32) =>;
10 foo 12;
```

Chaining infix function calls is possible (see the [precedence table](#Precedence-table)):
```py
def foo : (_a = i32, _b = i32) =>;
2 foo 4 foo 6; # equivalent to (2 foo 4) foo 6 
```

Trailing commas are allowed:
```py
def foo : (x = i32, y = i32, z = i32,) i32 => x+y+z;
def _ = foo(1, 2, 3,);
```

### Identifier naming rules
Identifiers have to start with an underscore or any letter, followed by an n amount of underscores, letters or numbers:
```py
a # valid identifier
_a # valid identifier
abc__0123 # valid identifier
0a # invalid identifier
```

[The `_` identifier is reserved](#Unused-variables-and-the-_-identifier)

### Unused arguments
Unused arguments causes a compile-time error:
```py
def sum : (a = i32, b = i32, c = i32) i32 => a + b; # error: 'c' is unused
```

To supress the error put an underscore as a prefix on the argument name:
```py
def sum : (a = i32, b = i32, _c = i32) i32 => a + b; # valid
```

### Void return type
For functions with no return value use [void](#The-void-type) as the return type:
```py
def foo : () void =>;
```

`void` is infered if no return type is provided:
```py
def foo : () =>;
```

### Multiple return values
Multiple return values aren't possible, but you can use a [struct](#Structs) instead:
```py
def div : (a, b = i32) (bool, i32) => b == 0 ? (false, 0) : (true, a/b);
def (is_div_by_zero, res) = div(4, 2);
```

### Expression blocks
For functions with more then one expression use an expression block.
```py
def foo : () => {
  expr0;
  expr1;
};
```

If an early break from the expression block is required use the `brk` keyword:
```py
def do_more_stuff : () => {
  do_stuff();
  do_stuff();
  brk;
  do_stuff(); # will not be executed
};
```

By default expression blocks do not return any value. Use the `brk` keyword followed by one expression to break from the expression block with a return value:
```py
def do_stuff_and_sum : (x = i32, y = i32) i32 => {
  do_stuff();
  do_stuff();
  brk x + y;
};
```

Because expression blocks are also just expressions they can be nested:
```py
def foo : () => {
  { };
  { };
};
```

### Break and return from expression blocks
Keep in mind that the `brk` keyword only breaks from the current expression block scope:
```py
def foo : () => {
  {
    brk;
    do_stuff(); # will not be executed
  };
  do_stuff(); # will be executed
};
```

If you want to return from all nested expression blocks at once use the `ret` keyword:
```py
def foo : () => {
  {
    ret;
    do_stuff(); # will not be executed
  };
  do_stuff(); # will not be executed
};
```

And of course, `ret` also can have an expression as a return value:
```py
def five : () i32 => {
  {
    ret 5; # 5 is returned from the function
    do_stuff(); # will not be executed
  };
  do_stuff(); # will not be executed
};
```

The `brk` expression can be nested:
```py
def foo : () => {
  {
    {
      brk brk;
      do_stuff(); # not executed
    };
    do_something(); # not executed
  };
  do_something_else(); # executed
};
```

This isn't a special rule, `brk` is an expression and because `brk` accepts an expression this is achieved for free.

Because of this you can nest it infinitely:
```py
def foo : () => {
  {
    {
      {
        {
          brk brk brk brk;
          do_stuff(); # not executed
        };
        do_something(); # not executed
      };
      do_something_else(); # not executed
    };
    do_something_extra(); # not executed;
  };
  do_real_work(); # executed;
};
```

And of course you can add non-`brk` expressions to it. Again this is not special semantics, it's all the same thing:
```py
def foo : () => {
  def x = {
    {
      brk brk 10;
      do_stuff(); # not executed
    };
    do_something(); # not executed
  };
  use(x); # executed
};
```

The verbosity of too many nested `brk`s is intentional. It indicates that you're doing something wrong in your scope management and your function should be simplified or refactored. Therefore multiple nested `brk`s as well as multiple nested expression blocks aren't recommended.

And of course, `brk` and `ret` only works inside expression-blocks.
```py
def fo : () i32 => ret 10; # error: 'ret' outside expression-block
```

### Defer expression
You can defer an expression to the end of an expression-block using the `defer` keyword. The deferred expression will run even if you used `brk` or `ret` to early return from an expression-block:
```py
def main() => {
  defer println("end");
  println("start");
};
```

The output will be:
```
start
end
```

The order of multiple defers is based on their definitions. The last defined runs first:
```py
def main() => {
  defer println("1");
  defer println("2");
  defer println("3");
};
```

The output will be:
```
3
2
1
```

Because the expression returned by a `brk` runs on the outside block, if a `defer` is returned it'll be deferred to the outside expression-block:
```py
def main() => {
  defer println("1");
  {
    defer println("2");
    prinln("3");
    brk defer println("4");
  };
  defer println("5");
};
```

The output will be:
```
3
2
4
5
1
```

### Variable and constant arguments
Most functions arguments are variables, so they can be defined in the same way (see more on [the variables section](#Variable-definitions)):
```py
def fn0 : (_x = i32) =>;
def fn1 : (_x, _y = i32, _c = f32) =>;
def fn2 : (_x = i32 1) =>; # default values have to be compile-time constants
def fn3 : (_x, _y = i32 1) =>;
def fn4 : (_x, _y = 0, _c, _d = 0.0) =>;
```

The only exception is inferred types by value:
```py
def foo : (_x = 0) =>; #error: argument with no explicit type
```

Because function arguments are variables they also are [immutable](#Variable-mutability) by default. Make them mutable with the `mut` attribute:
```py
def foo : (_x = i32) => _x = 10; # invalid, '_x' is immutable
def foo : (mut _x = i32) => _x = 10; # valid, '_x' is mutable
```

Functions can also have constant arguments:
```py
def foo : (num : u64 0) u64 => num;
```

That means that you can only pass constant parameters to those arguments:
```py
def foo : (num : u64 0) u64 => num;
def x = foo(10); # valid
def y = foo(x); # invalid, x isn't a constant
```

Constant arguments always have to be the first ones in a function:
```py
def foo : (variable = i32, constant : u64) =>; # invalid
def foo : (constant : u64, variable = i32) =>; # valid
```

Constant arguments are the only place where you don't need to specify the constant value on definition (see more on [the constants section](#Constants)):
```py
def foo : (num : u64) u64 => num;
```
So the code above doesn't actually make a type aliases, instead it makes an u64 constant.

Actually constant arguments _can't_ have a default constant value:
```py
def foo : (num : u64 10) u64 => num; # error: constant argument with default value
```

Keep in mind that non-compile-time functions with constant arguments, generates a new function every time they are used with a different constant value through out your program. This can lead to more 'bloated' executables, but the performance is not affected. 

### Compile-time functions
If a function only accept constant parameters and do not access external non-constant data, it's garanteed to run at compile time.
```py
def comp_sum : (a, b : i32) i32 => a + b;
def x = comp_sum(12, 4); # 16 will be computed at compile-time
```

It's actually possible to run any function, that don't access external non-constant data, at compile time if you pass compile-time known values as its arguments and use the `@` operator:
```py
def sum : (a, b = i32) i32 => a + b;
def x = @sum(12, 4); # 16 will be computed at compile-time despite 'sum' accepting variable parameters
```

If a function is only run at compile-time, and no function pointers are taken from it, it'll not be included on the binary. 

You actually can pass a variable to a function ran at compile-time if the function do not read or write to it:
```py
def taking_a_variable_for_some_reason : (_a = u32) =>;
def a = get_some_u32();
@taking_a_variable_for_some_reason(a); # valid, will be run at compile-time (in this case just optimised-out)
```

You generally can't run functions at module scope, using the `@` operator you can:
```py
def foo : () => do_something();

foo(); # invalid
@foo(); # valid

def main : () =>;
```

Keep in mind that the `@` operator means "at compile time". So functions ran at module-scope will be compile-time, and as such have to follow the non-constant data access and only constant parameters rules.

It's also possible to ensure that a function will always be run at compile-time one using `@` as a prefix on the function literal:
```py
def comp_sum : @(a, b = i32) i32 => a + b;
def x = @comp_sum(12, 4); # 16 will be computed at compile-time
```

Ensured-compile-time functions always have to be called via `@`:
```py
def comp_sum : @(a, b = i32) i32 => a + b;
def x = @comp_sum(12, 4); # valid
def x = comp_sum(12, 4); # invalid
```

If any compile-time function rules are broken it'll generate a compile-time error.

Ensured-compile-time function gain access to variables and return values of [compile-time-only types](#Compile-time-only-primitive-types):
```py
def type_of : @(T : imp type, _ = T) type => T;
def a = u32;
def b = @type_of(a) 10; # valid
```

A compile-time function can call another compile-time function inside of it. Compile-time functions aren't some macro-like expansion thing, they literally run at compile-time. So calling a compile-time function inside of a compile-time function have the same behavior of calling a normal function. With that recursion is fine:
```py
def comp_fac : @(x : u32) u32 => x == 0 ? 1 : x * @comp_fac(x-1);
```

The compile-time phase of the compiler is an interpreter of Stark byte-code. That's why this kind of stuff is fine.

### Generic functions
With constant arguments you can make generic functions using the `type` type:
```py
def sum : (T : type, a, b = T) T => a + b;
```

It's also possible to use the `is` (see more on [the class section](#The-is-operator)):
```py
def sum : (T : type is number, a, b = T) T => a + b;
```

### Unordered parameters
Another way to call a function is with directly naming the argument to which you want to pass the parameter:
```py
def foo : (a, b, c = i32) i32 => a+b+c;
def _ = foo(.c = 10, .a = 4, .b = -15);
```

Works even for constant arguments:
```py
def foo : (T : type, a, b, c = T) T => a+b+c;
def _ = foo(.c = 10, .a = 4, .b = -15, .T : f32); # constant parameters use ':' instead of '='
```

This way you can leave the default arguments untouched:
```py
def foo : (a = i32, b = i32 10, c = i32) i32 => a+b+c;
def _ = foo(9, .c = 11);
```

If you use unordered paremeters to just skip an argument it's possible to continue to use the function call order:
```py
def foo : (a = i32, b = i32 10, c, d = i32) i32 => a+b+c;
def _ = foo(9, .c = 11, 12); # a == 9, b == 10, c == 11, d == 12
```

With that in mind if an argument has a default that parameter is optional:
```py
def foo : (a, b = i32, c = i32 10) i32 => a+b+c;
def _ = foo(9, 11);
```

You can only mix named and unnamed paremeters to skip arguments. So if this mix occur, and the named arguments don't actually follow the function order specified on definition, it'll cause an error:
```py
def foo : (a = i32, b = i32 10, c, d = i32) i32 => a+b+c;
def _ = foo(9, .c = 11, .b = 12); # error: invalid order
```

This calling convention is the reason why [function types have their arguments names in it](#Functions-are-just-values).

### Implicit constant parameters
If a constant argument is used in some way by other arguments you can add the `imp` attibute to it. This will make that argument be implicitly defined based on it's first use:
```py
def add : (T : imp type is number, a, b = T) T => a+b;
def x = add(1, 2);
def y = add(1.6, 2.1);
def y = add(1.6, 2); # error: an i32 is being passed to 'b' and expected a f32
```

If a constant argument is marked as `imp` it has to be used on at least one other non-imp argument:
```py
def foo : (T : imp type is number, a, b = i32) i32 => a+b; # error: 'T' has to be used on other arguments
```

You can still set the value of the constant parameter explictly using the unordered parameter function call:
```py
def add : (T : imp type is number, a, b = T) T => a+b;
def x = add(.T : u64, 1, 2);
```

Infix and one-paremeter calls can be used with more than two or one paremeters if the extra ones are constant `imp` arguments:
```py
def add : (T : imp type is number, a, b = T) T => a+b;
def _ = 1 add 2; # valid
def _ = 1.2 add 2.1; # valid
def square : (T : imp type is number, a = T) T => a*a;
def _ = square 5; # valid
def _ = square 5.5; # valid
```

### Immutable arguments are references
Immutable arguments that have a value bigger than a register size (usually 8 or 4 bytes) are passed as references (implicit pointer). That way copies are not made.

So you don't need to explicitly pass immutable pointers around for big structs, like you would do it in C. It is actually more performant if you don't use immutable pointers to just pass values because inlining becomes a possibility:
```py
def foo : (x = *Big_Struct) Something => x.something;
def bar : (x = Big_Struct) Something => x.something;
def big = Big_Struct;
def _ = foo(&big);
def _ = bar(big); # performance for this is the same as 'foo' or better if inlined
```

It is not possible to take the address of immutable arguments because they actually can be references and that could be dangerous:
```py
def foo : (x = Big_Struct) => do_something(&x); # error: can't take address of immutable variable
```

### Implicit address on argument
If the convenience of not taking the address is desired for pointer arguments is desired (for instance, [operator overloading](#Operators-are-overload-sets)), you can add the `&` attribute to a pointer argument. This signals that an implicit address will be taken:
```py
def foo : (_x = &*Big_Struct) =>;
def a = Big_Struct;
foo(a); # equivalent to foo(&a)
```

Because these arguments are just pointers, pointers can still be directly passed:
```py
def foo : (_x = &*Big_Struct) =>;
def a = Big_Struct;
def ptr = &a;
foo(ptr); # valid
foo(&a); # valid
```

Also works for mutable pointers:
```py
def foo : (x = &*mut Big_Struct) => x.something = get_something();
def a = mut Big_Struct;
foo(a); # valid
```

### Variadic arguments
There is no variadic arguments syntax. Most of the time you actually just want to pass a [slice](#Slices) of a single type:
```py
def print_nums : (nums = [..]i32) => for i in 0..<len(nums) => println("%", [1]);
print_nums [1, 2, 3, 4];
```

But if you truly want to accept any type, for a custom printing function or something of that sort, use a slice of [anyrt](#The-anyrt-type). This is how `print` and `println` are implemented:
```py
def custom_print : (fmt = str, args = [..]anyrt) => ...;
custom_print("% % % %", [1, 1.0, "1", [1, 1, 1]]);
```

But only use it if it's really necessary, `anyrt` costs more than just a specified slice.

### Functions are just values
Functions are just values assigned to [constants](#Constants) with function types. So you can create an anonymous function using a function literal and call it directly:
```py
def num = (x, y = i32) i32 => x + y;(10, 20); # num is assigned to 30
```

The type of a function looks something like this:
```py
fn(const_arg0 : type0, const_arg1 : type1, var_arg0 = type0, var_arg1 = type1) return_type
```

Here are some examples with functions and what the type will look like:
```py
def square_i32 : (x = i32) i32            => x*x;
def square_any : (T : type is number, x = T) T    => x*x;
def sum_i32    : (x, y = i32) i32         => x+y;
def sum_any    : (T : type is number, x, y = T) T => x+y;
def set_any_x  : (T : type, foo = T)      => foo.x = something();
# square_i32 type: fn(x = i32) i32
# square_any type: fn(T : type is number, x = T) T
# sum_i32 type:    fn(x = i32, y = i32) i32
# sum_any type:    fn(T : type is number, x = T, y = T) T
# set_any_x type:  fn(T : type is number, x = T, y = T) void
```

With that in mind you can define a function without [type inference](#Type-inference):
```py
def sum_any : fn(T : type is number, x = T, y = T) T (T : type is number, x, y = T) T => x+y;
```

But this is extremely verbose.

[Argument names have semantic meaning](#Unordered-parameters). That's why they are included in function types.

Because of the 'functions are just values' mindset nested functions are simple:
```py
def foo : () => {
  def bar : (x, y = i32) i32 => x + y;
};
```

Function literals assigned to constants are considered 'named functions'. This is basicaly taking adventage of [constant scoping](#Constant-scoping) and not actually a seperate feature. I.e. recursion is possible:
```py
def foo : (x = i32) => foo(x + 1); # completely valid
```

### Function pointers
Variables can't have the function types, only constants. For variables use function pointers instead.

To assign a function pointer to a variable you take its address:
```py
def foo : () =>;
def a = &foo;
```

This would be an error:
```py
def foo : () =>;
def a = foo; # error: trying to assign function type to variable
```

Function literals can be assigned directly to variables if you take its address:
```py
def a = &(x = i32) i32 => x*x;
```

The code above is basically creating an anonymous function and assigning its address to the variable.

You can't use recursion on functions assigned directly to variables because they're anonymous:
```py
def a = &(x = i32) i32 => a(x)*x; # error: a is undefined
```

Function pointers can't have constant arguments. To get a function pointer from a function with constant arguments you have to do the following:
```py
def square : (T : type is number, x = T) T => x*x;
def ptr_to_square_i32 = &square(i32);
def ptr_to_square_f32 = &square(f32); # different pointer from 'ptr_to_square_i32'
```

The type of a function pointer looks like this:
```py
*fn(name0 = type0, name1 = type1, ...) return_type # function pointer
```

Obviously is not possible to take the mutable address of a function, so mutable function pointers aren't a thing.

### Passing functions as arguments
There are basically three ways of passing functions as arguments:
1. Function pointers
2. Constant arguments with function type
3. Constant arguments with [overload](#Overloads) type
```py
def some_overload : [a = number](a, a) a;
def foo : (fn = *(i32, i32) i32, x, y = i32) i32 => fn(x, y);
def bar : (fn : function, x, y = i32) i32 => fn(x, y);
def bar : (fn : some_overload, x, y = i32) i32 => fn(x, y);
```

The function pointer one will have memory, so it can be assigned to things.

The overload one will accept all functions that are part of that overload_set.

And the function type one will accept any function.

## Constants
Constants are immutable compile-time known values.

Keep in mind that everything that is named on the language is either a [variable](#Variables) or a constant.

So named functions are constants, named structs are constants, named modules are constants and so on.

### Constant definitions
To define a constant use the `def` keyword followed by an identifier and the constant assignment operator `:`. After that a compile-time known value must be provided:
```py
def thirty : 30;
```

### Type inference
You can optionally provide a type explicitly to a constant definition:
```py
def thirty : i32 30;
```

For more info into what type a literal infer check the [inference table](#Type-inference-by-value-table).

### Type aliases
Differently from [variables](#Assignment-after-definition), constants can't be assigned after definition. If you define a constant to be equal to a type, you'll instead create a type aliases:
```py
def int : i32; # int is a type alias of i32
def foo (x = int) int => x*x+x;
def my_i32 = i32 foo 10; # valid
```

the type of a type alias is... `type`. So you can define it explicitly:
```py
def int : type i32; # int is a type alias of i32
```

You can also create distinct type aliases using the `type()` constructor: 
```py
def int : type(i32); # int is its own distinct type
def foo (x = int) int => x*x+x;
def my_i32 = i32 foo 10; # invalid, int != i32
```

### Multiple constants definitions at once
It's also possible to define multiple constants at once separating them by commas:
```py
def FOO, BAR : 10; # both will have anyi type with 10 as the value
```

With this a function can have multiple names:
```py
def foo, foo_alternative : (x = i32) i32 => x*x+x; # Both foo and foo_alternative will have the same function address
def foo_alternative2 : foo; # This also generates a constant with the same function address
```

### Constant scoping
When a constant is defined it'll become available to all the current scope and all scopes inside the current scope. So forward declaring is possible:
```py
def bar_squared : () i32 => BAR*BAR;
def BAR : 10;
```

This is particularly good for functions:
```py
def square_and_sum : (x, y = i32) i32 => square x + square y;
def square : (x = i32) i32 => x*x;
```

### Constant shadowing
It's possible to shadow a constant in nested scopes:
```py
def TEN : 10;
def foo : () => {
  def TEN : 20; # for this scope only 'TEN' will be 20
};
def TEN_AGAIN : TEN; # 'TEN_AGAIN' will be 10
```

You can't shadow a constant in the same scope:
```py
def TEN : 10;
def TEN : 20; # invalid, 'TEN' is already defined
```

### Unused constants
Similarly to [unused arguments](#Unused-arguments), unused constants also throw compile errors:
```py
def FOO : 10; # error: 'FOO' is never used
```

You solve this in the same way unused arguments did: add an underscore at the start of the identifier:
```py
def _FOO : 10; # valid
```

## Variables
Variables are very similar to [constants](#Constants). The main difference is that variables are runtime values.

### Variable definitions
To define a constant use the `def` keyword followed by an identifier and the variable assignment operator `=`. It's similar to constants, infact you can define it in the same way constants can be defined:
```py
def x = 1; # inferred integer
def y = i32 2; # explicit type
def a, b, c = 345; # Multiple variables inferred as the same type
def e, f, g = i32 678; # Multiple variables defined with an explicit type
```

The first main difference between variable and constant definitions is type aliasing. You can't make a variable type alias.

This:
```py
def x = i32;
```
Will actually create a variable 'x' with type 'i32' initialized to 0.

This syntax also works for multiple variable definitions at once:
```py
def x, y = i32; # both x and y are of type 'i32' and initialized to 0
```

### Default initialization
When you define a variable without giving it a default value it'll always default to 0 ([or whatever the default value is](#Default-values)).

If you want to garbage initialize a variable, for performance or whatever reasons, you can do so explicitly:
```py
def x = i32 ---; # defaults to stack garbage
```

### Variable mutability
All variables are immutable by default and cannot be assigned to after its definition.
```py
def x = 10;
x = 20; # invalid, x is immutable
```

Forward assigning is possible. But only if the variable is not read from before it:
```py
def x = u32;
x = 20; # valid
def y = u32;
def z = y;
y = 10; # invalid, 'y' is immutable
```

If you're forward assigning with an `if` expression, the variable has to be assigned in a conditioless else:
```py
def x = u32;
if something => x = 20;
else => x = 0; # valid
def y = u32;
if something => y = 20;
# error: 'y' may be unassigned
```

If mutability is a needed factor for a variable use the `mut` attribute on its definition:
```py
def x = mut 10;
x = 20; # valid, x is mutable
def y = mut i32;
y = 10; # valid, y is mutable
```

### Variable assignment
A variable assignment is also an expression, it returns `void`. But if you wrap the assignment in parenthesis then it actually returns the value of the assigned variable after it's definition:
```py
def x = mut i32 get_x();
def y = (x = x * 2 + 1);
```

All of the assignment operators works in that way, including `++` and `--`:
```py
def x = mut i32 get_x();
def y = (x++);
```

The prefix `++` and `--` only differ with the parethesis assignment syntax:
```py
def x = mut get_x();
x++; # exatcly the same as ++x
++x; # exatcly the same as x++
def y = (x++); # assign x to y then increment x
def y = (++x); # increment x then assign x to y
```

### Variable aliasing
You can create an alias to a variable. Simply start to define a new variable an it's value has to be `var` followed by a valid variable:
```py
def x = mut i32;
def y = var x;
def _ = &y == &x; # true, both are variables of the same type with the same address
```

`y` will not be a pointer to `x`. `x` and `y` literally become the same thing.

### Unused variables and the '_' identifier
It's exactly the same situation as [unused constants](#Unused-constants) or [unused arguments](#Unused-arguments):
```py
def x = 10; # error: 'x' is never used
def _x = 10; # valid
```

The `_` identifier is a special one. It can be defined numerous times inside the same scope and it's not possible to read its value:
```py
def _ = 10;
def _ = 20;
def x = _; # error, '_' is not defined
```

The value inside `_` is immediataly thrown way and not stored in memory.

This is particularly useful for functions. Function return values _have_ to be used, so this is a way to signal that the value does not matter:
```py
def five : () i32 => 5;
five(); # error: unhandled return value
def _ = five(); # result is thrown away
```

You can also name function arguments as '_', this is particularly great for values that you don't care about in callbacks:
```py
def some_callback : (a, b = u32, _ = u32, _ = f32) u32 => a + b;
```

It's also possible to define `_` as a constant as well. This is useful for getting rid of compile-time functions return values at [module scope](#Module-scope):
```py
def foo : () i32 => return_some_i32();

@foo(); # invalid, unused return value
def _ : @foo(); # valid

def main : () =>;
```

Technically you can do the same thing using just the variable `_` instead of the constant one. But this is good if in the future you want this value to be a constant:
```py
def _ : @foo(); # before
def easy_change : @foo(); # after
```

### Variable scoping
it works differently than [constant scoping](#Constant-scoping). A variable cannot be forward declared.

### Variable shadowing
Exactly the same as [constant shadowing](Constant-shadowing).

### Global variables
variables can be defined on [module scope](#Module-scope). Their default values must be constant and they can't be assigned on module scope, even forward assignment isn't allowed:
```py
def x = i32;
x = 10; # error: assignment on module scope
def main : () =>;
```

You can reference variables on module scope for assignment of other variables or constants:
```py
def x = i32 10;
def y = x; # valid
def FOO : x; # valid
def main : () =>;
```

But if the variable is mutable you can't:
```py
def x = mut i32 10;
def y = x; # error: 'x' isn't a constant or an immutable variable
def FOO : x; # error: 'x' isn't a constant or an immutable variable
def main : () =>;
```

## Primitive types
There are two types of primitive types, the ones that can be assigned to runtime variables and the ones that dont.

### Normal runtime primitive types
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

### Compile-time-only primitive types
None of the compile-time-only primitives have a size
- __Any integer:__ `anyi`
- __Any floating point:__ `anyf`
- __Variable type:__ `type`
- __Ranges:__ `range`
- __Classes:__ `class`
- __Spaces__: `spc`
- __Foreign functions__: `unkfn`
- __Variables aliases__: `var`

This types are valid to use in constants and [ensured-compile-time functions](#Compile-time-functions).

### Signed integers
Signed integers are represented using [two's complement](https://en.wikipedia.org/wiki/Two%27s_complement).

### The void type
The `void` type is a special one. It's basically a type that doesn't hold any value.

It's useful for returning nothing from a function or [making a pointer that points to anything](#Void-pointer).

### Strings
Strings are UTF-8 enconded non-null-terminated.

The layout of a string in memory (illustrated as a struct) looks like this:
```py
def str : (
  data        = *u8,
  length      = u32,
  bytes_count = u32,
);
```
Where `length` is the amount of characters on the string and `bytes_count` is the actual amount of bytes needed by the string.

Strings don't actually hold any data. They're equivalent to string views in other languages.

C strings are similar, but they don't have a `bytes_length` field, they're ASCII encoded and null-terminated.

#### Raw string literals
Theres a way to transform a series of tokens into string literals directly: use `$` followed by an identifier, then you need to write the same identifier again. Everything in between the identifiers will become part of the string, the first and last white spaces do not count:
```py
def _  = $end hello, world! end;
```

The above raw string literal is equivalent to:
```py
def _ = "hello, world!";
```

Because the string only ends when the specified identifier is reached you don't need any escaping:
```py
def _ = $end
My name is: "John Doe"
I'm '20' years old
end
```

The above raw string literal is equivalent to:
```py
def _  = "My name is: \"John Doe\"\nI'm '20' years old"
```

You can add constant binds to the raw string. In between the `$` and the identifier add parenthesis, inside the parenthesis you can create constants that works on the string. To reference the constant binding put the specified identifier in between `$`:
```py
def _ = $(NAME : "John Doe", AGE : 20) end
His name is $NAME$. $NAME$ is $AGE$ years old.
$AGE$ is a good age. "$NAME$" is a good name.
end
```

The above raw string literal is equivalent to:
```py
def _ = "his name is John Doe. John Doe is 20 years old.\n20 is a good age. \"John Doe\" is a good name";
```

You can bind any constant into the raw string constants:
```py
def TEN : 10;
def _ = $(X : TEN) end $X$ $X$ $X$ end; # equivalent to "10 10 10"
```

If you want the constant bind and the constant itself to share the same name you can do this as a shortcut:
```py
def TEN : 10;
def _ = $(TEN, ELEVEN : 11) end $TEN$ $TEN$ $TEN$ $ELEVEN$ end; # equivalent to "10 10 10 11"
```

You can also bind every constant defined on _the current scope_ using `(*)`:
```py
def TEN : 11;
def FOO : "foo";
def _ = $(*) end $TEN$_$FOO$ end; # equivalent to "10_foo"
```

Remember `(*)` only works for definitions on the current scope:
```py
def TEN : 10;
{
  def FOO : "foo"; 
  def _ = $(*) end $TEN$_$FOO$ end; # equivalent to "$TEN$_foo"
}
```

Use `(*)` only in tight packed scopes because the risk of an override of an actual word and a predefined constant is high:
```py
def hello : "world";
def _ = $(*) end $hello$, $world$! end; # equivalent to "world, $world$!"
```

This can be a little dumb with this example, but it can be dangerous if you actually want a thing warpped around `$`s to be part of the string.

### Booleans
Booleans are really simple, they just can have two states: `true` or `false`
- `true`: Equivalent to 1 in integers
- `false`: Equivalent to 0 in integers

### Ranges
Ranges are a range between two integers: the starting index and the ending index. The starting index always have to be less then or equal to the ending index:
```py
0..4;
9..9;
30..20; # error: starting index has to be smaller then or equal to the ending index
```

Ranges with the `..` syntax are inclusive. This means that the ending index is included if the ranges is used on a [for loop](#For-loop) or [slice](#Slices).

For an exclusive range use `..<`. Exclusive ranges can't haver the starting index and ending index being the same value:
```py
```py
0..<4;
9..<9; # error: starting index has to be smaller then the ending index
30..<20; # error: starting index has to be smaller then the ending index
```

Ranges can be assigned to constants:
```py
def RANGE : 2..5;
```

You can access the starting index and the ending index of range with the `.start` and `.end` members:
```py
def RANGE : 2..5;
def starting_index = RANGE.start;
def ending_index = RANGE.end;
```

### Type inference by value table
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
| 'a'              | uchar literal    | uchar                               | uchar                                                             |
| c'a'             | achar literal    | achar                               | achar                                                             |
| "text"           | String literal   | str                                 | str                                                               |
| c"text"          | C string literal | cstr                                | cstr                                                              |
| i32, f32, Struct | Types directly   | type                                | The typed 'type' (i32, f32, etc) with a default value             |
| \[1, 2, 3\]      | Array literal    | \[amount-of-elements\]inferred      | \[amount-of-elements\]inferred                                    |
| \[3\]21          | Array literal    | \[amount-of-elements\]inferred      | \[amount-of-elements\]inferred                                    |
| $END END         | Raw string lit   | str                                 | str                                                               |
| 1..9             | Range literal    | range                               | range (Compile-time only)                                         |
| 1..<9            | Range literal    | range                               | range (Compile-time only)                                         |

__Caveats and Notes:__
* Passing constants with `anyi` type into mismatched integers will cause an error. E.g. constant with value 1234 passed to an `u8`
* Binary, octal and hexdecimal literals can't be negative. But they can be assigned to signed integers.
* An array type will always be based on the inferred type of the first element. E.g. `array = [1, 2, 3]` == `array = [3]inferred-type-of-array[0]`
* Integer literals can be separated by underscores for readability. E.g. `1_000_000`

### Default values
Every type has it's own set of default values:
- _Integers:_ Defaults to 0
- _Floating points:_ Defaults to 0.0
- _Strings:_ Defaults to ""
- _Structs:_ Defaults to their members defaults
- _Tuples:_ Defaults to their members defaults
- _Unions:_ Defaults to the specified tag default value
- _Arrays:_ Defaults to the default of their type

## Pointers
You can grab a pointer to a variable using the `&` operator:
```py
def a = 0;
def ptr = &a;
```

The type of a pointer is represented by a `*` followed by the type that the pointer points to:
```py
*i32 # pointer to an i32
*str # pointer to a str
**u8 # pointer to a pointer to an u8
```

### Dereferencing
To dereference a pointer use the `*` operator, similarly to C:
```py
def a = 12;
def ptr = &a;
def b = *ptr; # b = 12
```

Accessing a member from a pointer to a [struct](#Structs) causes an implicit dereference:
```py
def a = Some_Struct(.some_i32 = 12);
def ptr = &a;
def b = ptr.some_i32; # equivalent to 'def b = (*ptr).some_i32;'
```

Accessing the member from a pointer just to get it's address do not cause a dereference:
```py
def a = Some_Struct(.some_i32 = 12);
def ptr = &a;
def ptr_to_some_i32 = &ptr.some_i32; # no dereference
```

### Pointer mutability
Similarly to [variables](#Variable-mutability), pointers are immutable by default.

If you want to assign to the variable the pointers points to a mutable pointer is needed.

Use the `&mut` operator on a variable to take the mutable address from it. You can only make mutable pointers from mutable variables:
```py
def a = 0;
def ptr = &mut a; # invalid, 'a' isn't mutable
def b = mut 0;
def ptr_mut = &mut a; # valid, 'b' is mutable
```

Now assigning to dereferenced values is possible:
```py
def a = mut 0;
def ptr = &a;
*ptr = 10; # error: 'ptr' is not a mutable pointer
def mut_ptr = &mut a;
*mut_ptr = 10; # valid, now 'a' = 10
```

Do not confuse `*mut` with `mut *`. `*mut` is a pointer that can change the value inside the address it's pointing to. `mut *` is a pointer that can change the actual address it points to:
```py
def a = mut i32;
def b = mut i32;
def p0 = *mut i32 &a;
*p0 = 10; # valid, it's a mutable pointer
p0 = &b; # invalid, 'p0' is a immutable variable
def p1 = mut *i32 &a;
*p1 = 10; # invalid, it's an immutable pointer
p1 = &b; # valid, 'p1' is a mutable variable
def p2 = mut *mut i32 &a;
*p2 = 10; # valid, 'p2' is a mutable pointer
p2 = &b; # valid, 'p2' is a mutable variable
```

### Pointer arithimatic
It's not possible to use the `[]` operator to index through a pointer.

But you can achieve something similar with pointer arithimatic:
```py
def nums = [1, 2, 3];
def ptr = &mut something;
def x = *(ptr + 1); # equivalent to nums[1]
```

The only operations possible with pointers are: `+`, `-`, `+=`, `-=`, `++` prefix and suffix, `--` prefix and suffix.

Pointer's do not have bounds checking. It's not even possible because a pointer is just a pointer, no extra metadata.

### Pointer definition
Differently from other variable types, pointers cannot be initialized to nothing. This is because pointers don't have a default value and must always point to memory:
```py
def ptr = *i32; # error: uninitialized pointer
```

This actually enables several advantages compared to pointers that permit something like `NULL` or `nullptr`:
- No need to check for invalid pointers
- Less branching
- Safer, every pointer is garanteed to be valid

Because of that reason it's not possible to initialize pointers to garbage:
```py
def ptr = *i32 ---; # error: uninitialized pointer
```

Often you actually don't need a pointer that points to nothing. Though, sometimes when interacting with the OS, C APIs or something of that sort a nullable pointer is needed. Use an [optinal pointer](#Optional-pointer) on that occasions:
```py
def ptr = ?*i32; # valid, initialized to 'null'
```

Optional pointers can be initialized to garbage:
```py
def ptr = ?*i32 ---; # valid: initialized to garbage
```

Do not abuse of optional pointers. Only use when strictly necessary.

### Void pointer
[The void type](#The-void-type) is a type that holds no value. So a pointer to a `void` is basically a pointer to nothing, but because pointers must always point to something, they're a pointer to anything:
```py
def a = i32;
def b = f64;
def c = Some_Struct;
def ptr = mut *void &a;
ptr = &b; # valid
ptr = &c; # valid
def ptr_to_some_struct = *Some_Struct ptr; # valid
```

It's not possible to dereference void pointers.

### Pointer comparison
Pointer comparison is possible between pointers of same tipe or void:
```py
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

### Taking the address of literals
It's possible to take the address of literals:
```py
def ptr = &10;
```

This is sugar for:
```py
def _a = 10;
def ptr = &_a;
```

So the lifetime of the invisible variable created is the same as the variable it's assigned to.

Mutable addresses are also possible:
```py
def ptr = &mut 10;
```

Sugar for:
```py
def _a = mut 10;
def ptr = &_a;
```

It works for any literal.

## Arrays
Arrays are a buffer of objects of the same type with a compile-time known length.

To define an array open and close square brackets `[]`, in between the square brackets put a positive integer literal or positive integer constant. Follow the brackets by the desired type of the array elements:
```py
def arr = [10]i32;
```

All values inside of the array are defaulted if defined that way.

It's possible to use array literals for non-default values:
```py
def arr = [1, 2, 3]; # arr[0] == 1, arr[1] == 2, arr[2] == 3. Length of the array is 3
def arr_explicit = [3]i32 [4, 5, 6]; # same thing, but the array type is explicit
```

This type of array literal can have a type before it to ensure what the array type will be:
```py
def arr = i32[1, 2, 3];
def arr_explicit = [3]i32 i32[4, 5, 6]; # same thing, but the array type is explicit
```

Another way of writing an array literal is `[amount]default-value`:
```py
def arr = [3]1.5; # arr[0] == 1.5, arr[1] == 1.5, arr[2] == 1.5. Length of the array is 3
def arr = [3]f32 [3]1.5; # same thing, but the array type is explicit
```

You can also define an array of implicit length based on the literal:
```py
def arr = []u64 [1, 2, 3]; # equivalent to [3]u64
```

This is good for explicit typing the elements.

You can also initialize arrays to garbage:
```py
def arr = [3]u64 ---;
```

Array destructuring is possible, this is useful for creating multiple variable with different values:
```py
def [a, b, c] = [1, 2, 3];
def [c, d, e] = i32[1, 2, 3];
def [f, g, h] = [3]1.5;
def arr = [1, 2, 3];
def [i, j, k] = arr;
```

And using the ['_' identifier](#Unused-variables-and-the-_-identifier) for skipping is possible:
```py
def arr = [1, 2, 3];
def [i, j, _] = arr;
```

Mismatch variable definition with the array length isn't allowed:
```py
def arr = [1, 2, 3];
def [i, j] = arr; # error: expected 3 variable definitions, but found only 2
```

Arrays can have 0 length. The array will actually occupy 0 bytes, in other words, it does not exists in memory.

### Array mutability
Because arrays actually store the data for modifying the values you just need to set the array as `mut`:
```py
def arr0 = mut [1, 2, 3];
arr0[0] = 4; # valid
def arr1 = [1, 2, 3];
arr1[0] = 4; # invalid, array is mutable
```

### builtin 'len'
If an array length is needed you can use the builtin `len()`:
```py
def arr = [1, 2, 3, 4, 5];
def arr_len = @len(arr);
```

`len()` is actually an [overload](#Overloads), not a function. But specifically the array function overload is an ensured-compile-time one, so the `@` operator is necessary.

### Passing arrays as parameters
Because array values must have known compile-time length they can't be passed directly to function variable parameters.

There are four ways of passing an array as parameters:
1. Same length arrays
2. Pointers
3. Constant arguments
4. [Slices](#Slices)

#### Passing same length arrays
You technically can pass arrays as parameters if you specify the length:
```py
def foo(_arr = [3]u32) =>;
def arr0 = [1, 2, 3];
def arr1 = [1, 2, 3, 4];
foo(arr0); # valid
foo(arr1); # error: expected [3]u32 found [4]u32
```

Following the [passing as reference rule](#Immutable-arguments-are-references) the array will not be copied.

To avoid several performance and semantic problems, arrays passed directly can't be mutable:
```py
def foo(_arr = mut [3]u32) =>; # error: array argument can't be mutable
```

Insuring that arrays of the same length as passed to arguments can be useful sometimes. But most of the time a function wants to accept any array length, so this method isn't good for that.

#### Passing arrays as pointers
Arrays do not decay into pointers like in C, so this is invalid:
```py
def foo(_arr = *u32) =>;
def nums = [1, 2, 3, 4];
foo(nums); # error: expected *u32 found [3]u32
```

Taking the address of just the array will give you a pointer to the array type:
```py
def foo(_arr = *u32) =>;
def nums = [1, 2, 3, 4];
foo(&nums); # error: expected *u32 found *[3]u32
```

You could use casting to resolve this problem:
```py
def foo(_arr = *u32) =>;
def nums = [1, 2, 3, 4];
foo(&nums -> *u32); # valid
```

Or preferably take the address of the first element:
```py
def foo(_arr = *u32) =>;
def nums = [1, 2, 3, 4];
foo(&nums[0]); # valid
```

The problem with passing arrays as pointers is the lost of the array length.

To overcome this you could pass an extra argument to the function:
```py
def foo(_arr = *u32, _arr_len = usize) =>;
def nums = [1, 2, 3, 4];
foo(&nums[0], @len(nums)); # valid
```

It'll work, but this is extremely unsafe and cumbersome. Therefore, not recommended

#### Passing array to constant arguments
You can pass arrays of any length to constant arguments:
```py
def foo(_arr : []u32) =>;
def nums = [1, 2, 3, 4];
foo(nums); # valid
```

This is better than passing by pointer. The problem is that for every new array length that is passed a new version of the function is created. It's a viable option though, the performance will actually be great with compile-time catches on bound checks and compile-time known length.

You can also enforce same length on different array parameters using the [imp](#Implicit-constant-parameters) attribute:
```py
def foo(N : imp usize, _arr0 : [N]u32, _arr1 : [N]u32) =>;
def nums0 = [1, 2];
def nums1 = [1, 2];
def nums2 = [1, 2, 3];
foo(nums0, nums1); # valid
foo(nums0, nums2); # invalid
```

## Slices
Slices are views into arrays. They consist of a length and a pointer.

To slice an an array use a [range](#Ranges) inside the index `[]`:
```py
def arr = [6, 5, 4, 3, 2, 1];
def slice = arr[2..4]; # slice[0] == 4, slice[1] == 3, slice[2] == 2, len(slice) == 3
```

If you want to make a slice from the beginning to the end of the array use an empty range:
```py
def arr = [1, 2, 3];
def slice = arr[..];
```

Or just set only the starting index or ending index:
```py
def arr = [1, 2, 3, 4, 5, 6];
def slice0 = arr[..3]; # 0 starting index is implied
def slice1 = arr[2..]; # @len(arr)-1 ending index is implied
```

You can use variables when slicing, but keep in mind that does have some runtime overhead for bounds checking:
```py
def arr = [1, 2, 3, 4, 5, 6];
def n = get_some_i32(); # supposed this returns 3 for some reason
def slice = arr[..n]; # slice[0] == 1, slice[1] == 2, slice[2] == 3, len(slice) == 3
```

The type of a slice is `[..]<type>`, so use it to create a slice with an explicit type:
```py
def arr = [1, 2, 3];
def slice0 = [..]u32 arr[..];
def slice1 = [..]i32 arr[..]; # error: can't make slice u32 array into i32 
```

With explicit typed slices you can pass the array directly:
```py
def arr = [1, 2, 3];
def slice0 = [..]u32 arr; # valid
```

Similarly to pointers (because slices are basically just pointers), they can't be null and always have to point to some array:
```py
def slice 0 = [..]i32; # invalid
```

### Passing array as slice
This concludes the fourth method of [passing an array as a paremeter](#Passing-arrays-as-parameters). You simply just need to pass the array as an slice. This is the most preferred method for general use cases:
```py
def foo(_arr = [..]u32) =>;
def nums = [1, 2, 3, 4];
foo(nums); # valid
```

### Array literals on slices
Array literals can be sliced directly:
```py
def slice = [..]u32 [1, 2, 3]; # makes slice from array literal
```

The slice still doesn't hold the actual data of the array. What this actually does is create an implicit array that will live through the current stack frame.

This can be useful for some functions:
```py
def sum : (xs = [..]i32) i32 => {
  def res = 0;
  for x in xs => res += x;
  ret res;
};
def x = sum([1, 2, 3, 4]); # x = 10
def y = sum[1, 2, 3, 4]; # using the one paremeter call convention
```

### Getting the length of a slice 
Slices have a function in the builtin `len()` overload. It's not compile-time, so `@` isn't needed:
```py
def arr = [1, 2, 3];
def slice = arr[..];
def amount = len(slice);
```

### Slice mutability
The slices mutability is more akin to pointers than arrays. If you wan't to modify the values of an array through a slice you need a mutable slice using `[..]mut`:
```py
def slice0 = [..]mut u32 [1, 2, 3];
slice0[1] = 4; # valid
def slice1 = [..]u32 [1, 2, 3];
slice1[1] = 4; # invalid
```

If the slice variable itself is marked as mutable, it means you can change the slice not the values:
```py
def slice = mut [..]u32 [1, 2, 3];
slice = [4, 5, 6]; # valid, new array assigned to slice
slice[1] = 4; # invalid
```

## Structs
The definition of a struct is very similar to the [definition of a function](#Function-definition-and-calling-conventions). The difference is that you do not use the body assignemnt `=>`:
```py
def Some_Struct : (member0 = type0, member1 = type1, member2 = type2, ...);
```

It can facilitate to think of structs as "bodyless functions" because a lot of the arguments rules apply for them. But there are some differences.

Members of a struct are, mostly, variables. So all of this declarations are valid:
```py
def Some_Struct = (
  a = u32, # explicit type with default initialization (0 in this case)
  b, c = u32, # multi-member definition
  d = u32 1, # explicit type with value
  e, f, g = u32 2, # multi-member definition with explicit type and vlue
  h, i = 4, # multi-member definition with inferred type by value
  j = u32 ---, # garbage initialization
);
```

Similarly to function arguments, the only exception is inferred types by value:
```py
def Some_Struct = (
  a = 3, # error: member with no explicit type
);
```
```py
```

### Struct instantiation
Regular struct instantion is also similar to [regular function calls](#Function-definition-and-calling-conventions):
```py
def Some_Struct : (x, y = u32);
def some_struct_instance = Some_Struct(10, 20);
```

Infix-call and one parameter like calls aren't valid on struct instantiation though:
```py
def Foo : (x = u32);
def Bar : (x, y = u32);
def a = Foo 10; # invalid
def b = 10 Foo 20; # invalid
```

And of course, you can explicitly type an instance:
```py
def Foo : (x = u32);
def a = Foo Foo(10);
```

An instance with no value will be initialized with all the default values specified on the struct:
```py
def Foo : (x = u32, y = u32 6);
def a = Foo; # a is created with default values: 'a.x = 0' and 'a.y = 6'
```

Named members instantiation is also possible and follow the same rules as the [function ones](#Unordered-parameters):
```py
def Foo : (a = i32, b = i32 10, c, d = i32);
def _ = Foo(.c = 10, .a = 4, .b = -15, .d = 4);
def _ = Foo(9, .c = 11, .d = 12);
def _ = Foo(9, .c = 11, 12); # a == 9, b == 10, c == 11, d == 12
def _ = Foo(9, .c = 11, .b = 12); # error: invalid order
```

You can leave out any member at instantiation and it'll use it's default value:
```py
def Foo : (a = i32, b = i32 10, c, d = i32);
def _ = Foo(9); # a == 9, b == 10, c == 0, d == 0
def _ = Foo(.d = 12); # a == 0, b == 10, c == 0, d == 12
def _ = Foo(9, .c = 11); # a == 9, b == 10, c == 11, d == 0
```

Passing no arguments to the initialization will use initialize all of the members to their proper default values:
```py
def Foo : (a = i32, b = i32 10, c, d = i32);
def _ = Foo(); # a == 0, b == 10, c == 0, d == 0
```

It's similar to just using the type explicitly without no initialization:
```py
def Foo : (a = i32, b = i32 10, c, d = i32);
def _ = Foo(); # a == 0, b == 10, c == 0, d == 0
def _ = Foo; # a == 0, b == 10, c == 0, d == 0
```

The difference is that you're explicitly assigning, so forward assigning is not an option:
```py
def Foo : (a = i32, b = i32 10, c, d = i32);
def a = Foo(); # a == 0, b == 10, c == 0, d == 0
a = Foo(1, 2, 3, 4); # error: 'a' is immutable
```

#### Implicit struct name on instantiation
If a variable already has the type of a struct you can initialize it with the parenthesis directly, without the struct name:
```py
def Vec2 : (x, y = f32);
def a = Vec2;
a = (1.2, 2.1); # x == 1.2, y == 2.1
```

Works on function return values:
```py
def Vec2 : (x, y = f32);
def addv : (a, b = Vec2) Vec2 => (a.x+b.y, a.y+b.y);
```

And on paremeters:
```py
def Vec2 : (x, y = f32);
def addv : (a, b = Vec2) Vec2 => (a.x+b.y, a.y+b.y);
def _ = addv((1.1, 2.2), (3.3, 4.4)); # valid
def _ = addv((), (3.3)); # valid
def _ = addv((.y = 1.0, .x = 2.0), (3.0, 4.0)); # valid
```

This is particularly useful with [one-parem function calls](#Function-definition-and-calling-conventions):
```py
def Vec2 : (x, y = f32);
def double : (a = Vec2) Vec2 => (a.x*2, a.y*2);
def _ = double(1.0, 2.0); # valid
```

#### Destructed struct instantiation
You actually can instantiate a struct with all of it's members being actual variables in the main scope:
```py
def Vec2 : (x, y = f32);
def (x, y) = Vec2(1.0, 2.0);
```

The variables created can have any name you want:
```py
def (a, b) = Vec2(1.0, 2.0);
```

You can skip fields using the ['_' identifier](#Unused-variables-and-the-_-identifier):
```py
def Foo : (a, b, c = i32);
def (_, b, _) = Foo(1, 2, 3);
```

Mismatch variable definition isn't allowed:
```py
def Foo : (a, b, c = i32);
def (a, b) = Foo(1, 2, 3); # error: expected 3 variable definitions, but found only 2
```

### Unnamed members
Struct members can actually not have names:
```py
def Some_Struct : (u32, named_member = f32);
```

If that's the case you access the unnamed member via the order of unnamed member definition number, starting at 0. Named members do not count toward unnamed index positions:
```py
def Some_Struct : (u32, named_member = f32, i32); # struct has '.0' as u32 and '.1' as i32
def foo = Some_Struct(1, 2.0, -3);
def a = foo.0; # a == 1u32
def b = foo.1; # b == -3i32
def c = foo.2; # error: '.2' is not a member of 'Some_Struct'
```

#### Tuples
If a struct only has unnamed variable members, this is equivalent to tuples in other languages. Because of this, these structs are also refered as tuples through out the specification:
```py
Some_Tuple : (u32, f32, i32);
def foo = Some_Tuple(1, 2.0, -3);
def a = foo.0;
def b = foo.1;
def c = foo.2;
def (x, y, z) = Some_Tuple(4, 5.0, -6);
```

To define a tuple-like struct with only one member, a trailing comma is necessary:
```py
def Foo : (u32); # 'Foo' is an alias to 'u32'
def Bar : (u32,); # 'Bar' is a struct with one unnamed member
```

### Struct typing
Struct types are always distinct, even if the underlying data is the same:
```py
def Foo0 : (u32, u32);
def Foo1 : (u32, u32);
def a = Foo0(1, 2);
def b = Foo1 a; # error: 'a' isn't of the type 'Foo1'
```

See the [casting](#Casting) section for workarounds.

### Struct mutability
Forward assigning is still possible with immutable structs:
```py
def Foo : (x = u32, y = u32 6);
def a = Foo; # not initialized yet
a = Foo(10, 20); # valid, initialized here
```

For the forward assigning to work, not only the whole struct cannot be read, but members as well:
```py
def Foo : (x = u32, y = u32 6);
def a = Foo; # not initialized yet
def b = a.x; # 'b = 0'
a = Foo(10, 20); # invalid, 'a' is already initialized
```

Forward declaring works only for the whole struct, if you try to assign to a member the struct is assumed to already be properly initialiazed:
```py
def Foo : (x = u32, y = u32 6);
def a = Foo;
a.x = 10; # invalid, 'a' is immutable
```

For this to be possible, use a mutable instance:
```py
def Foo : (x = u32, y = u32 6);
def a = mut Foo; # initializes struct to 'x = 0' and 'y = 6'
a.x = 10; # invalid, 'a' is immutable
```

As most things in stark structs are anonymous. So it is possible to use the struct literal directly on a variable type, argument type, etc:
```py
def foo = (x, y, z = i32); # Foo has the type '(x, y, z = i32)'
foo = (1, 2, 3); # valid
```

You can use the initialization on the struct literal directly:
```py
def foo = (x, y, z = i32)(.z = 1, .x = 2, .y = 3); # valid
```

### 'imm' and 'renege':
Members mutabilities are based on the instance mutability. You can't use the `mut` attribute on field members:
```py
def Some_Struct : (
  x = mut u32, # error: members can't use the 'mut' attribute
);
```

But if you want to enforce _immutability_ there is the `imm` attribute. This attribute can only be used on struct members:
```py
def Some_Struct : (x = i32, y = imm u32);
def a = mut Some_Struct;
a.x = 10; # valid, 'a' is mutable
a.y = 20; # error: 'a.y' field is immutable
```

To be able to modify the field member use the `renege` keyword:
```py
def Some_Struct : (x = i32, y = imm u32);
def a = mut Some_Struct;
renege a.y = 20; # valid
```

The `renege` will only work on mutable instances:
```py
def Some_Struct : (x = i32, y = imm u32);
def a = Some_Struct;
renege a.y = 20; # error: 'a' is immutable
```

### Constant members
Constant members are extremely similar to [functions constant arguments](#Variable-and-constant-arguments): They have to come before any variable member and they can't have default values:
```py
def Vec2_0 : (x, y = T, T : type); # invalid
def Vec2_1 : (T : type u32, x, y = T); # invalid
def Vec2_2 : (T : type, y = T); # valid
```

Instantiating a struct with a constant member follow all the previous rules established:
```py
def Foo : (T : type is number, a = T, b = T 10, c, d = T);
def _ = Foo(.c = 10, .a = 4, .b = -15, .d = 4, .T = i32);
def _ = Foo(i32, 9, .c = 11, .d = 12);
def _ = Foo(i32, 9, .c = 11, 12); # a == 9, b == 10, c == 11, d == 12
def _ = Foo(i32, 9, .c = 11, .b = 12); # error: invalid order
def _ = Foo(i32); # a == 0, b == 10, c == 0, d == 0
def _ = Foo(i32, .d = 12); # a == 0, b == 10, c == 0, d == 12
def _ = Foo(i32, 9, .c = 11); # a == 9, b == 10, c == 11, d == 0
```

The `imp` attribute can be used on constant members:
```py
def Vec2 : (T : imp type is number, x, y = T);
def _ = Vec2(1, 2); # T = i32
def _ = Vec2(1.2, 2.1); # T = f32
```

And follow the same rules as `imp` arguments:
```py
def Vec2 : (T : imp type is number, x, y = f32); # error: 'T' needs to be used in other members
```

The type of a variable with a constant members is `Struct(<constant-arguments>)`, this is called a 'struct variant':
```py
def Vec2 : (T : imp type is number, x, y = T);
def a = Vec2(1, 2); # type_of(a) == Vec2(i32)
def b = Vec2(1.2, 2.1); # type_of(b) == Vec2(f32)
def c = Vec2(i32) Vec2(3, 4); # Explicitly typed
```

Structs with constant members without the `imp` attribute can't be forward assigned because of this:
```py
def Vec2 : (T : type is number, x, y = T);
def a = Vec2(i32); # 'a' type is actually being inferred by the initializer 'Vec2(i32)'
def b = Vec2(i32) Vec2(i32); # equivalent to 'a' but explicitly typed.
a = Vec2(i32, 1, 2); # error: 'a' is immutable
```

With the `imp` attribute is fine though:
```py
def Vec2 : (T : imp type is number, x, y = T);
def a = Vec2(i32);
a = Vec2(1, 2); # valid
```

Structs with `imp` constant members cannot be empty initialized:
```py
def Vec2 : (T : imp type is number, x, y = T);
def a = Vec2(); # error: 'T' needs to be assigned
```

Assign the constant member explicitly if this is desired behaviour:
```py
def Vec2 : (T : imp type is number, x, y = T);
def a = Vec2(.T = i32); # valid
```

Or set the variable type explicitly, them the empty initializer will work:
```py
def Vec2 : (T : imp type is number, x, y = T);
def a = Vec2(i32) Vec2(); # valid
```

It's possible to access constant members:
```py
def Vec2 : (T : imp type is number, x, y = T);
def a = Vec2(1, 2);
def b = a.T Vec2(); # valid: 
```

But is not possible to modify them:
```py
def Vec2 : (T : imp type is number, x, y = T);
def a = Vec2(1, 2);
a.T = f32; # error: 'T' is a constant
```

To pass structs with constant members to functions the struct variant must be explicit:
```py
def Vec2 : (T : imp type is number, x, y = T);
def vec2_add(a, b = Vec2(i32)) Vec2(i32) => Vec2(a.x+b.x, a.y+b.y);
def _ = Vec2(1, 2) vec2_add Vec2(3, 4); # valid
def _ = Vec2(1.2, 2.1) vec2_add Vec2(3.4, 4.3); # invalid
```

If generic behavior is desired use constant arguments:
```py
def Vec2 : (T : imp type is number, x, y = T);
def vec2_add(T : imp type is number, a, b = Vec2(T)) Vec2(T) => Vec2(a.x+b.x, a.y+b.y);
def _ = Vec2(1, 2) vec2_add Vec2(3, 4); # valid
def _ = Vec2(1.2, 2.1) vec2_add Vec2(3.4, 4.3); # valid
```

#### Compile-time only structs
If a struct has only constant members it becomes a compile-time only type:
```py
def Vec2 : (T : imp type is number, a, b : T);
def a = Vec2(1, 2); # error: 'Vec2' is a compile-time only type and can't be assigned to a variable
```

This is useful for something like this:
```py
def Plugin : (
  T : imp type,
  init : fn() T,
  run : fn(inst = T) void,
);
def load_plugin : (P : Plugin) => {
  def inst = P.init();
  P.run(inst);
}
```

### Static constants
Member constants are per instance constants and cannot have a default value. Static constants are per struct constant and they need to have a value.

To define a static constant use the following syntax:
```py
def <struct>.<constant-name> : <value>;
```

This constant can be accessed only via the struct name, not by it's instances:
```py
def Point : (x, y = i32);
def Point.ONE : Point(1, 1);
def p0 = Point.ONE; # valid
def p1 = p0.ONE; # error: 'ONE' is not a member of 'Point'
```

You can have member constants and variables with the same name as static constants:
```py
def Point : (ONE : Point, x, y = i32);
Point.ONE : Point(Point(1, 1), 1, 1);
def p0 = Point.ONE; # valid
def p1 = p0.ONE; # valid
```

You can access static constants via struct variants:
```py
def Vec2 : (T : type is number, x, y = T);
def Vec2.ONE : Vec2(1, 1);
def v0 = Vec2(f32).ONE; # in this case a Vec2(i32) is being generated from a Vec2(f32) variant
```

Inside static constant initializers, `()` refers to the current struct variant. Use `().<name>` to access constant member values:
```py
def Vec2 : (T : type is number, x, y = T);
def Vec2.ONE : Vec2(1 -> ().T, 1 -> ().T);
def v0 = Vec2(f32).ONE; # generates an Vec2(f32) with x and y as 1
```

If a static constant uses `()`, it must be called only from struct variants. Accessing it via the base struct will cause an error:
```py
def Vec2 : (T : type is number, x, y = T);
def Vec2.ONE : Vec2(1 -> ().T, 1 -> ().T);
def v0 = Vec2.ONE; # error: 'ONE' can only be accessed via 'Vec2' variants
```

Static variables aren't a thing.

#### Methods
Methods are static constant functions that accept a, mutable or immutable, pointer, reference, view or direct value to the struct of the static constant itself as it's first variable argument:
```py
def Vec2 : (x, y = f32);
def Vec2.add : (self = Vec2, other = Vec2) => Vec2(self.x+other.x, self.y+other.y);
def a = Vec2(1, 2);
def b = Vec2(3, 4);
def c = Vec2.add(a, b);
```

Methods are the only static constants that can be accessed via instances. This will actually pass an address of value to the instance as the method first paremeter:
```py
def Vec2 : (x, y = f32);
def Vec2.add : (self, other = Vec2) => Vec2(self.x+other.x, self.y+other.y);
def a = Vec2(1, 2);
def b = Vec2(3, 4);
def c = a.add(b); # sugar for Vec2.add(a, b)
```

`self` is just an argument so it can use any of the argument definition established previously:
```py
def Vec2 : (x, y = f32);
def Vec2.add0 : (self = Vec2, other = Vec2) => ...; # valid
def Vec2.add1 : (self, other = Vec2) => ...; # valid
```

It's a convention to call the `self` argument as `self`, it actually can be named as anything:
```py
def Vec2 : (x, y = f32);
def Vec2.add : (this, other = Vec2) => Vec2(this.x+other.x, this.y+other.y);
```

Methods and static constants as a whole are mainly used on structs, but they actually can be defined for _any_ type:
```py
def i32.square : (self = i32) => self*self;
def twenty_five = 25.square();
```

Infix calls cannot be used on method calls. But if the method takes only one argument besides the `self` argument the one-parameter call convention is possible:
```py
def i32.add : (self, other = i32) => self+other;
def twenty_five = 20.add 5;
```

So this is completely possible with the language rules:
```py
def Vec2 : (x, y = f32);
def Vec2.add : (self, other = Vec2) Vec2 => (self.x+other.x, self.y+other.y);
def _ = Vec2(1.0, 2.0).add(3.0, 4.0);
```

If a method have the same name as a member constant or variable the member takes priority, so methods have to be called as standard static constant functions:
```py
def Foo : (x = i32, one = i32 1);
def Foo.one : (_self = Foo) i32 => 1;
def a = Foo(.one = 2);
def one = mut i32;
one = a.one; # one == 2
one = Foo.one(a); # one == 1
```

Keep in mind that you technically can use [function pointers](#Function-pointers) to have functions directly inside structs:
```py
def Vec2 : (
  x, y = f32,
  add = imm &(self, other = Vec2) { # the 'Vec2' in here doesn't count as a recursive variable
    ret Vec2(self.x+other.x, self.y+other.y);
  },
)
```

This is not recommended if the intention is method behavior because: it'll waste memory, it'll be slower than methods because of function pointer derreferencing, the method-like call will not work and the member it's not garanteed to always have the same beahvior even with `imm`.

#### Generics and static constants
When you define a function with a constant argument, what you're actually doing is creating a template. So if you try to access a static constant from a generic type of a function the function will simply assume that the type has the static constant. The same thing applies on method-like call:
```py
def foo : (T : type) u64 => {
  def a = T.FOO;
  ret something() * T.FOO; # because of the usage 'T.FOO' is implied to u64, same thing for 'a' 
};
def bar : (T : imp type, a = T) => {
  a.something(T.FOO); # 'T.something' is expected to be able to receive 'T.FOO' as it's first argument
};
def Foo : (a, b = i32);
def Foo.FOO : 1;
def Foo.something : (x = i32) => ...;
def _  = foo(Foo); # valid
def _  = foo(i32); # error: i32 doesn't have static constant 'FOO'
bar(Foo(1, 2)); # valid
bar(1); # error: i32 doesn't have static constant 'FOO' nor 'something'
```

So functions with generic types actually are duck typed at compile-time.

### 'take' attribute
There are times when you want to treat the members of a struct type member as its own. Use the `take` attribute for that:
```py
def Vec2 : (x, y = f32);
def Entity : (
  position = take Vec2,
  id = u32,
);
```

Then the members of the member type itself can be accessed by the base struct:
```py
def ent = mut Entity();
ent.x = 10; # valid, same as 'ent.position.x = 10'
```

If member names conflict the own struct member takes priority:
```py
def Vec2 : (x, y = f32);
def Entity : (
  position = take Vec2,
  x = u32,
  id = u32,
);
def ent = mut Entity();
ent.x = 10; # this modifies the 'Entity' 'x' not the position 'x'
```

If conflict between two takes arrises you simply can't use the shortcut:
```py
def Vec2 : (x, y = f32);
def Entity : (
  position = take Vec2,
  velocity = take Vec2,
  id = u32,
);
def ent = mut Entity();
ent.x = 10; # error: 'x' conflicting 'take' members
```

Methods from a `take` member have to be called through the member directly:
```py
def Vec2 : (x, y = f32);
def Vec2.add_eq : (self = *mut Vec2, other = Vec2) => *self = Vec2(self.x+other.x, self.y+other.y);
def Entity : (position = take Vec2, id = u32);
def ent = mut Entity();
ent.position.add_eq(Vec2(1, 2, 3)); # valid
ent.add_eq(Vec2(1, 2, 3)); # error: 'add_eq' isn't a static function for the type 'Entity'
```

`take` also works with [unions](#Unions) and basically any type with members.

### Structure of Arrays
Most languages have the Array of Structures layout as the default way to store buffers of structs.

With the AoS layout a struct that looks like this:
```py
def Foo : (x, y, z = i32);
```

Would have this layout on arrays and other sort of buffers:
```
+-----------------------------------------------------------+
| x0 | y0 | z0 | x1 | y1 | z1 | x2 | y2 | z2 | x3 | y3 | z3 |
+-----------------------------------------------------------+
```

All packed in one contiguous array. This type of buffer made sense on old computers. But on modern CPUs this module of data storage is very slow for cache locality and SIMD instructions.

The most efficient way of storing that is actually multiple arrays for each member, this method is called Structure of Arrays:
```
+-------------------+
| x0 | x1 | x2 | x3 |
+-------------------+
+-------------------+
| y0 | y1 | y2 | y3 |
+-------------------+
+-------------------+
| z0 | z1 | z2 | z3 |
+-------------------+
```

In other languages achieving this kind of layout can be quite cumbersome to create and handle, an example in C would be:

This would translate to this C code:
```c
#define FOO_BUFF_LEN 100 
typedef struct {
  int x[FOO_BUFF_LEN];
  int y[FOO_BUFF_LEN];
  int z[FOO_BUFF_LEN];
} Foo;
Foo foo_buf;
for (int i = 0; i < FOO_BUFF_LEN; i++) {
  foo_buf.x[i] = i;
}
```

In stark when creating an array of some struct you can use the `soa` attribute, that way this layout is achieve automatically:
```py
def Foo : (x, y, z = i32);
def foo_buf = soa [3]Foo;
for i in 0..@len(foo_buf) => foo_buf[i].x = i;
```

That way structure of arrays are handled in a extremely similar way to the familiar AoS.

Nested structures will also follow the SoA layout. This:
```py
def Vec2 : (x, y = f32);
def Entity : (position = Vec2, id = u32);
def ent_buf = soa [3]Foo;
for i in 0..@len(ent_buf) => ent_buf[i].position.x = i;
```

Would translate to this C code:
```c
#define ENT_BUFF_LEN 100 
typedef struct {
  float position_x[ENT_BUFF_LEN];
  float position_y[ENT_BUFF_LEN];
  unsigned int id[ENT_BUFF_LEN];
} Entity;
Entity ent_buf;
for (int i = 0; i < ENT_BUFF_LEN; i++) {
  ent_buf.position_x[i] = i;
}
```

If a member struct is not supposed to unroll like this add the `crate` attribute to it:
```py
def Vec2 : (x, y = f32);
def Entity : (position = crate Vec2, id = u32);
def ent_buf = soa [3]Foo;
for i in 0..@len(ent_buf) => ent_buf[i].position.x = i;
```

Then the `position` would not be unrolled, this is the C translation:
```c
#define ENT_BUFF_LEN 100 
typedef struct { float x, y; } Vec2;
typedef struct {
  Vec2 position[ENT_BUFF_LEN]
  unsigned int id[ENT_BUFF_LEN];
} Entity;
Entity ent_buf;
for (int i = 0; i < ENT_BUFF_LEN; i++) {
  ent_buf.position[i].x = i;
}
```

#### Views
There is some pitfalls for using SoA over AoS, the major one is passing pointers to individual elements of the array. This is actually not possible in stark:
```py
def Foo : (x, y, z = i32);
def foo_buf = soa [100]Foo;
def ptr_to_some_foo = &foo_buf[0]; # error: taking address of soa buffer element
```

If this is inteded behavior what you really want is a view, not a pointer. Views are a collection of pointers into a SoA buffer, to take a view of a struct element use the `[&]` operator:
```py
def Foo : (x, y, z = i32);
def foo_buf = soa [100]Foo;
def view_to_some_foo = [&]foo_buf[0]; # valid
```

The "all values are immutable by default" rule applies to views, use `[&]mut` to take a mutable view:
```py
def Foo : (x, y, z = i32);
def foo_buf = [100]Foo;
def view_to_some_foo = [&]mut foo_buf[0];
view_to_some_foo.x = 10; # foo_buf[0].x == 10
```

The type of a view is `[*]` for immutable and `[*]mut` for mutable followed by the type that the view views into:
```py
def view0 = [*]Foo [&]foo_buf[0];
def view1 = [*]mut Foo [&]mut foo_buf[0];
```

It's possible to derreference a view:
```py
def view = [*]Foo [&]foo_buf[0];
*view = Foo(1, 2, 3);
```

Keep in mind that SoA elements can't use the [arguments as references rule](#Immutable-arguments-are-references) for optimization. The values will have to be copied.

It is also only possible to use methods from SoA elements that receive a view or a value:
```py
def Vec2 : (x, y = f32);
def Vec2.add : (self = Vec2, other = Vec2) Vec2 => Vec2(self.x+other.x, self.y+other.y);
def Vec2.add_eq_view : (self = [*]mut Vec2, other = Vec2) => *self = Vec2(self.x+other.x, self.y+other.y);
def Vec2.add_eq_ptr : (self = *mut Vec2, other = Vec2) => *self = Vec2(self.x+other.x, self.y+other.y);
def Entity : (position = Vec2, id = u32);
def ent = mut soa [10]Entity;
def new_pos = ent[0].position.add(Vec2(1, 2, 3)); # valid
ent[0].position.add_eq_view(new_pos); # valid
ent[0].position.add_eq_ptr(new_pos); # error: 'ent[0]' can't be passed as a pointer
```

### Packed structs
Structs have padding between members if necessary. This is done for optimization reasons, but if a struct has to be packed without padding add `pack` as prefix in the struct definition:
```py
def Some_Unpacked_File_Header : (tag = [4]achar, big_id = u64, size = u16); # size_of(Some_Unpacked_File_Header) = 24 bytes
def Some_Packed_File_Header : pack(tag = [4]achar, big_id = u64, size = u16); # size_of(Some_Unpacked_File_Header) = 14 bytes
```

## Unions
Unions are a collection of _tags_. Each tag represents a unique variant of the union, associated with a specific type. The _tag_ acts as the discriminator, and the _type_ defines how data must be held. 

To define an union tag, first put the name of the tag, followed by the desired type to be associated with it (e.g. `tag type`). To add an additional tag use `|`, meaning 'or', as a separator. Every tag must be enclosed by a single pair of parenthesis to be a valid union definition:
```py
def Some_Union : (tag0 type0|tag1 type1|...);
```

A simple example:
```py
def Some_Struct : (a, b, c = i32);
def Numbers : (Integer i32|Floating_Point f32|A_Tag Some_Struct);
```

Trailing `|` are allowed:
```py
def Some_Struct : (a, b, c = i32);
def Numbers : (Integer i32|Floating_Point f32|A_Tag Some_Struct|);
```

Infact to define an union with only one tag, trailing `|` are required:
```py
def Foo : (Integer i32); # Can be ambiguious in some cases
def Bar : (Integer i32|); # Union with one tag
```

And of course, direct struct literals are allowed:
```py
def Numbers : (Integer i32|Floating_Point f32|A_Tag(a, b, c = i32)|);
```

### Repeated tag name and tag type
Tags can have names of outside constants and predefined types:
```py
def Vec2 : (x, y = u32);
def Numbers : (i32 i32|f32 f32|Vec2 Vec2); # valid
```

This is repetitive. To overcome this problem add `+` as a suffix to the tag name:
```py
def Vec2 : (x, y = u32);
def Numbers : (i32+|f32+|Vec2+); # shortcut for (i32 i32|f32 f32|Vec2 Vec2)
```

### Tags numeric representation
Unions tags are just integers IDs. The first tag defaults to 0 and each subsequent tag increment by 1 automatically.

It's possible to set the tag value manually:
```py
def Some_Union : (Tag0 = 1 i32|Tag1 = 2 u32|Tag2 = 0 f32);
```

Tags with the same underlying type can have the same numeric value:
```py
def Some_Union : (Tag0 = 0 i32|Tag1 = 0 i32); # valid
```

And tags with different underlying types cannot have the same numeric value:
```py
def Some_Union : (Tag0 = 0 i32|Tag1 = 0 u32); # error: 'Tag0' and 'Tag1' share same tag number, but have diffent underlying types
```

Also, after setting a tag numeric value manually subsequent tags still have the value incremented manually:
```py
def Some_Union : (Tag0 i32|Tag1 = 3 u32|Tag2 f32); # Tag0 == 0, Tag1 == 3, Tag2 == 4
```

Tags can be negative:
```py
def Some_Union : (Tag0 = -2 i32|Tag1 u32|Tag2 f32); # Tag0 == -2, Tag1 == -1, Tag2 == 0
```

The tag numeric ID type always defaults to the smallest possible type that holds the range of all tags numeric values. Unsigned types are always prioritized:
- `u8` in between 0 and 255
- `i8` in between -128 and 127
- `u16` in between 0 and 65_535
- `i16` in between -32768 and 32767
- `u32` in between 0 and 4_294_967_295
- `i32` in between -2_147_483_648 and 2_147_483_647
- `u64` in between 0 and 18_446_744_073_709_551_615
- `i64` in between -9_223_372_036_854_775_808 and 9_223_372_036_854_775_807

But if you desire to enforce a specific integer type on the tags you can do so:
```py
def Some_Union : (Tag0 = 256 i32) u8; # error: union with tags of type 'u8' can't hold the tag numeric value of '256'
```

### Union instantiation
Unions don't have a specific initialization like [structs](#Struct-instantiation). You actually just need to provide the correct union type + tag to a variable with the following syntax `<union>.<tag>`:
```py
def Vec2 : (x, y = f32);
def Some_Union : (Number i32|String str|Vector Vec2);
def number = Some_Union.Number; # 'number' is of type 'Some_Union.Number'
def string = Some_Union.String; # 'string' is of type 'Some_Union.String'
def vector = Some_Union.Vector; # 'vector' is of type 'Some_Union.Vector'
```

Then you simply just pass the appropriate values or initializators:
```py
def Vec2 : (x, y = f32);
def Some_Union : (Number i32|String str|Vector Vec2);
def number = Some_Union.Number 10;
def string = Some_Union.String "10";
def vector = Some_Union.Vector Vec2(10.0, 0.10);
```

Because each union tag already holds the proper type, you can [initialize a struct without the explicit type](#Implicit-struct-name-on-instantiation):
```py
def Vec2 : (x, y = f32);
def Some_Union : (Number i32|String str|Vector Vec2);
def vector = Some_Union.Vector(10.0, 0.10); # valid
```

Keep in mind that the actual type of the union isn't `<union>.<tag>`, just `<union>`. The tag just signals to the compiler how the variable data and type checking should be treated. So a explicitly typed union would be:
```py
def Vec2 : (x, y = f32);
def Some_Union : (Number i32|String str|Vector Vec2);
def vector = Some_Union Some_Union.Vector(1.0, 2.0); # valid
```

If the variable already has the proper union type you can use `.<tag>` as a shortcut:
```py
def Some_Union : (Number i32|String str);
def foo = Some_Union;
foo = .String "hello";
```

#### Union default tag
The default value of an union is always the first declared tag (the tag numeric value does not matter):
```py
def Some_Union : (Number i32|String str);
def num0 = Some_Union;
def num1 = Some_Union.Number;
```

In the code above, both `num0` and `num1` have `Some_Union.Number` as their tag numeric representation. 

### Accessing union data
To access the data of an union use `^` as a suffix:
```py
def Vec2 : (x, y = f32);
def Some_Union : (Number i32|String str|Vector Vec2);
def vector = Some_Union.Vector(1.0, 2.0); # valid
def number = Some_Union.Number 10;
def x = vector^.x; # valid, x == 1.0
def a = number^; # valid, a == 10
```

The `^` suffix can only be used when it's absolutely clear to the compiler what's the tag of the giving union. If it isn't use a `switch` or `if` expression:
```py
switch some_union (
  .Int   => some_union^ = 10,
  .Float => some_union^ = 10.0,
);
if some_union == .Float => some_union^ *= 0.5;
```

### Union mutability
The mutability of unions works in [the same way as the rest of the variables](#Variable-mutability). If you wish to change a value from a type within an union use the `mut` attribute as always:
```py
def Vec2 : (x, y = f32);
def Some_Union : (Number i32|String str|Vector Vec2);
def vector = mut Some_Union.Vector(1.0, 2.0); # valid
vector^.x = 10!;
```

Mutable unions also permit you to directly change its tag. Keep in mind that when the tag is changed the data is also reset to the proper defaults of the new tag:
```py
def vector = mut Some_Union.Vector(1.0, 2.0); # valid
vector = .String; # Vector is initialized to the default value, in this case an empty string ""
vector = .String "hi"; # You can also provide an value along with the tag. 'vector' is equal to the "hi" string
```

### Tags comparability
You can compare tags with other tags:
```py
def vector = mut Some_Union.Vector(1.0, 2.0); # valid
vector == Some_Union.Vector; # true
```

This is not a data comparison, it just compares the tags numeric values.

The union name can also be omitted here:
```py
def vector = mut Some_Union.Vector(1.0, 2.0); # valid
vector == .Vector; # valid
```

### Getting the tag numeric value
If a tag numeric value is needed use `^` as a prefix:
```py
def Some_Union : (Number i32|String str|Vector Vec2);
def vector = mut Some_Union.Vector(1.0, 2.0); # valid
def tag_val = ^vector; # in this case, tag_val == 2
```

It can be used directly from union tags:
```py
def Some_Union : (Number i32|String str|Vector Vec2);
def tag_val = ^Some_Union.String; # in this case, tag_val == 1
```

You can use a specific variant to get the union tag number, but it's not necessary:
```py
def Some_Union : (T : type is number|Number T|String str|Vector Vec2);
def tag_val0 = ^Some_Union(i32).String; # valid, tag_val0 == 0
def tag_val1 = ^Some_Union.String; # valid, tag_val1 == 0
```

The inferred type based on a tag numeric value is equal to the [tag numeric value type](#Tags-numeric-representation). But if the type is not enforced tags numeric values can be passed into any integer that can hold the specified range:
```py
def Some_Union0 : (Number i32|String str|Vector Vec2);
def tag_val0 = ^Some_Union0.String; # tag_val0 type is u8, tag_val == 1
def tag_val1 = i32 ^Some_Union0.String; # tag_val1 type is i32, tag_val == 1
def Some_Union1 : (Number i32|String str|Vector Vec2) i32;
def tag_val2 = ^Some_Union1.String; # tag_val2 type is i32, tag_val == 1
def tag_val3 = u8 ^Some_Union1.String; # error: tags from 'Some_Union1' are i32
```

Note that the union name type can't be omitted for getting tags because there aren't a way to know which union you're referencing:
```py
def Some_Union0 : (Number f32|String str|Vector Vec2);
def Some_Union1 : (Number i32|String str|Vector Vec2);
def tag_val = ^.Number; # error: missing union name after '^'
```

### Untyped unions
You can set the underlying type of a tag to be `void`, this actually indicates that no memory will be held by that tag. If only `void` types are provided the union only holds enough data for the tag numeric value:
```py
def Color : (Red void|Green void|Blue void);
def color = mut Color;
color = .Red;
```

You actually can leave out the tag type and it'll default to void ([this is similar to the function return type](#Void-return-type)):
```py
def Color : (Red|Green|Blue);
def color = mut Color;
color = .Red;
```

This is equivalent to enums in other languages.

### Passing unions as parameters
You can pass unions to arguments as expected:
```py
def Some_Union : (Int i32|Float f32|String str);
def do_something : (a = Some_Union) => ...;
def foo = Some_Union.Int 10;
def bar = Some_Union.Float 10.0;
def baz = Some_Union.String "10";
do_something(foo); # valid
do_something(bar); # valid
do_something(baz); # valid
```

But if you want, you can determine that a function can only accept one specific union tag:
```py
def Some_Union : (Int i32|Float f32);
def do_something : (a = Some_Union.Int) => ...;
def foo = Some_Union.Int 10;
def bar = Some_Union.Float 10.0;
do_something(foo); # valid
do_something(bar); # error: passed 'Some_Union.Float', but expected 'Some_Union.Int'
do_something(baz); # error: passed 'Some_Union.String', but expected 'Some_Union.Int'
```

With this approach you're still passing an union type, not just the tag value.

It's also possible to expected just more than one specific tag with a `|`:
```py
def Some_Union : (Int i32|Float f32);
def do_something : (a = Some_Union.Int|.Float) => ...; # 'a = Some_Union.Int|Some_Union.Float' is also valid
def foo = Some_Union.Int 10;
def bar = Some_Union.Float 10.0;
do_something(foo); # valid
do_something(bar); # valid
do_something(baz); # error: passed 'Some_Union.String', but expected 'Some_Union.Int'
```

It's not possible to have a variable be from two different distinct union types using this though:
```py
def Some_Union0 : (Int i32|Float f32);
def Some_Union1 : (Int i32|Float f32);
def do_something : (a = Some_Union0.Int|Some_Union1.Float) => ...; # invalid
```

Keep in mind that using specific tagged functions is only possible when it's clear to the compiler which tag that union has. Similar to [the '^' suffix](#Accessing-union-data).

### Constant tags
[Similar to structs constant members](#Constant-members), unions can have constant tags. They follow the same rules and are defined in the same way as constant members. The only definition difference is the separator, it's a pipe instead of a comma:
```py
def Some_Union : (T : type|N : usize|Type T|Buffer [N]T|);
```

To instantiate an union with a constant tag you actually use an initializer on the union type itself. These are union variants:
```py
def Some_Union : (T : type|N : usize|Type T|Buffer [N]T|);
def type_i32 = Some_Union(i32, 3).Type 10;
def buff_i32 = Some_Union(i32, 3).Buffer [1, 2, 3];
def type_f32 = Some_Union(f32, 3).Type 10;
def buff_f32 = Some_Union(f32, 3).Buffer [1, 2, 3];
```

Constant members of unions can't really accept `imp`. The reason for this is because not all constant tags may be used on all tag types. 

An union can only accept struct variants, so if being generic is a desired factor the union will also have to accept the required constant members:
```py
def Vec2 : (T : imp number, x, y = T);
def Some_Union : (T : type is number|Int i32|Vec Vec2(T)|);
```

### Unions static constants
Unions can have [static constants](#Static-constants), and they work exactly like the struct ones:
```py
def Some_Union : (Int i32|Float f32);
def Some_Union.ZERO : 0;
def a = Some_Union.ZERO;
```

You can also have static constants on specific tags:
```py
def Some_Union : (Int i32|Float f32);
def Some_Union.Int.ZERO : 0;
def Some_Union.Float.ZERO : 0.0;
def a = Some_Union.Int.ZERO;
def b = Some_Union.Float.ZERO;
```

[Methods](#Methods) also behave the same:
```py
def Some_Union : (Int i32|Float f32);
def Some_Union.do_something : (self = Some_Union) =>...;
def a = Some_Union.Int 10;
def b = Some_Union.Float 10.0;
a.do_something();
b.do_something();
```

Methods for unions have to have a pointer, view or value to the type of the specified union. It can't be specific tags for the method call to work.

And they can also be bound to specific tags:
```py
def Some_Union : (Int i32|Float f32);
def Some_Union.Int.do_something : (self = Some_Union.Int) =>...;
def a = Some_Union.Int 10;
def b = Some_Union.Float 10.0;
a.do_something();
b.do_something(); # error: 'b' has no method 'do_something'
```

Methods for specific union tags have to have a pointer, view or value to that specific tag only.

Specific tag methods have priority over whole union methods:
```py
def Some_Union : (Int i32|Float f32);
def Some_Union.do_something : (self = Some_Union) =>...;
def Some_Union.Int.do_something : (self = Some_Union.Int) =>...;
def a = Some_Union.Int 10;
def b = Some_Union.Float 10.0;
a.do_something(); # Some_Union.Int.do_something(b);
b.do_something(); # Some_Union.do_something(b);
```

### Untagged unions
You can mark the enforced type of the numeric value of a union to be `void`. If that's the case you simply cannot define tags, only the types directly:
```py
def Some_Union : (i32|f32) void;
```

The `^` prefix and suffix will not work, this union will behave like a C union:
```py
def Vec3 : ((x, y, z = f32)|(r, g, b = f32)) void;
def foo = mut Vec3;
foo.x = 10;
a = foo.r; # a == 10
```

## Casting
Stark is strongly typed. You can't pass an u8 to a f32 or even an i8 to an i16. But sometimes casting is needed, on those cases use the `->` operator:
```py
def a = 1.5;
def b = a -> i32; # now 'b' is an i32 with a value of 1
```

Chained casting is valid:
```py
def a = 1.5;
def b = a -> i32 -> u8; # 'b' is an i8 with value 1
```

### Pointer casting
The `->` operator can also be used on pointers:
```py
def a = i32;
def p0 = &a;
def p1 = p0 -> *i8; # p1 points to 'a' as if it was a '*i8'
```

It's not possible to cast an immutable pointer into a mutable one:
```py
def a = i32;
def p0 = &a;
def p1 = p0 -> *mut i8; # error
```

Casting between pointers and integers is possible:
```py
def a = i32;
def p0 = &a;
def i0 = p0 -> u64; # valid
def i1 = p0 -> isize; # valid
def i2 = p0 -> i8; # valid
def p1 = i0 -> *i32; # valid
```

Keep in mind that casting between pointers, and specially pointer-integer casting, can be extremely unsafe:
```py
def a = 0xdeadbeef;
def p1 = i0 -> *i32;
def b = *p1; # program will crash
```

It's not possible to cast an integer into a mutable pointer:
```py
def a = 0xdeadbeef;
def p1 = i0 -> *mut i32; # error
```

### String casting
Casting from `cstr` to `str` is possible:
```py
def some_str = c"hello" -> str;
```

Casting from `str` to `cstr` is also possible:
```py
def some_cstr = "hello" -> cstr;
```

Keep in mind that when casting `str -> cstr` non-ascii characters will be translated to `-1`:
```py
def some_cstr = "olá" -> cstr; # 'á' will become -1 and the printing will be weird
```

### Union tag casting 
Casting between union tags is possible:
```py
def Some_Union : (i32+|str+);
def x = Some_Union.str "hi";
def _ = x -> Some_Union.i32; # valid
def _ = x -> .i32; # valid
```

This just changes the tag of the casted union, the data will remain the same.

It's not possible to cast between different unions, even if the data and tag numeric value would technically remain the same:
```py
def Some_Union0 : (i32+|str+);
def Some_Union1 : (i32+|str+);
def x = Some_Union0.str "hi";
def _ = x -> Some_Union1.str; # invalid
```

### Struct casting
If two struct types have the exact same data layout it is possible to cast between them:
```py
def Vec3  : (x, y, z = f32);
def Color : (r, g, b = f32);
def pos = Vec2(1.0, 2.0, 3.0);
def col = pos -> Color; # valid
```

### Auto casting
Sometimes you don't want to explicitly cast to a type. Use the `!` operator as an automatic `-> type` cast to the expected type:
```py
def sum : (a, b = i32) i32 => a+b;
def x = sum(12, 1.6!); # equivalent to 'sum(12, 1.6 -> i32)'
def y = u64 1.6!;
```

`!` operator is not valid for pointers:
```py
def foo : (ptr = *i32) => ...;
def x = i32 10;
def y = *u8;
foo(x!); # invalid
foo(y!); # invalid
```

Or union tags:
```py
def Some_Union : (i32+|str+);
def foo : (x = Some_Union.i32) => ...;
def x = Some_Union.str "hi";
foo(x!); # invalid
```

Auto casting between structs with same layout is possible, this is useful for unnamed [structs](#Structs) or [tuples](#Tuples):
```py
def div : (a, b = i32) (bool, i32) => b == 0 ? (false, 0) : (true, a/b);
def Result : (bool, i32);
def res = Result div(4, 2); # error: types differ
def res = Result div(4, 2)!; # valid
```

### Invalid casting
Casting is not valid for [arrays](#Arrays), [slices](#Slices), [anyrt](#The-anyrt-type) and custom types (except for distinct aliases or union tags).

## The meta type
A `meta` is a builtin [union](#Unions) type that represents a node from the Stark AST.

It is a [compile-time-only type](#Compile-time-only-primitive-types) and it follows the same rules as the primitive ones.

To see an in-depth list of all tags held by a `meta` see: [Meta types](#Types-of-metas).

### Meta literals
To create a `meta` from code use the builtin `code` function, this function accepts strings, but they really shine if [raw string literals](#Raw-string-literals) are use:
```py
some_code : @code $code_end
  def add : (T : imp type is number, a, b = T) T => a+b;
  def sub : (T : imp type is number, a, b = T) T => a-b;
  def mul : (T : imp type is number, a, b = T) T => a*b;
  def div : (T : imp type is number, a, b = T) T => a/b;
code_end;
```

Because of the way raw string literals works, they can be nested:
```py
some_code : @code $code_end
  some_code_inside_some_code : @code $code2_end
    def square_from_the_code_inside_the_code : (T : imp type is number, x = T) T => x*x;
  code2_end
code_end;
```

The tag of the generated `meta` is `meta.root`.

To put all this meta-code inside your actual source code is very simple, use the `@` operator:
```py
some_code : @code $code_end def TEN : 10; code_end;
@some_code; # now 'TEN' is defined
```

What that did was put whatever was in the `some_code` AST into the main code AST at that spot.

The raw literals constant bindings gives you macro behaviour:
```py
def TEN : 10;
define_ten2 : @code $(TEN) code_end def TEN2 : $TEN$; code_end;
@define_ten2; # TEN2 is now defined with the value 10 on it
```

### meta.iden and meta.expr
The `meta.iden` tag from `meta` can accept an identifier directly:
```py
an_actual_identifier : meta.iden some_random_identifier;
```

Then it can be directly injected into code:
```py
def an_actual_identifier : meta.iden some_random_identifier;
def @an_actual_identifier : 20; # 'some_random_identifier' is defined here with the value of 20
```

The `meta.expr`, similarly to `meta.iden`, can accept expressions directly on it's definition:
```py
def an_actual_expression : meta.expr 3 + 4;
def something : () i32 => @an_actual_expression; # 'something' returns 3 + 4
```

### Metaprogramming with metas
You can return metas from ensured-compile-time functions:
```py
def return_code : @() meta => @code $end
  def square(T : type is number, a = T) T => a*a;
end;
```

Using the `@` operator on a function that returns a `meta` will not only call the function, but also inject the `meta` AST into the main AST:
```py
def gen_square_fn : @() meta => @code $end
  def square(T : type is number, a = T) T => a*a;
end;
@gen_square_fn(); # now the square function is defined
```

The `meta.iden` and `meta.expr` also can be passed as constant arguments, this is useful for putting their values directly on the code function:
```py
def gen_fn_with_arg_a : @(fn_name : meta.iden, fn_expr : meta.expr) meta => @code $(*)end
  def $fn_name$(T : type is number, a = T) T => $fn_expr$;
end;
@gen_fn_with_arg_a(square, a * a);
```

### Types of metas
This is the meta definition:
```py
def meta : (
    root   (child = *meta)     # Root of an AST
  | expr   (child = *meta)     # Expression
  | iden   (name = str)        # Identifier
  | int    (value = str)       # Integer literal
  | float  (value = str)       # Floating point literal
  | string (value = str)       # String literal
  | add    (lhs, rhs = *meta)  # Addition
  | sub    (lhs, rhs = *meta)  # Subtraction
  | mul    (lhs, rhs = *meta)  # Multiplication
  | div    (lhs, rhs = *meta)  # Division
  Work in progress...
);
```

## The anyrt type
`anyrt` is another builtin union. It stands for 'any runtime', and as it implies, you can pass any value to it at runtime and it'll hold type information plus an immutable pointer to the data:
```py
def a = 10;
def b = "hello";
def x = anyrt a; # valid
def y = anyrt b; # valid
```

You can pass values directly, the data for storing the value will be allocated on the current stack frame:
```py
def x = anyrt 10; # valid
```

Similarly to [pointers](#Pointer-definition), `anyrt` variables can't be initialized without any values:
```py
def x = anyrt; # error: no value provided
```

The difference is that `anyrt` cannot be an [option](#Options):
```py
def x = ?anyrt; # error: optinal anyrt
```

`anyrt` variables also can't be mutable:
```py
def x = mut anyrt 0; # error: mutable anyrt
```

### Getting the data from an anyrt
Every `anyrt` tag are structs, and they have a `.data` member field. That field is a pointer to the correct type:
```py
def x = anyrt 10;
def x_val = *x.data; # x_val is now defined as an i32 with value 10
def x_wrong_val = f32 *x.data; # error: passing wrong type
```

### Anyrt tags
`anyrt` is actually specific to every Stark project. It already holds many tags by default, but every time a custom type is used as the value of an `anyrt` variable, the type will be added to the `anyrt` union, or a sub-union of `anyrt`, as a tag.

This are the primitive tags:
```py
  i8 (data = *i8)
| u8 (data = *u8)
| u16 (data = *u16)
| i16 (data = *i16)
| u32 (data = *u32)
| i32 (data = *i32)
| u64 (data = *u64)
| i64 (data = *i64)
| f32 (data = *f32)
| f64 (data = *f64)
| usize (data = *usize)
| isize (data = *isize)
| str (data = *str)
| cstr (data = *cstr)
| bool (data = *bool)
| uchar (data = *uchar)
| achar (data = *achar)
```

[Struct](#Structs), [union](#Unions), [tuple](#Tuples), [array](#Arrays), [slice](#Slices), [pointer](#Pointers) and [view](#Views) tags are actually their own unions.

Every tag from the `struct` tag of `anyrt` follow this layout:
```py
(
  data = *void,
  name = str,
  members = [..](
    name = str,
    info = anyrt,
  ),
);
```

Every tag from the `union` tag of `anyrt` follow this layout:
```py
(
  data = *void,
  name = str,
  tags = [..](
    id = (u8+|i8+|u16+|i16+|u32+|i32+|u64+|i64+),
    name = str,
    info = str,
  ),
);
```

Every tag from the `array` tag of `anyrt` follow this layout:
```py
(
  data = *void,
  info = *anyrt,
  length = usize,
);
```

Every tag from the `slice` tag of `anyrt` follow this layout:
```py
(
  data = *void,
  info = *anyrt,
  length = usize,
);
```

Every tag from the `pointer` tag of `anyrt` follow this layout:
```py
(
  data = **void,
);
```

Every tag from the `view` tag of `anyrt` follow this layout:
```py
(
  data = **void,
);
```

The `data` field of every tag will obviously be a pointer to the correct type instead of `void`. For the `array` and `slice` tags it'll be a slice to the corret type instead of a pointer. And for the `view` tag it'll be views instead of pointers.

Actually the above struct layouts are also tags of `anyrt`, they're called `struct_generic`, `union_generic`, `tuple_generic`, `array_generic`, `slice_generic`, `pointer_generic` and `view_generic`. These tags are used for when you want to account to any type of struct, union or tuple.

Distinct aliases are added to `anyrt` directly.

The tag name of any named type will be the type name itself.

There is a `unnamed` tag, this tag is an union. This union is basically an `anyrt` in itself, every sub-union of `anyrt` has an equivalent inside `unnamed`, every unnamed custom type goes inside `unnamed`, the name of each tag will be `t` +  an arbitrary id.

## Comparable types
A comparable type is a type that allowes the usage of the `==`, `!=`, `>=` and `<=` operators.
- `u8` is comparable with `u8` and `anyi`
- `u16` is comparable with `u16` and `anyi`
- `u32` is comparable with `u32` and `anyi`
- `u64` is comparable with `u64` and `anyi`
- `usize` is comparable with `usize` and `anyi`
- `i8` is comparable with `i8` and `anyi`
- `i16` is comparable with `i16` and `anyi`
- `i32` is comparable with `i32` and `anyi`
- `i64` is comparable with `i64` and `anyi`
- `isize` is comparable with `isize` and `anyi`
- `f32` is comparable with `f32` and `anyf`
- `f64` is comparable with `f64` and `anyf`
- `str` is comparable with `str`
- `cstr` is comparable with `cstr`
- Arrays can be compared if their length and type is the same
- Slices can be compared if their type is the same
- Struct instances can be compared if all their members are comparable types, and their struct type is the same
- Unions are comparable with it's tags

## Control flow (if, else, switch and loops)
### If and else
As everything else in the language, `if`s are expressions. To make an if expression use the `if` keyword, followed by a boolean condition, then the body assignment operator `=>` and finally an expression:
```py
if <condition> => <expr>;
```

After the `if` expression you can optionally have an `else` expression. It's similar to the `if` one, except that the condition is optional:
```py
if x == 10  => do_something();
else x < 10 => do_something_else();
else        => do_fallback();
```

The expression inside the `if` body will be returned. But returning a value from an `if` expression only works, if it's followed by an conditionless `else` expression:
```py
def y = if x == 10 => 10; else => 9;
def z = if x == 10 => 10; else x == 9 => 9; # error: if/else chain that returns expression missing a conditionless else
```

And all expressions from the `if`/`else` chain must return the same type:
```py
def y = if x == 10 => 10; else => 9.0; # error: if/else chain that returns different types
```

And of course, if a `if`/`else` chain returns something, it needs to be handled:
```py
if x == 10 => 10; else => 9; # error: unhandled expression
```

The ternary operator is also supported, they are exactly the same as an `if` and conditionless `else` that return an expression. The expression of ternary operations can't return `void`:
```py
def _ = x == 10 ? 10 : 9;
```

So the ternary operator is in reality just sugar for `if`/`else`.

#### Assigning on if
As it's known, [variable assignments](#Variable-assignment) wrapped around parenthesis return the variable value after it's assignment. This becomes really useful on if expressions:
```py
def x = mut i32;
if (x = get_x()) == 10 => do_stuff();
```

#### 'if def'
It's very commom to do an operation put the result in a variable and only use said variable if it's in certain coditions. Use an `if def` to make the variable be visible only on the `if`/`else` chain. The assignment rule is still applicable, so it's needed to wrap it around parenthesis:
```py
if def (x = i32 get()) == 10 && x < 10 => ...;
else x == 9                            => ...;

if def (x = bool get()) => ...;
else                    => ...;
```

You can also use `def` on `else`s with conditions:
```py
if something => ...;
else def (x = get_something()) == 10 => ...;
else x == 9 => ...; # 'x' now becomes avaiable for subsequent elses
```

Shadowing in `if`/`else` with `def` isn't allowed:
```py
if   def (x = get_this()) == 10 => ...;
else def (x = get_that()) == 9  => ...; # error: 'x' is already defined
```

Do an assignment without `def` for this is needed:
```py
if def (x = get_this()) == 10 => ...;
else   (x = get_that()) == 9  => ...; # valid
```

### Switch expression
An easier way then a `if`/`else` chain to check if a value is equal to some specific case, is the `switch` expression:
```py
switch <value> (
  <case> => do_this(),
  <case> => do_that(),
);
```

The equivalent of a conditionless else in a switch expression is the `or` keyword:
```py
switch <value> (
  <case> => do_this(),
  <case> => do_that(),
  or     => do_fallback(),
);
```

The `<value>` must be a value that can be compared.

The `<case>` must be a value that the `<value>` can be compared to.

A `switch` expression follow to returning values as an `if`/`else` chain:
```py
def _ = switch some_i32 (10 => 10, or   => 0); # valid
def _ = switch some_i32 (10 => 10, 0..9 => 9); # error: switch that returns expression missing a '_' case
def _ = switch some_i32 (10 => 10, or   => 9.0); # error: switch that returns different types
switch some_i32 (x == 10 => 10, _ = 9); # error: unhandled expression
```

You can have multiple cases assigned to the same body:
```py
switch some_i32 (
  0, 1, 2 => do_something(),
  or      =>               ,
);
```

In C the cases fallthrough each other automatically, in Stark they don't. If the fallthrough behavior is desired, use the `&&` operator before the next case: 
```py
switch some_i32 (
  0 => do_something(), &&
  1 => do_something_extra(),
  2, 3 => do_something_new(), &&
  4, 5 => do_something_extra_new(),
);
```

#### Switch on unions
A switch can accept an union because them can be compared with tags. The difference with a `switch` on unions is that all tags must be handled:
```py
def Color : (Red|Green|Blue);
def col = Color;
switch col (
  Red => do_red(),
  Green => do_green(),
); # error: unhandled 'Blue' tag on an union switch
```

You can use the `or` case to explicitly say that you don't want to handle does cases:
```py
def Color : (Red|Green|Blue);
def col = Color;
switch col (
  Red => do_red(),
  Green => do_green(),
  or =>;
); # valid
```

#### 'switch def'
Similarly to the `if def`, switches can also have a `switch def` and it works the same way:
```py
switch def (x = get_i32()) (
  0 => do_this_with_x(x);
  1 => do_that_with_x(x);
);
```

### While loop
`while` loops are simple, the loop will run until it's condition is false:
```py
def something = true;
while something => something = get_something();
```

Following the `switch` and `if`, `while` expression can also have `def`. This way they work like C for loops:
```py
while def (i = 0) < 10 => i++;
```

The definition will only occur once, on next iterations only the current value of `i` will be used.

You can use the `nxt` expression to go to the next iteration immediately: 
```py
while def (i = 0) < 10 => {
  i++;
  if i % 2 == 0 => nxt;
  do_something();
}
```

What the `nxt` expression does is it's just jumps to the end of the current expression-block. So it can only be used on loops and on expression blocks:
```py
while something => nxt; # error: 'nxt' outside of expression-block
```

Because `nxt` just jumps to the end of the expression block, all deferred expressions will be ran. So the correct way to do a C for loop is:
```py
while def (i = 0) < 10 => {
  defer i++;
  if i % 2 == 0 => nxt;
  do_something();
}
```

`nxt` accepts an expression that will be ran at the next loop iteration:
```py
while def (i = 0) < 10 => {
  defer i++;
  if i % 2 == 0 => nxt i++; # i will incremeant by 1 in the next iteration
  do_something();
}
```

Because `nxt` is an expression it can be nested:
```py
while def (i = 0) < 10 => {
  defer i++;
  if i % 2 == 0 => nxt nxt; # skips two iterations
  do_something();
}
```

### For loop
`for` loops are loops that iterate over an [array](#Arrays), a [slice](#Slices), a [range](#Ranges) or an [iterator](#Iterators).

Each iteration of a `for` loop gives you a value.

For ranges this value is an integer:
```py
for i in 0..9 => println("%", [i]);
```

By default is an i32, i64 or u64 depending on the values on the range. But you can specify exactly the type of integer that you want:
```py
for i = i8 in 0..9 => println("%", [i]);
```

As any variable, the iteration variable is immutable by default, but you can mark it as mutable:
```py
for i = mut in 0..9 => {
  i *= 2;
  println("%", [i]);
}
```

Mixing `mut` and the specified type is possible:
```py
for i = mut i8 in 0..9 => {
  i *= 2;
  println("%", [i]);
}
```

You can mark the range as `rev` for reverse iteration:
```py
for i in rev 0..9 => println("%", [i]);
```

For arrays and slices the value is a [tuple](#Tuples) of the current iteration index + an element from the array/slice. This element behavies like a value, but it can actually be a reference. It's similar to how [function arguments](#Immutable-arguments-are-references) works:
```py
def nums = [1, 2, 3];
for (i, n) in nums => println("nums[%] = %", [i, n]);
```

It's also possible to mark the [tuple](#Tuples) as mutable, if that's the case the element will be a pointer:
```py
def nums = mut [1, 2, 3];
for (i, n) = mut in nums => {
  *n *= 2;
  println("nums[%] = %", [i, *n]);
};
```

When an immutable pointer to an element is needed use `= *`:
```py
def nums = mut [1, 2, 3];
for (i, n) = * in nums => println("&nums[%] = %", [i, n]);
```

You obviously only can do that on mutable arrays/slices:
```py
def nums = [1, 2, 3];
for (i, n) = mut in nums => ...; # error: nums is immutable
```

You can mark the array/slice as `rev` for reverse iteration:
```py
def nums = [1, 2, 3];
for (i, n) in rev nums => println("nums[%] = %", [i, n]);
```

`soa` arrays and slices can't be iterated through. Do this instead:
```py
def Vec2 : (x, y = f32);
def pos = soa []Vec2 [(1.0, 2.0), (3.0, 4.0), (5.0, 6.0)];
for i in 0..@len(pos) => do_stuff(pos[i].x);
for i in 0..@len(pos) => do_stuff(pos[i].y);
```

#### Iterators
Iterators are just types that are part of the `iterator` [class](#Classes).

An iterator has to have one or more of the methods: `next`, `mnext`, `rnext` and `rmnext`.
- If it has `next` it'll work on normal `for` loops
- if it has `mnext` it'll work on mutable `for` loops
- if it has `rnext` it'll work on reversed `for` loops
- if it has `rmnext` it'll work on reversed mutable `for` loops

You don't need all of the methods to be implemented, just the ones you want to support.

If it has `next` or `mnext` a `has_next` is mandatory. And if it has `rnext` or `rmnext` a `has_rnext` is mandatory.

The `next`, `mnext`, `has_next`, `rnext`, `rmnext` and `has_rnext` methods should look like this:
```py
iterator <- Some_Iter;
def Some_Iter.next : (self = *Some_Iter) (i64, *<some-type>) => ...;
def Some_Iter.mnext : (self = *mut Some_Iter) (i64, *mut <some-type>) => ...;
def Some_Iter.has_next : (self = *Some_Iter) bool => ...;
def Some_Iter.rnext : (self = *Some_Iter) (i64, *<some-type>) => ...;
def Some_Iter.rmnext : (self = *mut Some_Iter) (i64, *mut <some-type>) => ...;
def Some_Iter.has_rnext : (self = *Some_Iter) bool => ...;
```
`<some-type>` can be of any type you want. The index type can be any type of integer that you want.

If the type isn't part of the `iterator` class, you can add an `iter` method. This way you can still put a variable with that type on a for loop.

The `iter` method should look like this:
```py
def Some_Type.iter : (self = *Some_Type) Some_Iter => ...;
```

Iterators on `for` loops are similar to arrays/slices, the difference is that the element will always be a pointer:
```py
def nums = Linked_List.make[1, 2, 3];
for (i, n) in nums => println("nums[%] = %", [i, *n]);
for (i, n) in rev nums => println("nums[%] = %", [i, *n]);
def nums_mut = mut Linked_List.make[1, 2, 3];
for (i, n) = mut in nums_mut => {
  *n = 2;
  println("nums[%] = %", [i, *n]);
};
```

### Compile-time control flows
Any control flow expression can be ran at compile-time, just prefix it with `@` and ensure to only use compile-time known values in it:
```py
@if SOME_CONST => ...;
else SOME_OTHER => ...;
else => ...;

@switch SOME_NEW_CONST (
  case0 => ...;
  case1 => ...;
  or    => ...;
);

@while SOME_CONST => ...;

@for i in 0..9 => ...;
```

And all of them can be ran in [module-scope](#Module-scope).

All compile-time loops are unrolled. And all false `if`/`else`/`switch` conditions aren't included on the final code. They're still checked for errors though.

## Error handling
Functions can return an error union instead of the specified return type using `||`:
```py
def DivErr : (By_Zero|);
def div : (a = i32, b = i32) i32 || DivErr => b == 0 ? .By_Zero : a/b;
```

This will implicitly create a temporary union for usage on error handling.

If both the normal return value and the error return value are unions you need to specify the union name on the tag. This avoids ambiguity:
```py
def Color : (Red|Green|Blue|Invalid);
def Error : (Invalid);
def colorize : (r, g, b = bool) Color || Error =>
  if   r && (g || b) => Error.Invalid;
  else g && (r || b) => Error.Invalid;
  else b && (r || g) => Error.Invalid;
  else               => r ? Color.Red : g ? Color.Green : b ? Color.Blue : Color.Invalid;
```

Otherwise it's safe to just use the `.<tag>` shortcut:
```py
def Error : (Invalid|None);
def colorize : (r, g, b = bool) u32 || Error =>
  if   r && (g || b) => .Invalid;
  else g && (r || b) => .Invalid;
  else b && (r || g) => .Invalid;
  else               => r ? 0xff0000 : g ? 0x00ff00 : b ? 0x0000ff : .None;
```

If that's the case you can't just call the function normally, you have to handle the error.

### The 'or' operator
The `or` operator is the way to handle errors returned by functions. If no errors occur the expected value is returned from the expression, if errors do occur a the value is returned from the right-hand side of the `or` operator:
```py
def res = div(10, 0) or 0;
```

The value returned from the right-hand side has to have the same type of the left-hand side or void. If the right-hand side returns void while the other side doesn't the program crashes:
```py
def res = div(10, 0) or this_returns_void(); # will crash
def res = div(10, 0) or 0.0; # compile-time error
```

If the `or` rhs is empty the program will crash if it reaches it as well:
```py
def foo : (error = bool) void || Some_Error => if error => Some_Error.Occurred;
foo(false) or; # fine
foo(true) or this_returns_void(); # will not crash, both sides return void
foo(true) or; # will crash, rhs of 'or' is empty
```

You can think of this as an implicit exit [syscall](https://en.wikipedia.org/wiki/System_call) being added automatically. Because that's exactly what's happening.

If you use `ret` or `brk` you actually avoid the exit call, no crash occur, but the expressions will still work like usual:
```py
{
  foo(true) or brk;
  println("never printed");
}
println("no crash!");
```

If you're sure that an error will never occur use the `nothing` keyword. This get rid of the check:
```py
def res = div(10, 2) or nothing;
```

If an error does occur the variable will hold garbage:
```py
def res = div(10, 0) or nothing; # res holds stack garbage now
```

Even if the variable was already defined:
```py
def res = 0;
res = div(10, 0) or nothing; # res holds stack garbage now
```

Because of this `or nothing` is extremely unsafe. Use only when you're certain that no errors will occur, if not just stick with the simpler and safer `or;`.

An example of safe use of `or nothing` for optimization:
```py
def x = 0;
for i in 1..10 => x += div(x, i) or nothing;
```

You can get the error value with the `or(<name>)` syntax, this will create a temporary variable that lives for the duration of the rhs expression:
```py
res = div(10, 0) or(e) if e == .ByZero => println("division by zero"); # will crash
```

`or` is an operator, and as such chaining is possible:
```py
this() or that() or(e) println("error: %", [e]);
```

### Options
An option is just a type that can have the value `null` instead of a valid value. Internaly the type just gains a flag value telling if it's null or not. To create an option add `?` as a prefix to the type:
```py
def x = ?i32 null;
```

The default value of that type is now `null`:
```py
def x = ?i32; # x == null
```

You can only use an optional variable if it's clear to the compiler that said variable isn't null:
```py
def x = ?i32 maybe_number();
def y = i32 x; # error: 'x' may be null
```

The correct way to handle an optinal variable is to check if it's `null` first
```py
def x = ?i32 maybe_number();
def y = i32;
if x != null => y = x; # valid
else => y = 0;
```

You can use the `or` operator on options. This will make the check automatic:
```py
def x = ?i32 maybe_number();
def y = x or 0;
```

The same `or` rules established earlier applies here. `or;` will crash, mismatched type will cause an error and void return will crash:
```py
def x = ?i32 maybe_number();
def _ = x or; # maybe crash
def _ = x or 0.0; # error: mismatched type
def _ = x or returns_void(); # maybe crash
def _ = x or nothing; # stack garbage if 'x' is null
```

#### Optional pointer
An optional pointer doesn't add an aditional flag, instead it just treats the value `0` as null. All the other constraints still applies:
```py
def p = ?*i32 maybe_reference();
def _ = *p; # error: 'p' maybe null
def _ = *p or; # maybe crash
def _ = *p or 0.0; # error: mismatched type
def _ = *p or returns_void(); #maybe crash
def _ = *p or nothing; # derreferecing null pointer if 'p' is null
```

## Precedence table
| Precedence | Operator | Description                                | Associativity |
|------------|----------|--------------------------------------------|---------------|
| 0          | @        | At compile-time operator                   | Right to Left |
| 1          | or       | Or error operator                          | Left to Right |
| 2          | func()   | Regular function call                      | Left to Right |
| 2          | func x   | One argument function call                 | Left to Right |
| 2          | x func y | Infix function call                        | Left to Right |
| 2          | ++       | Suffix increment                           | Left to Right |
| 2          | --       | Suffix decrement                           | Left to Right |
| 2          | []       | Indexing                                   | Left to Right |
| 2          | .        | Member access                              | Left to Right |
| 2          | Struct() | Struct literal                             | Left to Right |
| 3          | ++       | Prefix increment                           | Right to Left |
| 3          | --       | Prefix decrement                           | Right to Left |
| 3          | +        | Unary plus                                 | Right to Left |
| 3          | -        | Unary minus                                | Right to Left |
| 3          | !        | Logical not                                | Right to Left |
| 3          | ~        | Bitwise not                                | Right to Left |
| 3          | ^        | Get union value (suffix)                   | Left to Right |
| 3          | ^        | Get union tag number (prefix)              | Right to Left |
| 3          | *        | Pointer dereferencing                      | Right to Left |
| 3          | &        | Address of                                 | Right to Left |
| 3          | &mut     | Mutable address of                         | Right to Left |
| 3          | ->       | Safe cast between types                    | Left to Right |
| 3          | !        | Safe automatic cast between types          | Left to Right |
| 4          | *        | Multiplication                             | Left to Right |
| 4          | /        | Division                                   | Left to Right |
| 4          | %        | Modular                                    | Left to Right |
| 5          | +        | Addition                                   | Left to Right |
| 5          | -        | Subtraction                                | Left to Right |
| 6          | <<       | Bitwise shift left                         | Left to Right |
| 6          | >>       | Bitwise shift right                        | Left to Right |
| 7          | >        | Greater than                               | Left to Right |
| 7          | <        | Less than                                  | Left to Right |
| 7          | >=       | Greater or equals than                     | Left to Right |
| 7          | <=       | Less or equals than                        | Left to Right |
| 8          | ==       | Equals                                     | Left to Right |
| 8          | !=       | Not equal                                  | Left to Right |
| 9          | &        | Bitwise and                                | Left to Right |
| 10         | \|       | Bitwise or                                 | Left to Right |
| 11         | ^        | Bitwise xor                                | Left to Right |
| 12         | &&       | Logical and                                | Left to Right |
| 13         | \|\|     | Logical or                                 | Left to Right |
| 14         | ?:       | Ternary conditional                        | Right to Left |
| 15         | :        | Constant assignment                        | Right to Left |
| 15         | =        | Variable assignment                        | Right to Left |
| 15         | +=       | Variable assignment by sum                 | Right to Left |
| 15         | -=       | Variable assignment by difference          | Right to Left |
| 15         | *=       | Variable assignment by product             | Right to Left |
| 15         | /=       | Variable assignment by quotient            | Right to Left |
| 15         | %=       | Variable assignment by modular             | Right to Left |
| 15         | <<=      | Variable assignment by bitwise left shift  | Right to Left |
| 15         | >>=      | Variable assignment by bitwise right shift | Right to Left |
| 15         | &=       | Variable assignment by bitwise and         | Right to Left |
| 15         | \|=      | Variable assignment by bitwise or          | Right to Left |
| 15         | ^=       | Variable assignment by bitwise xor         | Right to Left |
| 16         | ,        | Comma                                      | Right to Left |
| 16         | |        | Union pipe                                 | Right to Left |

## Overloads
An overload is just a set of functions. If a function is on an overload it can be called via the overload itself.

### Overload definitions
The syntax of an overload is more complicated than the previous ones. First you open and close square brackets `[]`, inside the square brackets you create type patterns. After the pattern brackets you open and close parenthesis `()`, that's where you're going to put the argument list pattern. And lastly, comes the return pattern. An example of a simple overload would be:
```py
def some_overload : [](_+) +||+;
```

#### Type patterns
A type pattern determines a pattern that can be used on variable argument or return type. The type pattern is defined in a very similar way to a [variable](#Variable-definitions), but it can't have a value. You can put any type or class on a type pattern, the `_` identifier (that would mean any type) or a previously defined type pattern.
```py
def some_overload : [a = _, b = u32, c = a](a, b) c||+;
```

The type pattern represents a type, it can also be a pointer, slice, view or array (the array length needs to be a constant integer or `_`):
```py
def some_overload : [
  a = *_,
  b = **_,
  c = *mut _,
  d = [3]_,
  e = [_]_,
  f = [..]_,
  g = [..]mut _,
  h = [*]_,
  i = [*]mut _
](a, b, c, d, e, f, g, h) i||+;
```
This isn't all the possible combinations, because the possible combinations are infinite, like on variables where you can have a pointer to a slice of arrays of length 3 of mutable views to some type.

This isn't exclusive to the `_` identifier. These type attributes can be used on previously defined type patterns or real external types:
```py
def some_overload : [
  a = *u32,
  b = **a,
  c = *mut b,
  d = [3]i32,
  e = [_]d,
  f = [..]f,
  g = [..]mut f32,
  h = [*]number,
  i = [*]mut h
](a, b, c, d, e, f, g, h) i;
```

If a type pattern name collides with an external type, the type pattern takes priority:
```py
def some_overload [a = u32, u32 = f32, b = u32](a, u32) b||+; # 'a' will be an 'u32' type and 'u32' will be an 'f32' type. because 'b' is defined after 'u32', 'b' will actually be a 'f32' type.
```

Type patterns must be used inside of the arguments list or the return pattern. Differently from variables and constants, you can't silence the error by putting an underscore as a prefix on the type pattern.

#### Argument list pattern
The argument list pattern represents how the variable part of an argument list accepted by the overload should look like. An argument pattern can be compused by the following (`a` and `b` represents any type patterns):
- `_`: Any type can be placed there
- `a`: Only types that have the same composition as the type pattern can be placed there
- `<pat0>|<pat1>`: both patterns can be placed there. `<pat0>` and `<pat1>` can be of any argument pattern
- `<pat>*`: that pattern can be placed there 0 or more times. `<pat>` can be of any argument pattern
- `<pat>+`: that pattern can be placed there 0 or 1 time. `<pat>` can be of any argument pattern
- `_-<pat>`: Any type expect does bounded to a specific pattern. `<pat>` can be of any argument pattern

The precedence of patterns is:
1. `a` and `_`
2. `_-`
3. `*` and `+`
4. `|`

You can always wrap things in parentheses to make the pattern take priority.

If the argument list pattern is empty, the overload only accept functions with no arguments.

__Examples:__
Accepts `a` type, then any amount of any type until it finds the same `a` type again:
```py
def some_overload : [a = _](a, _*, a) +||+;
```

Accepts any type excluding `a`:
```py
def some_overload : [a = number](_-a) +||+;
```

Accepts `a` or `b` type or no types:
```py
def some_overload : [a = u32, b = f32]((a|b)+) +||+;
```

Accepts any amount of `a` types or `b` one b type:
```py
def some_overload : [a = u32, b = f32](a*|b) +||+;
```

Accepts any amount of `a` types or `b` type one or zero times:
```py
def some_overload : [a = u32, b = f32](a*|b+) +||+;
```

Accepts any amount of any type or `b` type one or zero times, then any amount of any type, then a `a` type:
```py
def some_overload : [a = u32, b = f32](_*|b+, _*, a) +||+;
```

As said before a variable pattern represents a type in specific. So when you reuse the same type pattern in an overload, it means that the types of both arguments have to be equivalent:
```py
def some_overload : [a = _](a, a) +||+;
```
The overload above only accept functions that have only two arguments, where both arguments are the same type.

#### Return pattern
The return pattern represents how the return type of a function accepted by the overload should look like. A return pattern can have one of four values (`a` represents any type pattern):
- `a`: only types that have the same composition as the type pattern can be place there
- `_`: any type can be placed there, excluding void
- `+`: any type can be placed there, including void
- `void`: only void can be placed there

The return pattern have two values, the return type and the error type. They're separated by `||`, on the left is the return type and on the right is the error type; like in real functions. The error type can only accept type patterns that have `_` or an union as it's value:
```py
def some_overload0 : []() +||+;
def some_overload1 : []() void||void;
def some_overload2 : []() _||_;
def some_overload3 : [a = _]() a||a;
def some_overload4 : [a = *_]() a||a; # error: pattern on return error pattern isn't valid
```

#### Putting fuctions directly into an overload
You can put functions in an overload directly in it's creation. After the overload definition add parenthesis, inside does parenthesis put all functions that you want. If they don't follow the constraints established by the overload an error will occur:
```py
def sum2_i32 : (a, b = i32) i32 => a + b;
def sum3_i32 : (a, b, c = i32) i32 => a + b + c;
def sum2_f32 : (a, b = f32) f32 => a + b;
def sum3_f32 : (a, b, c = f32) f32 => a + b + c;
def sum : [a = number](a*) a||void (sum2_i32, sum3_i32, sum2_f32, sum3_f32);
```

And because functions are first-class citizens you can pass function literals to `ovl(<funcs>)` directly:
```py
def sum : [a = number](a*) a||void (
  (a, b = i32) i32 => a + b,
  (a, b, c = i32) i32 => a + b + c,
  (a, b = f32) f32 => a + b,
  (a, b, c = f32) f32 => a + b + c,
);
```

#### Overload type
An overload has the `ovl` type, so you can defined it with an explicit type like this:
```py
def some_overload : ovl [](_*) +||+;
```
The overload above accepts any function.

#### Simple overload construct
If you don't care about constraints you can use the `ovl(<funcs>)` construct, that way you can pass any function to it (ambiguities still produce error):
```py
def sum2_i32 : (a, b = i32) i32 => a + b;
def sum3_i32 : (a, b, c = i32) i32 => a + b + c;
def sum2_f32 : (a, b = f32) f32 => a + b;
def sum3_f32 : (a, b, c = f32) f32 => a + b + c;
def sum : ovl(sum2_i32, sum3_i32, sum2_f32, sum3_f32);
```

This is sugar for:
```py
def sum2_i32 : (a, b = i32) i32 => a + b;
def sum3_i32 : (a, b, c = i32) i32 => a + b + c;
def sum2_f32 : (a, b = f32) f32 => a + b;
def sum3_f32 : (a, b, c = f32) f32 => a + b + c;
def sum : ovl [](_*) +||+ (sum2_i32, sum3_i32, sum2_f32, sum3_f32);
```

Putting anonymous functions into the construct still aplies here:
```py
def sum : ovl(
  (a, b = i32) i32 => a + b,
  (a, b, c = i32) i32 => a + b + c,
  (a, b = f32) f32 => a + b,
  (a, b, c = f32) f32 => a + b + c,
);
```

### Adding functions to overloads
To add a function to an overload use the `<-` operator:
```py
def math : [](_*) +||+;
def sum3 : (a, b, c = i32) i32 => a + b + c;
math <- sum3;
def sum2 : (a, b = i32) i32 => a + b;
math <- sum2;
def square : (x = i32) i32 => x * x;
math <- square;
def a = math(5, 5, 5) # a == 15
def b = 5 math 5 # b == 10, infix calls are possible
def c = math 5; # c == 25, one-parameter calls are possible
```

Because functions are just compile-time values, you can assign them directly to an overload:
```py
def sum : [a = _](a*) a||void;
sum <- (a, b = i32) i32 => a + b;
sum <- (a, b, c = i32) i32 => a + b + c;
def a = sum(1, 2); # a == 3
def b = sum(1, 2, 3); # b == 6
```

### Conflicts and ambiguity on overloads
If two functions have the same variable argument types in the same order they can't be passed to the same overload. Even if their return types are distinct:
```py
def sum : [](_*) +||+;
sum <- (a, b = i32) i32 => a + b;
sum <- (a, b = i32) f32 => (a + b) -> f32; # error: arguments conflict with previous 'sum' overload
```

If an argument with a default value causes the ambiguity on the overload, it simply can't be put in the overload:
```py
def sum : [](_*) +||+;
sum <- (a, b = i32) i32 => a + b;
sum <- (a, b = i32, c = i32 0) f32 => a + b + c; # error: default value on argument 'c' causes ambiguity with previous 'sum' overload
```

### Constant arguments and overloads
You can only pass a function with constant arguments on overloads if that argument is `imp`:
```py
def sum : [a = _](a*) a||void;
sum <- (T : imp type is number, a, b = T) T => a + b;
sum <- (T : type is number, a, b, c = T) T => a + b + c; # error: overloads only accepts functions with constant arguments marked as 'imp'
```

Keep in mind that the more specific overload beats the more generic:
```py
def operation : [a = _](a*) a||void;
operation <- (a, b = i32) i32 => a - b;
operation <- (T : imp type is number, a, b = T) T => a + b;
def a = 1.0 operation 2.0; # a == 3.0
def b = 1 operation 2; # a == -1
```

### Operator overload
There are a set of builtin overloads that automatically bounds to some operators. These are it's definitions:
```py
def __add__ : [a = _](a, a) a||+; # plus '+'
def __sub__ : [a = _](a, a) a||+; # minus '-'
def __mul__ : [a = _](a, a) a||+; # multiplication '*'
def __div__ : [a = _](a, a) a||+; # division '/'
def __mod__ : [a = _](a, a) a||+; # modular '%'
def __addeq__ : [a = _, b = *_](b, a) void||+; # plus equals '+=', returns void
def __subeq__ : [a = _, b = *_](b, a) void||+; # minus equals '-=', returns void
def __muleq__ : [a = _, b = *_](b, a) void||+; # multiplication equals '*=', returns void
def __diveq__ : [a = _, b = *_](b, a) void||+; # division equals '/=', returns void
def __modeq__ : [a = _, b = *_](b, a) void||+; # modular equals '%=', returns void
def __addeqe__ : [a = _, b = *_](b, a) a||+; # plus equals '+=', returns expression value
def __subeqe__ : [a = _, b = *_](b, a) a||+; # minus equals '-=', returns expression value
def __muleqe__ : [a = _, b = *_](b, a) a||+; # multiplication equals '*=', returns expression value
def __diveqe__ : [a = _, b = *_](b, a) a||+; # division equals '/=', returns expression value
def __modeqe__ : [a = _, b = *_](b, a) a||+; # modular equals '%=', returns expression value
def __incp__ : [a = *mut _](a) void||+; # prefix increment '++', returns void
def __decp__ : [a = *mut _](a) void||+; # prefix decrement '--', returns void
def __incs__ : [a = *mut _](a) void||+; # suffix increment '++', returns void
def __decs__ : [a = *mut _](a) void||+; # suffix decrement '--', returns void
def __incpe__ : [a = *mut _](a) a||+; # prefix increment '++', returns expression value
def __decpe__ : [a = *mut _](a) a||+; # prefix decrement '--', returns expression value
def __incse__ : [a = *mut _](a) a||+; # suffix increment '++', returns expression value
def __decse__ : [a = *mut _](a) a||+; # suffix decrement '--', returns expression value
def __indg__ : [a = _, b = _, c = *_](c, b) a||+; # index get []
def __inds__ : [a = *mut _, b = _](a, b) a||+; # index set []
```

The operators that can be overloaded are limited to mathematical operations or data structure indexing. Do not abuse this feature.

## Classes
Classes aren't types like in other programming languages. Classes are just a compile-time group for types. They don't have any specific purpose and can be used to do whatever you want with them. But the main use is type filtering in generics.

To create a class simply sum up different types together:
```py
def Num32 : i32+u32;
```

You can add a type to a class at any time using the `<-` operator:
```py
def Num32 : i32+u32;
Num32 <- f32;
```

Adding new types into classes modify ([most of](#Unbounded-parent-class)) all later _and_ previous usages.

To create a class from a single type you can use the unary `+`. This is needed because just simply using the type would create a type alias:
```py
def Type_Alias : u32;
def Single_Class : +u32;
```

A class has the type `class`, and you can use the type explicitly:
```py
def Num32 : class i32+u32+f32;
def Single_Class : class +u32;
def Empty_Class : class +;
```

### Child and parent classes
You can also create classes from a parent class:
```py
def Integers : i8+u8+i16+u16+u32+i32+u64+i64+usize+isize;
def Floats : f32+f64;
def Numbers : Integers+Floats;
```

Using the `<-` operator with classes on classes is also possible:
```py
def Numbers : i8+u8+i16+u16+u32+i32+u64+i64+usize+isize;
def Floats : f32+f64;
Numbers <- Floats;
```

So to add multiple new types at once you need to create an anonymous class:
```py
def Integers : i8+u8+i16+u16+u32+i32+u64+i64;
Integers <- usize+isize; # an anonymous class is being created from 'usize+isize' 
```

The child class is bound to the parent one. So if you add a type to the parent class, it'll also be added to the child:
```py
def Integers : i8+u8+i16+u16+u32+i32+u64+i64;
def Floats : f32+f64;
def Numbers : Integers+Floats;
Integers <- usize+isize; # now 'Numbers' also have this types
```

#### Unbounded parent class
If you don't want the child class to be bound to the parent one, surround the parent class with pipes:
```py
def Integers : i8+u8+i16+u16+u32+i32+u64+i64;
def Floats : typefam(f32);
def Numbers : [Integers]+Floats;
Integers <- usize+isize; # 'Numbers' still don't have usize and isize
Floats <- f64; # 'Numbers' has f64 now
```

The `[<class>]` will have access to all types in that class until that moment. So, for example:
```py
def Integers : u8+u16+u32+u64;
Integers <- i8+i16+i32+i64;
def Numbers : [Integers]+f32+f64;
Integers <- usize+isize;
```
`Numbers` will have `u8`, `i8`, `u16`, `i16`, `u32`, `i32`, `u64` and `i64`, but it'll not have `usize` or `isize`.

### Intersection between classes
You can make a class be the intersection between two different classes with the `&` operator:
```py
def Readable : File+Buffer+str+cstr;
def Writable : File+Buffer+Socket;
def ReadWrite : Readable&Writable; # ReadWrite will have types 'File' and 'Buffer'
```

### Excluding types and class operations
You can subtract types from classes, this will produce a new class:
```py
def Some_Class : i32+u32+f32+f64;
def Num32 : Some_Class-f64;
```

You can even subtract types that aren't on that class, this will guarantee that that type will never be a part of the newly created class, even if the parent class add that type later:
```py
def Some_Class : i32+u32+f32;
def Num32 : Some_Class-f64;
Some_Class <- f64+u64; # Only 'u64' is added to 'Num32'
```

You can exclude entire classes from a class:
```py
def Integers : i8+u8+i16+u16+u32+i32+u64+i64;
def Signed : i8+i16+i32+i64;
def Unsigned : Integers-Signed;
```

You can create a class that only has an excluded type, or class, with the unary `-`:
```py
def Not_i32 : -i32;
Not_i32 <- u32+i32+u64+i64+i8+u8+i16+u16; # All types are added, except 'i32'
```

If you use parenthesis on a class creation you'll be temporarily creating an anonymous class, so this is possible:
```py
def Integers : i8+u8+i16+u16+u32+i32+u64+i64;
def Unsigned : Integers-(i8+i16+i32+i64);
```

So all basic `+` and `-` operations that you would expect, works.

### Compile-time types on class
Classes can accept [compile-time only types](#Compile-time-only-primitive-types). This include `class` itself:
```py
Compile_Time_Only : anyi+anyf+type+range+class;
```

Because classes are a compile-time only concept, there aren't specific rules for compile-time only types.

### The 'is' operator
The only purpose of the `is` operator is to identify if a type is part of a class. If it isn't a compile time error occur:
```py
def Num32 : i32+u32+f32;
u32 is Num32; # nothing happens
u64 is Num32; # error: 'u64' isn't part of the 'Num32' class
```

And you obviously can use `is` on anonymous classes:
```py
def Num32 : i32+u32+f32;
def Num64 : i64+u64+f64;
u32 is Num32+Num64; # nothing happens
u64 is Num32+Num64; # nothing happens
u16 is Num32+Num64; # error: 'u16' isn't part of the 'Num32+Num64' class
```

`is` generates a compile-time only expression, but it returns void, so it can be used anywhere `void` is expected.

### Class comparison
You can verify if a type is part of a class with the `<type>==<class>`. This will just return a boolean, `true` if `<type>` is inside of `<class>` and false otherwise:
```py
def Num32 : i32+u32+f32;
def a = u32 == Num32; # a == true
def b = u64 == Num32; # b == false
```

### Builtin classes
There are several builtin classes:
- `signed`: `i8+i16+i32+i64+isize`
- `unsigned`: `u8+u16+u32+u64+usize`
- `integer`: `signed+unsigned`
- `float`: `f32+f64`
- `number`: `integer+float`
- `string`: `str+cstr`
- `componly`: `class+type+spc+anyi+anyf+range+var+unkfn`, [function types](#Functions-are-just-values) and [compile-time struct](#Compile-time-only-structs) are also part of it
- `runonly`: `any-componly`
- `customtype`: All custom types (structs, unions, etc)
- `struct`: All structs
- `union`: All unions
- `any`: All types

Some of them are updated when custom types are added. Like `struct`, `union`, `any`, etc.

## Spaces
Spaces are a subset of the current [module](#Modules) into one common constant. Everything that's valid on the outside module is valid inside of a space. To define a space simply use curly brackets, everything inside it is part of the space:
```py
def some_space : {
  def sum : (x, y = i32) i32 => x+y;
  def sub : (x, y = i32) i32 => x-y;
};
```

To access a symbol from a space use the `<space>.<symbol>` syntax:
```py
some_space : {
  def sum : (x, y = i32) i32 => x+y;
  def sub : (x, y = i32) i32 => x-y;
};
def _ = some_space.sum(1, 2);
```

### Space privacy
Everything inside of a space is public by default, so the `pub` attribute doesn't really do anything. But you can mark them as `prv` to be only accessable inside the space itself:
```py
def some_space : {
  prv def mul : (x, y = f32) f32 => x+y;
  pub def dot : (x0,y0, x1,y1 = f32) f32 => x0 mul x1 + y0 mul y1; # 'pub' isn't really needed
};
def _ = some_space.dot(1.0, 2.0, 3.0, 4.0);
def _ = some_space.mul(1.0, 2.0); # error: 'mul' is private
```

### Space type
All spaces have the type `spc`, so you can define them explicitly:
```py
def some_space : spc {
  def foo : () =>;
  def bar : () =>;
};
```

### The 'use' overload
There's a bultin overload called `use`, it has two functions by default in it:
```py
def use : [a = spc, b = [..]meta.iden, c = meta](a, b+) c||void (
  (space = spc) meta =>...,
  (space = spc, symbols = [..]meta.iden) meta =>...,
);
```

The function that just takes a space will make binds for all the spaces symbols:
```py
def some_space : {
  def add : (x, y = i32) i32 => x+y;
  def sub : (x, y = i32) i32 => x-y;
  def mul : (x, y = i32) i32 => x*y;
  def div : (x, y = i32) i32 => x/y;
};
@use some_space;
```

`@use some_space` does actually this:
```py
def add : some_space.add;
def sub : some_space.sub;
def mul : some_space.mul;
def div : some_space.div;
```

And the function that takes a space and a slice of identifiers will make a bind only for those specific identifiers:
```py
def some_space : {
  def add : (x, y = i32) i32 => x+y;
  def sub : (x, y = i32) i32 => x-y;
  def mul : (x, y = i32) i32 => x*y;
  def div : (x, y = i32) i32 => x/y;
};
@use(some_space, [add, div]);
```

`@use(some_space, [add, div])` does actually this:
```py
def add : some_space.add;
def div : some_space.div;
```

If `@use` finds a conflict between the binding is trying to make and an already defined symbol it asserts:
```py
def some_space : {
  def add : (x, y = i32) i32 => x+y;
  def sub : (x, y = i32) i32 => x-y;
  def mul : (x, y = i32) i32 => x*y;
  def div : (x, y = i32) i32 => x/y;
};
def add : some_space.add;
@use some_space; # error: 'add' is already defined
```

## Modules
Every `.sk` file is a module. A module is a collection of symbols (global variables and constants).

The file name will be the module name, and as such it must follow the [identifier naming rules](#Identifier-naming-rules). If it don't it simply can't be used by other modules with the `ext` keyword or even compiled, effectively becoming useless. 

### External module symbols
You can get am external module symbol using the `ext` keyword. The `ext` keyword must be followed by a valid locatable module or bundle. Then you can get the symbols from it like if it was a [space](#Spaces):
```py
# external module called 'mod'
pub def ONE : 1;
# main module
def ONE : ext mod.ONE; # ONE == 1
```

An `ext` with just a module name is invalid:
```py
def ONE : ext mod; # error: no symbol provided in 'ext'
```

### Module privacy
Opposite from [spaces](#Space-privacy), module symbols are always private by default, so the `prv` attribute is the one that actually doesn't do anything. If you want a symbol to be visible and used by external modules add the `pub` attribute:
```py
# external module called 'mod'
prv def mul : (x, y = f32) f32 => x+y; # 'prv' isn't really needed
pub def dot : (x0,y0, x1,y1 = f32) f32 => x0 mul x1 + y0 mul y1;
# main module
def _ = ext mod.dot(1.0, 2.0, 3.0, 4.0);
def _ = ext mod.mul(1.0, 2.0); # error: 'mul' is private
```

### Importing
To import all of the public symbols of a module automaticaly use the builtin `import` function:
```py
# external module called 'mod'
pub def add : (x, y = i32) i32 => x+y;
pub def sub : (x, y = i32) i32 => x-y;
pub def mul : (x, y = i32) i32 => x*y;
pub def div : (x, y = i32) i32 => x/y;
# main module
def mod : @import "mod";
def a = mod.add(1, 2); # a == 3
```

What this function actually do is create a space with bindings to all public symbols of a module. So the `def mod : @import "mod"` above actualy translates to this:
```py
def mod : {
  def add : ext mod.add;
  def sub : ext mod.sub;
  def mul : ext mod.mul;
  def div : ext mod.div;
};
def a = mod.add(1, 2); # a == 3
```

Because the `import` function returns a space you can name it whatever you want. This is good to avoid conflicts:
```py
some_module : @import "mod";
def a = some_module.add(1, 2); # a == 3
```

You can even pass it to the [`use`](#The-use-overload) overload directly:
```py
@use @import "mod";
def a = add(1, 2); # a == 3
```

### Bundles
A bundle is just a directory with `.sk` Stark source files in it.

Similarly the file name of modules, the directory name will be the bundle name, and it must follow the [identifier naming rules](#Identifier-naming-rules). If it don't it simply can't be used with the `ext` keyword.

As said before: ["The `ext` keyword must be followed by a valid locatable module or bundle"](#External-module-symbols). If the `ext` is followed by a bundle them it must be followed by the specific module that you want:
```py
def _ = ext bundle.mod.ONE;
def _ = ext bundle; # error: no module provided in 'ext'
```

You can also use bundles on the `import` function:
```py
def mod : @import "bundle.mod";
def _  = mod.ONE; # valid
```

### Building
To compile a Stark project simply run in your shell:
```
stark <path/to/project/>
```

The Stark compiler will search for a `main.sk` inside the provided path.

In Stark there is no build system. The compiler automatically detect the correct modules to compile based on the `ext` expressions around your project.

There are two places that the compiler search for modules and bundles when you use `ext`:
1. Inside a subdirectory called `ext` located in the same directory as the `main.sk`. This directory is known as the local `ext`
2. Inside a subdirectory called `ext` located in the same directory as the stark compiler. This directory is known as the shared `ext`

If a bundle or module name collides with another one between the two `ext` directories, the local `ext` takes priority.

You can access the absolute path of both of those directories via two predefined constants:
- For the shared `ext`: `__ext_shared_path__`
- For the local `ext`: `__ext_local_path__`

#### Dynamic model loading
You can compile a project into a `.sko` file. `sko` stands for Stark Object file. This file has a header with metadata info about symbols and a shared library in it. You can later use it to hot reload code using the [`std.hot`](#Hot-reloading-module) module.

To compile a project into a `.sko` instead of an executable use the `-obj` flag.

### The base module
All of the bultin functions stated before, like `len`, `use` or `import`, aren't bultin into the language, but actualy part of a module on the [standard bundle](#Standard-bundle) called `base`. The following line is added by default at the top of every module:
```py
@ext std.base.use(@ext std.base.import("std.base"));
```

This uses the `use` overload and `import` function directly from the `std.base` module to import all symbols from `std.base`.

In other words, everything inside `std.base` is available by default.

Keep in mind that because that line is added by default and the local `ext` has priority over the shared `ext`, if you create your own `std` bundle it'll cause an error if it doesn't have a `base` module in it with `use` and `import` functions.

You can add the `-nobase` flag to the compiler, that way that line isn't added by default and you are free to not have a `base` module inside your custom `std` bundle.

### Dividing a module into multiple files
You can divide one module into multiple files by using a `.sks` Stark space file. You can import this files into the module using the builtin `include` function. The entire file will be put inside of a space:
```py
# math.sks
def add : (x, y = i32) i32 => x+y;
def sub : (x, y = i32) i32 => x-y;
def mul : (x, y = i32) i32 => x*y;
def div : (x, y = i32) i32 => x/y;
# main module
def math : @include "math";
```

This will literaly translate to:
```py
def math : {
  def add : (x, y = i32) i32 => x+y;
  def sub : (x, y = i32) i32 => x-y;
  def mul : (x, y = i32) i32 => x*y;
  def div : (x, y = i32) i32 => x/y;
};
```

The reason why `.sks` is used for the extension of those separate space files is to avoid confusion. There's one key difference between a space and a module and that is their default privace: All modules have their symbols as private by default and all spaces have their symbols as public by default.

The search path for those `.sks` files is the directory in which the module that they are being included to is located.

## Foreign function interface
You can get a symbol from a foreign dynamic or static library using the `unk` keyword, followed by a string with the name of said library and another string representing the name of said symbol:
```py
unk "libm" "sqrt"
```

The extension `.so`, `.dll`, etc; must not be included with the library name.

This will return a value with the type `unkfn`, that value can be assigned to a constant function of any type:
```py
def sqrt : fn(x = f64) f64 unk "libm" "sqrt";
```

The linker will automaticaly link to every library specified by every `unk`.

The linking path will be specified by the linker. But you can also create a directory named `unk`, that path will be automatically added by the linker. 

## Machine code capabilities in Stark
It's possible to embed binary data directly into stark code using the `embed` expression. This expression takes flags for the embeded data permission and a constant slice of `u8`s. It'll return a `*void` or a `*mut void` depending on the flags: 
`def data : embed rw [0x86, 0xff]`;

### Embed flags
This are the possible flags that `embed` accepts:
- `r`: Readable data
- `w`: Writable data
- `x`: Executable data
- `rw`: Readable writable data
- `rx`: Readable executable data
- `wx`: Writable executable data
- `rwx`: Readable writable executable data

If you don't have the appropriate flag for what you're trying to do with that data (read/write/execute/) a segmentation fault will occur.

### Executing embedded data
To execute data created using `embed` first you need to cast it to a [function pointer](#Function-pointers), then simply run the function:
```py
def add : embed x [0x48, 0x8D, 0x04, 0x37, 0xC3] -> *fn(a = u64, b = u64) u64;
def x = add(3, 4); # x == 7
```

### The bultin 'asm' function
There's a function on the `std.base` module called `asm`, this function takes intel assembly source code and returns a pointer to the assembly data. It internaly uses `embed` with the flag `x`. The valid assembly source will depend on the current platform (x86, x64, etc):
```py
def add : @asm $end
  lea rax, [rdi+rsi]
  ret
end -> *fn(a = u64, b = u64) u64;
def x = add(3, 4); # x == 7
```

### Manipulating registers
You can activaly read from and write to registers for your target platform using the `reg` keyword:
```py
reg rax = 10;
def x = u64 reg rax; # x == 10
```

The infered type of the variable will be based on the register: `rax` = `u64`, `eax` = u32, etc:
```py
def x = reg ax; # x is of type u8
```

This can pair up way of `embed` and `asm`.

## Standard bundle
### Base module
Work in progress...

### Mem module
NOTE: arena allocator and Arena_Array
Work in progress...

### I/O module
Work in progress...

### String module
NOTE: String builder
Work in progress...

### Hot reloading module
Work in progress...

### OS module
Work in progress...

