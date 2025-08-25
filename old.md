# Stark
A language for systems programming.

Current version: 0.0.1

## Hello, World!
```py
main ::= () => ext std.io.slow_println("Hello, world!");
```

## Philosophy
__Stark__, as it's name suggests, is a language that focus on:
- Robustness
- Explicitness
- No-magic behavior
- Zero overhead

The main focus of the language is performance and type safety.

## Expressions
There are no statements in Stark. Literally everything is an expression that returns something. If an expression returns [nothing](#The-nothing-type) it still is, in the end of the day, returning something.

Every expression that is not used to compose another expression should be terminated via a semicolon. 

Every expression that returns something other than _nothing_ needs to be used to compose another expression. If the expression returns _nothing_ it doesn't need to be used by other expressions.

Every [variable](#Variables) and [constant](#Constants) should be used inside at least one expression.

## Identifier naming rules
There are two ways of naming an identifier:
The primary way is: start with an underscore or an ASCII letter, followed by any amount of underscores, ASCII letters or numbers:
```py
a         # valid
_a        # valid
abc__0123 # valid
0a        # invalid
ça        # invalid
a+        # invalid
```

The other way is using backticks. In between them you can put any unicode symbol:
```py
`a`  # valid
`0a` # valid
`a+` # valid
`ça` # valid
`π`  # valid
```

With the backticks even keywords become available:
```py
`if` # valid
`do` # valid
`is` # valid
```

Spaced identifiers are also possible:
```py
`that thing`      # valid
`player velocity` # valid
```

Empty backticks is also a valid identifier:
```py
`` # valid
```

Keep in mind that backticks are just a way to refer to an identifier, so if an identifier is exactly the same with and without backticks it is the indeed the same identifier:
```py
`player` # same as 'player'
player   # same as '`player`'
```

[The `_` identifier is reserved](#Unused-variables-and-the-_-identifier)

## Functions
### Function definition and calling conventions
Functions are defined by using an identifier, followed by the constant definition and assignment operator `::=`, then the argument list `(arg0 : type0, arg1 : type1, ...)`, next the return type, followed by the guard body symbol `=>` and finally an expression, that expression is the function body:
```py
func_name ::= (arg0 : type0, arg1 : type1, ...) return_type => func_body_expr;
```

Functions always returns the expression at the function body result.

A basic function that returns a sum of two numbers would be defined as:
```py
sum ::= (a : i32, b : i32) i32 => a + b;
```

To call a regular function:
```py
foo ::= (_a : i32, _b : i32) =>;
foo(10, 12);
```

But if it takes exactly one parameter the parenthesis are optional:
```py
foo ::= (_x : i32) =>;
foo 10;
```

Trailing commas are allowed:
```py
foo ::= (x : i32, y : i32, z : i32,) i32 => x+y+z;
_ := foo(1, 2, 3,);
```

### Unused arguments
Unused arguments causes a compile-time error:
```py
sum ::= (a : i32, b : i32, c : i32) i32 => a + b; # error: 'c' is unused
```

To supress the error put an underscore as a prefix on the argument name:
```py
sum ::= (a : i32, b : i32, _c : i32) i32 => a + b; # valid
```

### Return nothing
If a function is supposed to not return anything then simply omit the return type:
```py
foo ::= () =>;
```

You're basically returning _nothing_, so you can use [the nothing type](#The-nothing-type) more explicitly using [backticks](#Identifier-naming-rules):
```py
foo ::= () `` =>;
```

### Multiple return values
Multiple return values aren't possible, but you can use a [struct](#Structs) instead:
```py
div ::= (a, b : i32) (i32, bool) => if b == 0 do (0, false) else (a/b, true);
(is_div_by_zero, res) := div(4, 2);
```

### Expression blocks
For functions with more then one expression use an expression block.
```py
foo ::= () => {
  expr0;
  expr1;
};
```

If an early break from the expression block is required use the `brk` keyword:
```py
do_more_stuff ::= () => {
  do_stuff();
  do_stuff();
  brk;
  do_stuff(); # will not be executed
};
```

By default expression blocks [return nothing](#Return-nothing). Use the `brk` keyword followed by one expression to break from the expression block with a return value:
```py
do_stuff_and_sum ::= (x : i32, y : i32) i32 => {
  do_stuff();
  do_stuff();
  brk x + y;
};
```

Because expression blocks are also just expressions they can be nested:
```py
foo ::= () => {
  { };
  { };
};
```

#### Break and return from expression blocks
Keep in mind that the `brk` keyword only breaks from the current expression [block-scope](#Scoping):
```py
foo ::= () => {
  {
    brk;
    do_stuff(); # will not be executed
  };
  do_stuff(); # will be executed
};
```

If you want to return from all nested expression blocks at once use the `ret` keyword:
```py
foo ::= () => {
  {
    ret;
    do_stuff(); # will not be executed
  };
  do_stuff(); # will not be executed
};
```

And of course, `ret` also can have an expression as a return value:
```py
five ::= () i32 => {
  {
    ret 5; # 5 is returned from the function
    do_stuff(); # will not be executed
  };
  do_stuff(); # will not be executed
};
```

The `brk` expression can be nested:
```py
foo ::= () => {
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
foo ::= () => {
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

And of course you can add non-`brk` expressions to it. Again this is not special semantic, it's all the same thing:
```py
foo ::= () => {
  x := {
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
foo ::= () i32 => ret 10; # error: 'ret' outside expression-block
```

### Defer expression
You can defer an expression to the end of an expression-block using the `defer` keyword. The deferred expression will run even if you used `brk` or `ret` to early return from an expression-block:
```py
main ::= () => {
  defer io.slow_println("end");
  io.slow_println("start");
};
```

The output will be:
```
start
end
```

The order of multiple defers is based on their definitions. The last defined runs first:
```py
main ::= () => {
  defer io.slow_println("1");
  defer io.slow_println("2");
  defer io.slow_println("3");
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
main ::= () => {
  defer io.slow_println("1");
  {
    defer io.slow_println("2");
    io.slow_prinln("3");
    brk defer io.slow_println("4");
  };
  defer io.slow_println("5");
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
fn0 ::= (_x : i32) =>;
fn1 ::= (_x, _y : i32, _z : f32) =>;
fn2 ::= (_x : i32 = 1) =>; # default values have to be compile-time constants
fn3 ::= (_x, _y : i32 = 1) =>;
```

The only exception is inferred types by value:
```py
foo ::= (_x := 0) =>; #error: argument with no explicit type
```

Because function arguments are variables they also are [immutable](#Variable-mutability) by default. Make them mutable with the `mut` modifier:
```py
foo ::= (_x : i32) => _x = 10; # invalid, '_x' is immutable
foo ::= (_x : mut i32) => _x = 10; # valid, '_x' is mutable
```

Functions can also have constant arguments:
```py
foo ::= (num :: u64) u64 => num;
```

That means that you can only pass constant parameters to those arguments:
```py
foo ::= (num :: u64) u64 => num;
x := foo(10); # valid
y := foo(x); # invalid, x isn't a constant
```

Constant arguments always have to be the first ones in a function:
```py
foo ::= (variable : i32, constant :: u64) =>; # invalid
foo ::= (constant :: u64, variable : i32) =>; # valid
```

Constant arguments are the only place where you don't need to specify the constant value on definition (see more on [the constants section](#Constants)):
```py
foo ::= (num :: u64) u64 => num;
```
So the code above doesn't actually make a type aliases, instead it makes an u64 constant.

Actually constant arguments _can't_ have a default constant value:
```py
foo ::= (num :: u64 10) u64 => num; # error: constant argument with default value
```

Keep in mind that non-compile-time functions with constant arguments, generates a new function every time they are used with a different constant value through out your program. This can lead to more 'bloated' executables, but the performance is not affected. 

### Compile-time functions
If a function only accept constant parameters and do not access external non-constant data, it's garanteed to run at compile time.
```py
comp_sum ::= (a, b :: i32) i32 => a + b;
x := comp_sum(12, 4); # 16 will be computed at compile-time
```

It's actually possible to run any function, that don't access external non-constant data, at compile time if you pass compile-time known values as its arguments and use the `@` operator:
```py
sum ::= (a, b : i32) i32 => a + b;
x := @sum(12, 4); # 16 will be computed at compile-time despite 'sum' accepting variable parameters
```

If a function is only run at compile-time, and no function pointers are taken from it, it'll not be included on the binary. 

You actually can pass a variable to a function ran at compile-time if the function do not read or write to it:
```py
taking_a_variable_for_some_reason ::= (_a = u32) =>;
a := get_some_u32();
@taking_a_variable_for_some_reason(a); # valid, will be run at compile-time (in this case just optimised-out)
```

You generally can't run functions at [module-scope](#Scoping), using the `@` operator you can:
```py
foo ::= () => do_something();

foo(); # invalid
@foo(); # valid

main ::= () =>;
```

Keep in mind that the `@` operator means "at compile time". So functions ran at [module-scope](#Scoping) will be compile-time, and as such have to follow the non-constant data access and only constant parameters rules.

It's also possible to ensure that a function will always be run at compile-time using `@` as a prefix on the function literal:
```py
comp_sum ::= @(a, b :: i32) i32 => a + b;
x := @comp_sum(12, 4); # 16 will be computed at compile-time
```

Ensured-compile-time functions always have to be called via `@`:
```py
comp_sum ::= @(a, b :: i32) i32 => a + b;
x := @comp_sum(12, 4); # valid
x := comp_sum(12, 4); # invalid
```

If any compile-time function rules are broken it'll generate a compile-time error.

Ensured-compile-time function gain access to variables and return values of [compile-time-only types](#Compile-time-only-primitive-types):
```py
type_of ::= @(T :: imp type, _ : T) type => T;
a : u32;
b := @type_of(a) 10; # valid
```

A compile-time function can call another compile-time function inside of it. Compile-time functions aren't some macro-like expansion thing, they literally run at compile-time. So calling a compile-time function inside of a compile-time function have the same behavior of calling a normal function. With that recursion is fine:
```py
comp_fac ::= @(x :: u32) u32 => if x == 0 do 1 else x * @comp_fac(x-1);
```

The compile-time phase of the compiler is an interpreter of Stark byte-code. That's why this kind of stuff is fine.

### Generic functions
With constant arguments you can make generic functions using the `type` type:
```py
sum ::= (T :: type, a, b : T) T => a + b;
```

It's also possible to use the [`is`](#The-is-operator) operator. A compile-error will occur if `is` returns `false`, but if `is` returns `true` the constant parameter will hold the proper type:
```py
sum ::= (T :: type is number, a, b : T) T => a + b;
a := sum(i32, 1, 2); # 'T' will be set to 'i32'
b := sum(str, "one", "two"); # error: type 'str' isn't part of the class 'number'
```

### Unordered arguments
Another way to call a function is with directly naming the parameter to which you want to pass the argument:
```py
foo ::= (a, b, c : i32) i32 => a+b+c;
_ := foo(.c = 10, .a = 4, .b = -15);
```

Works even for constant parameters:
```py
foo ::= (T :: type, a, b, c : T) T => a+b+c;
_ := foo(.c = 10, .a = 4, .b = -15, .T = f32);
```

This way you can leave the default arguments untouched:
```py
foo ::= (a : i32, b : i32 = 10, c : i32) i32 => a+b+c;
_ := foo(9, .c = 11);
```

If you use unordered arguments to just skip an argument it's possible to continue to use the function call order:
```py
foo ::= (a : i32, b : i32 = 10, c, d : i32) i32 => a+b+c;
_ := foo(9, .c = 11, 12); # a == 9, b == 10, c == 11, d == 12
```

With that in mind if a parameter has a default the argument is optional:
```py
foo ::= (a, b : i32, c : i32 = 10) i32 => a+b+c;
_ := foo(9, 11);
```

You can only mix named and unnamed arguments to skip parameters. So if this mix occur, and the named arguments don't actually follow the function order specified on definition, it'll cause an error:
```py
foo ::= (a : i32, b : i32 = 10, c, d : i32) i32 => a+b+c;
_ := foo(9, .c = 11, .b = 12); # error: invalid order
```

This calling convention is the reason why [function types have their parameters names in it](#Functions-are-just-values).

### Implicit constant parameters
If a constant parameter is used in some way by other parameters you can add the `imp` attibute to it. This will make that argument be implicitly defined based on it's first use:
```py
add ::= (T :: imp type is number, a, b : T) T => a+b;
x := add(1, 2);
y := add(1.6, 2.1);
z := add(1.6, 2); # error: an i32 is being passed to 'b' and expected an f32
```

If a constant argument is marked as `imp` it has to be used on at least one other non-imp argument:
```py
foo ::= (T :: imp type is number, a, b : i32) i32 => a+b; # error: 'T' has to be used on other arguments
```

You can still set the value of the constant parameter explictly using the unordered parameter function call:
```py
add ::= (T :: imp type is number, a, b : T) T => a+b;
x := add(.T = u64, 1, 2);
```

One-paremeter calls can be used with more than one paremeters if the extra ones are constant `imp` arguments:
```py
square ::= (T :: imp type is number, a : T) T => a*a;
_ := square 5; # valid
_ := square 5.5; # valid
```

### Immutable arguments are references
Immutable arguments that have a value bigger than a register size (usually 8 or 4 bytes) are passed as references (implicit pointer). That way copies are not made.

So you don't need to explicitly pass immutable pointers around for big structs, like you would do it in C. It is actually more performant if you don't use immutable pointers to just pass values because inlining becomes a possibility:
```py
foo ::= (x : *Big_Struct) Something => x.something;
bar ::= (x : Big_Struct) Something => x.something;
big : Big_Struct;
_ := foo(&big);
_ := bar(big); # performance for this is the same as 'foo' or better if inlined
```

It is not possible to take the address of immutable arguments because they actually can be references and that could be dangerous:
```py
foo := (x : Big_Struct) => do_something(&x); # error: can't take address of immutable variable
```

### Implicit address on argument
If the convenience of not taking the address for pointer arguments is desired (for instance, [operator overloading](#Operator-overload)), you can add the `&` attribute to a pointer argument. This signals that an implicit address will be taken:
```py
foo ::= (_x : &*Big_Struct) =>;
a := Big_Struct;
foo(a); # equivalent to foo(&a)
```

Because these arguments are just pointers, pointers can still be directly passed:
```py
foo ::= (_x : &*Big_Struct) =>;
a : Big_Struct;
ptr := &a;
foo(ptr); # valid
foo(&a); # valid
```

It also works for mutable pointers:
```py
foo ::= (x : &*mut Big_Struct) => x.something = get_something();
a : mut Big_Struct;
foo(a); # valid
```

### Variadic arguments
There is no variadic arguments syntax. Most of the time you actually just want to pass a [slice](#Slices) of a single type:
```py
print_nums ::= (nums : [..]i32) => for i in 0..<len(nums) => io.slow_println("%", [1]);
print_nums [1, 2, 3, 4];
```

But if you truly want to accept any type, for a custom printing function or something of that sort, use a slice of [anyrt](#The-anyrt-type). This is how `print` and `println` are implemented:
```py
custom_print ::= (fmt : str, args : [..]anyrt) => ...;
custom_print("% % % %", [1, 1.0, "1", [1, 1, 1]]);
```

But only use it if it's really necessary, `anyrt` costs more than just a specified slice.

### Function guards
You can have more than one body on a function. The guard body symbol `=>` accepts a pattern on its left-hand-side, this is called a guard. You separate guards by commas. A patternless `=>` is the fallback:
```py
foo ::= (x : i32) i32
  x > 0 => x - 1,
  x < 0 => x + 1,
        => x; # fallback
```

You can't have guards after the fallback:
```py
foo ::= (x : i32) i32
        => x, # error: patternless '=>' has to end with a semicolon
  x > 0 => x - 1,
  x < 0 => x + 1;
```

If at least one of the patterns isn't constant the patternless `=>` is obligatory:
```py
foo ::= (x : i32) i32
  x > 0 => x - 1,
  x < 0 => x + 1; # error: 'foo' has non-constant guard patterns, but no patternless '=>' fallback
```

If all the patterns are constant you don't need a fallback:
```py
foo ::= (CONST :: i32) i32
  CONST > 0 => x - 1,
  CONST < 0 => x + 1; # fine
```

When calling a function without fallback and no guards are met a compile-time error will occur:
```py
foo ::= (CONST :: i32) i32
  CONST > 0 => x - 1,
  CONST < 0 => x + 1;
_ := foo(0); # error: '0' doesn't match any guard pattern of 'foo'
```

Any boolean expression is a guard pattern:
```py
foo ::= (x, y : i32) bool
  y > 0 && x * x < y => true, # valid
                     => false;
not_foo ::= (x, y : i32) bool
  foo(x, y) => false, # valid, 'foo(x, y)' is an expression that return a boolean
            => true;
```

You can use the [`is` operator](#The-is-operator) as a guard pattern:
```py
sum ::= (T :: imp type, x, y : T) T
  T is number => x + y;
```

This will basically have the same effect of using the `is` operator directly on the `T` argument. But with guards you can actually have different bodies for different `is` conditions:
```py
sum ::= (T :: imp type, x, y : T) T
  T is number => x + y,
  T is string => x.concat y;
```

[Patterned destructuring](#Patterned-destructuring) can be used as patterns:
```py
Point3D ::= (x, y, z : i32);
foo ::= (p : Point3D)
  (0, y, z)      := p => do_this(y, z),
  (x, y > 10, z) := p => do_that(x, y, z);
                      => do_else();
bar ::= (a : [3]i32)
  [0, y, z]      := p => do_this(y, z),
  [x, y > 10, z] := p => do_that(x, y, z);
                      => do_else();
baz ::= (a : vec[3]i32)
  [0, y, z]      := p => do_this(y, z),
  [x, y > 10, z] := p => do_that(x, y, z);
                      => do_else();
```

Keep in mind that the guard body will only have the variables defined by its own pattern destructuring:
```py
foo ::= (p : Point3D)
  (0, a, b)      := p => do_this(a, b),
  (x, y > 10, z) := p => do_that(x, a, b); # error: variables 'a' and 'b' aren't defined
                      => do_else();
```

The first patterned matched will be executed, even if other patterns would also match:
```py
foo ::= (a, b : i32) i32
  a < 1 => b,
  b < 1 => a,
        => a + b;
_ := foo(1, 2); # fallback branch is ran
_ := foo(0, 2); # 'a < 1' branch is ran
_ := foo(1, 0); # 'b < 1' branch is ran
_ := foo(-1, -2); # 'a < 1' branch is ran even if branch 'b < 1' would also match
```

You can use something called arguments pattern, this is a list of the arguments constants and variables. Each elements of an arguments pattern can be a constant expression that can be compared with the parameter using the `==` operator, a destructuring, the name of the variable/constant or the [`_`](#Unused-variables-and-the-_-identifier) identifier:
```py
Vec2 ::= (x, y : f32);
add ::= (T ::= imp type, a, b : T) T
  (Vec2, (0.0, 0.0), (x1, y1)) => (x1, y1),
  (Vec2, (0.0, y0), (x1, y1)) => (x1, y0 + y1),
  (Vec2, (x0, 0.0), (x1, y1)) => (x0 + x1, y1),
  (Vec2, (x0, y0), (x1, y1)) => (x0 + x1, y0 + y1),
  (_, 0, b) => b,
  (_, a, 0) => a,
  (_, a, b) => a + b;
```

An arguments pattern with all its elements being non-literals is equivalent to a patternless guard, thus you can't have guards after it:
```py
add ::= (a, b : i32) i32
  (a, b) => a + b,
  (0, b) => b; # error: guard after catch-all guard '(a, b) =>'
```

If an argument on the arguments pattern is set to be the `_` identifier you can't use that argument:
```py
add ::= (a, b : i32) i32
  (a, _) => a + b; # error: can't use argument 'b' as stated by the guard pattern '(a, _) =>'
```

And if it's set to its proper name you need to use the argument on the guard body:
```py
add ::= (a, b : i32) i32
  (a, _) => 0; # error: not using argument 'a'
```

If a function has only one parameter you don't need the paranthesis on the arguments pattern:
```py
is_zero ::= (_x : i32) bool
  0 => true,
  _ => false;
```

The `_` identifier can be used as a suger for an arguments pattern that looks like: `(_, _, _, ...)`:
```py
are_both_zero ::= (_x, _y: i32) bool
  (0, 0) => true,
  _ => false;
```

### Functions are just values
Functions are just values assigned to [constants](#Constants) with function types. So you can create an anonymous function using a function literal and call it directly:
```py
num := ((x, y : i32) i32 => x + y)(10, 20); # num is assigned to 30
```

The type of a function looks something like this:
```py
(const_arg0 :: type0, const_arg1 :: type1, var_arg0 : type0, var_arg1 : type1) return_type
```

Here are some examples with functions and what the type will look like:
```py
square_i32 ::= (x : i32) i32                     => x*x;
square_any ::= (T :: type is number, x : T) T    => x*x;
sum_i32    ::= (x, y : i32) i32                  => x+y;
sum_any    ::= (T :: type is number, x, y : T) T => x+y;
set_any_x  ::= (T :: type, foo : T)              => foo.x = something();
# square_i32 type: (x : i32) i32
# square_any type: (T :: type, x : T) T
# sum_i32 type:    (x : i32, y : i32) i32
# sum_any type:    (T :: type, x : T, y : T) T
# set_any_x type:  (T :: type, x : T, y : T) ``
```

You can still ommit the type of a variable if the next variable is of the same type on function types:
```py
sum :: (x, y : i32) i32 = (x, y : i32) i32 => x + y; # valid
```

The type of the function above will still be `(x : i32, y : i32) i32`, this is just sugar.

Functions that don't return anything still needs the return type on their type. You can't ommit the type to get [the nothing type](#The-nothing-type), so use the [empty identifier](#Identifier-naming-rules) for that:
```py
foo :: () `` # valid
  = () =>;
bar :: () # error: '()' isn't a valid type
  = () =>;
```

[Argument names have semantic meaning](#Unordered-parameters). That's why they are included in function types.

Because of the 'functions are just values' mindset nested functions are simple:
```py
foo ::= () => {
  bar ::= (x, y : i32) i32 => x + y;
};
```

Function literals assigned to constants are considered 'named functions'. This is basicaly taking adventage of [constant scoping](#Constant-scoping) and not actually a seperate feature. I.e. recursion is possible:
```py
foo ::= (x : i32) => foo(x + 1); # completely valid
```

#### Explicitly typed functions
You can explicitly type a function without [type inference](#Type-inference):
```py
sum_any :: (T :: type, x, y : T) T = (T :: type is number, x, y : T) T => x+y;
```

If a constant already have the proper function type you can use the [guards](#Function-guards) directly, without the parameter list or return type:
```py
sum :: (x, y : i32) i32 = (x, y) => x+y;
```

Because functions are just constants you can split the definition and assignment:
```py
sum :: (x, y : i32) i32;
sum = (x, y) => x+y;
```

The patternless guard is still possible to use:
```py
sum :: (x, y : i32) i32;
sum ==> x+y;
```

`==>` isn't one operator is the assignment operator `=` followed by the guard body symbol `=>`:
```py
sum = => x+y; # same thing, but with spacing in between '=' and '=>'
```

### Function pointers
Variables can't have function types, only constants. For variables use function pointers instead.

To assign a function pointer to a variable you take its address:
```py
foo ::= () =>;
a := &foo;
```

This would be an error:
```py
foo ::= () =>;
a := foo; # error: trying to assign function type to variable
```

Function literals can be assigned directly to variables if you take its address:
```py
a := &(x : i32) i32 => x*x;
```

The code above is basically creating an anonymous function and assigning its address to the variable.

You can't use recursion on functions assigned directly to variables because they're anonymous:
```py
a := &(x : i32) i32 => a(x)*x; # error: a is undefined
```

Function pointers can't have constant arguments. To get a function pointer from a function with constant arguments you have to do the following:
```py
square ::= (T :: type is number, x : T) T => x*x;
ptr_to_square_i32 := &square(i32);
ptr_to_square_f32 := &square(f32); # different pointer from 'ptr_to_square_i32'
```

The type of a function pointer is `*<function-type>` as expected, they look like this:
```py
*(name0 : type0, name1 : type1, ...) return_type
```

Because the name of parameters is part of the type, you can't assign functions with same structure but different parameter names:
```py
sum : *(a, b : i32) i32 = &(x, y : i32) i32 => x+y; # error: types differ
```

But [casting](#Function-casting) is possible:
```py
sum : *(a, b : i32) i32 = &((x, y : i32) i32 => x+y) -> *(a, b : i32) i32; # valid
```

If the variable is explicitly typed you can take the address of just a [guard](#Function-guards) list:
```py
sum : *(a, b : i32) i32 = &(a, b) => a + b; # valid
```

Taking the address of a patternless guard body symbol `=>` is also valid:
```py
sum : *(a, b : i32) i32 = &=> a + b; # valid
```

Obviously is not possible to take the mutable address of a function, so mutable function pointers aren't a thing.

### Passing functions as arguments
There are basically three ways of passing functions as arguments:
1. Function pointers
2. Constant arguments with function type
3. Constant arguments with [overload](#Overloads) type
```py
some_overload ::= [a : number](a, a) a;
foo ::= (fn : *(x, y : i32) i32, x, y : i32) i32 => fn(x, y);
bar ::= (fn :: (x, y : i32) i32, x, y : i32) i32 => fn(x, y);
baz ::= (fn :: some_overload, x, y : i32) i32 => fn(x, y);
```

The function pointer one will have memory, so it can be assigned to things.

The overload one will accept all functions that are part of that overload_set.

And the function type one will accept any function with that type signature.

You can pass just the guards to functions or function pointers, so this is valid:
```py
foo ::= (fn : *(x, y : i32) i32, x, y : i32) i32 => fn(x, y);
bar ::= (fn :: (x, y : i32) i32, x, y : i32) i32 => fn(x, y);
apply ::= (fn :: (x : i32) i32, x : i32) i32 => fn(x);
a := foo(&=> x + y, 1, 2); # valid, 'a == 3'
b := bar(=> x + y, 1, 2); # valid, 'b == 3'
c := apply(x => x*x, 10); # valid, 'c == 100'
```

## Constants
Constants are immutable compile-time known values.

Keep in mind that everything that is named on the language is either a [variable](#Variables) or a constant.

So named functions are constants, named structs are constants, named modules are constants and so on.

### Constant definitions
Constants are defined by using an identifier, followed by the constant definition symbol `::`, followed by the type of the constant, then the assign operator `=` and finally it's constant value:
```py
thirty :: i32 = 30;
```

The assignment is optional, if no value is provided it'll use the default value specified by the type:
```py
zero :: i32; # 'zero' is going to be equal to 0
```

Differently from [variables](#Variables) forward assignment of constant is possible:
```py
const :: i32;
const = 10; # valid
```

### Type inference
If you omit the type in between the `::` and `=` the type will be inferred based on the value:
```py
FOO ::= 10;
```

To see more on how inference is determined check the [inference table](#Type-inference-by-value-table).

### Type aliases
If you set the value of a constant to be equal to a type, the constant will be a type alias:
```py
int ::= i32; # int is a type alias of i32
double ::= (x : int) int => x+x;
my_i32 : i32 = double 10; # valid
```

the type of a type alias is... `type`. So you can define it explicitly:
```py
int :: type = i32; # int is a type alias of i32
```

You can also create distinct type aliases using the `type()` constructor: 
```py
int ::= type(i32); # int is its own distinct type
double ::= (x : int) int => x+x;
my_i32 : i32 = double 10; # invalid, int != i32
```

### Multiple constants definitions at once
It's also possible to define multiple constants at once separating them by commas:
```py
FOO, BAR ::= 10; # both will have anyi type with 10 as the value
```

With this a function can have multiple names:
```py
foo, foo_alternative ::= (x = i32) i32 => x*x+x; # Both foo and foo_alternative will have the same function address
foo_alternative2 ::= foo; # This also generates a constant with the same function address and type
```

### Constant scoping
When a constant is defined it'll become available to all the current and nested scopes. So forward declaration is possible:
```py
BAR_SQUARED ::= BAR*BAR; # BAR_SQUARED == 100
BAR ::= 10;
```

This is particularly good for functions:
```py
square_and_sum ::= (x, y : i32) i32 => square x + square y;
square ::= (x : i32) i32 => x*x;
```

If you use a forward declared constant A to define a constant B, and at the same time use constant B to define constant A an error will occur:
```py
FOO ::= BAR;
BAR ::= FOO; # error: no value can be determined
```

This does not influence functions, though. The function body is completely unrelated to the value assigned to a constant, the constant internally is just an address to said function. So this is totally fine:
```py
foo ::= (x : i32) i32 => if x == 0 do bar(x) else      1;
bar ::= (x : i32) i32 => if x == 0 do      0 else foo(x);
```

Naturally, recursiveness on constant values is not possible:
```py
FOO ::= FOO + 1; #error: no value can be determined
```

But as [explained before](#Functions-are-just-values), recursion on functions is possible. Keeping the fact that the body of a function does not influence the constant at all:
```py
fact ::= (x : i32) i32 
  x < 1 => 1,
        => x * fact(x-1);
```

Because constants exists on the scope as a whole, even after its definition, forward assignment is just a split of the definition:
```py
ten :: i32;
twenty ::= ten * 2; # twenty == 20
ten = 10;
```

### Constant shadowing
Shadowing is often allowed in most languages. This is an absurd mistake, it brings a lot of errors and harder to maintain code. Constant shadowing can be traded by better naming.

Because of this shadowing of constants is disallowed:
```py
TEN ::= 10;
TEN ::= 20; # error: redefining 'TEN'
```

Even on different scopes:
```py
TEN ::= 10;
{
  TEN ::= 20; # error: redefining 'TEN'
};
```

### Unused constants
Similarly to [unused arguments](#Unused-arguments), unused constants also throw compile errors:
```py
FOO ::= 10; # error: 'FOO' is never used
```

You solve this in the same way unused arguments did: add an underscore at the start of the identifier:
```py
_FOO ::= 10; # valid
```

## Variables
Variables are very similar to [constants](#Constants). The main difference is that variables are runtime values.

### Variable definitions
Variables are defined by using an identifier, followed by the variable definition symbol `:`, followed by the type of the constant, then the assign operator `=` and finally it's value:
```py
x : i32 = 10;
```

In fact all the ways of defining a constant are aplicable to variables:
```py
x : i32 = 1; # explicit type with value
a, b : i32 = 2; # multiple variables defined with explicit type and value
y := 3; # inferred type by value
c, d := 4; # multiple variables defined with inferred type by value
z : i32; # defined with just a type with it's default value (0 in this case)
e, f : i32; # multiple variables defined with just a type with it's default value (0 in this case)
```

### Variable mutability
All variables are immutable by default and cannot be assigned to after its definition.
```py
x := 10;
x = 20; # error: 'x' is immutable
```

Forward assigning of immutable variables is possible. But only if the variable is not read before it:
```py
x : u32;
x = 20; # valid

y : u32;
z := y;
y = 10; # error: 'y' is immutable
```

If you're forward assigning with an `if` expression, the variable has to be assigned in a conditionless else:
```py
x : u32;
if something do x = 20
else            x = 0; # valid

y : u32;
if something do y = 20;
```

If mutability is a needed factor for a variable use the `mut` modifier on its definition:
```py
x : mut i32 = 10;
x = 20; # valid, x is mutable
```

Keep in mind that `mut T` is not a distinct type from `T`. `mut` is a variable modifier, not a type one.

`mut` can be used on values as well. This will mean "this value can only be assigned to mutable variables":
```py
x : mut i32 = mut 1; # valid
y :     i32 = mut 1; # error: trying to assign mutable value to immutable variable
```

This is not allowed:
```py
x : mut = 1; # error: 'mut' modifier without type
```

The correct way of inferring the type of a variable and make it mutable is using mutable values:
```py
x := mut 1;
x += 1; # valid, 'x' is mutable
```

`var := value` will always generate an immutable variable, even if `value` is mutable:
```py
x := mut 1; # 'x' is mutable
y := x; # 'y' is immutable
```

So if a function returns a mutable value you will need to use `mut` explicitly when inferring the type:
```py
counter ::= (x : i32) mut i32 => x+1;
x := mut counter(0); # valid
y := counter(x); # error: assigning mutable value to immutable variable
```

The decision of this design was made to avoid creating mutable variables without the user knowing.

### Default initialization
When you define a variable without giving it a default value it'll always default to 0 ([or whatever the default value is](#Default-values)).

If you want to garbage initialize a variable, for performance or whatever reasons, you can do so explicitly:
```py
x : mut i32 = ---; # defaults to stack garbage
```

Stack garbage initialization can only be done on mutable variables:
```py
x : i32 = ---; # error: '---' can only be used on mutable variables
```

And can only be used at the variable definition:
```py
x : i32 = 1;
x = ---; # error: '---' can't be assign to variable post-definition
```

Forward assignment with it is not possible:
```py
x : i32;
x = ---; # error: '---' can't be assign to variable post-definition
```

And obviously the type can't be inferred from `---`:
```py
x := ---; # error: can't infer type from '---'
```

### Variable assignment
A variable assignment is also an expression, it returns [nothing](#The-nothing-type). But if you wrap the assignment in pipes then it actually returns the value of the assigned variable after it's definition:
```py
x := mut 1;
y := |x = x + 1|; # y == 2
```

All of the assignment operators works in that way, including `++` and `--`:
```py
x := mut 1;
y = |x++|; # y == 2
```

The prefix `++` and `--` only differ with the parethesis assignment syntax:
```py
x := mut 1;
x++; # exatcly the same as ++x
++x; # exatcly the same as x++
y := |x++|; # assign x to y then increment x
z := |++x|; # increment x then assign x to z
```

This also works for variable definitions:
```py
y := |x := 10|; # equivalent to 'x, y := 10'
```

If you wrap a variable definition on parentheses you'll actually be creating a nameless struct:
```py
y := (x := 10); # error: assigning struct to variable
```

Keep in mind that when assigning a variable assignment to another variable you will create a variable of type [nothing](#The-nothing-type), as said before:
```py
x := mut 1;
y := x++; # y == ()
```

So you have to remember:
- `x = x + 1` = `()`
- `|x = x + 1|` = `x + 1`
- `|x := 10|` = `10`
- `(x := 10)` = new struct defined, even though it's an invalid struct (see the [structs](#Structs) section for more information)

### Variable aliasing
No copies will be made when creating an immutable variable if the following conditions are met:
1. The new defined variable value is just another variable. E.g. `y := x;`
2. The new defined variable is immutable
3. The source variable is immutable, or it's not modified during the lifetime of the new variable
4. The lifetime of the source variable is equal to or greater than the lifetime of the new variable

When these conditions are met the new variable is actually just an alias. So don't be afraid to assign by value:
```py
x := 0;
y := x; # no actual copy is made
```

This also aplies to field members of [structs](#Structs):
```py
foo : Some_Struct;
y := foo.x; # no actual copy is made
```

### Unused variables and the '_' identifier
It's exactly the same situation as [unused constants](#Unused-constants) or [unused arguments](#Unused-arguments):
```py
x := 10; # error: 'x' is never used
_x := 10; # valid
```

The `_` identifier is a special one. It can be defined numerous times inside the same scope and it's not possible to read its value:
```py
_ := 10;
_ := 20;
x := _; # error, '_' is not defined
```

The value inside `_` is immediataly thrown way and not stored in memory.

This is particularly useful for functions. Function return values _have_ to be used, so this is a way to signal that the value does not matter:
```py
five ::= () i32 => 5;
five(); # error: unhandled return value
_ := five(); # valid, result is thrown away
```

You can also name function arguments as '_', this is particularly great for values that you don't care about in callbacks:
```py
some_callback ::= (a, b, _ : u32, _ : i32, _ : f32) u32 => a + b;
```

It's also possible to define `_` as a constant as well. This is useful for getting rid of compile-time functions return values at [module-scope](#Scoping):
```py
foo ::= () i32 => return_some_i32();

@foo(); # invalid, unused return value
_ ::= @foo(); # valid

main ::= () =>;
```

Technically you can do the same thing using just the variable `_` instead of the constant one. But this is good if in the future you want this value to be a constant:
```py
_ ::= @foo(); # before
easy_change ::= @foo(); # after
```

### Variable scoping
Differently from [constants](#Constant-scopint), variables cannot be forward declared:
```py
x := y*y; # error: 'y' is not defined
y := 10;
```

### Variable shadowing
Shadowing of variables is disallowed:
```py
x := 1;
x := 2;
```

Use mutable variables instead:
```py
x := mut 1;
x = 2;
```

For a more in-depth reasoning of this decision see the [constant scoping section](#Constant-scoping);

### Global variables
variables can be defined on [module-scope](#Scoping). Their default values must be constant and they can't be assigned on module-scope, even forward assignment isn't allowed:
```py
x : i32;
x = 10; # error: assignment on module-scope
main ::= () =>;
```

You can reference variables on [module-scope](#Scoping) for assignment of other variables or constants:
```py
x := 10;
y := x; # valid
FOO ::= x; # valid
main ::= () =>;
```

But if the variable is mutable you can't:
```py
x := mut 10;
y := x; # error: 'x' isn't a constant or an immutable global variable
FOO ::= x; # error: 'x' isn't a constant or an immutable global variable
main ::= () =>;
```

## Primitive types
There are two types of primitive types, the ones that can be assigned to runtime variables and the ones that dont.

### Compile-time-only primitive types
None of the compile-time-only primitives have size or alignment
- __Any integer:__ `anyi`
- __Any floating point:__ `anyf`
- __Variable type:__ `type`
- __Ranges:__ `range`
- __Classes:__ `class`
- __Spaces__: `spc`
- __Foreign functions__: `unkfn`

This types are valid to use in constants and [ensured-compile-time functions](#Compile-time-functions).

### Integers
There are only three language-defined integer types:
1. `ibits(N)`
2. `ubits(N)`
3. `anyi`

Both `ibits(N)` and `ubits(N)` let you create integers of any size:
- The integer will have `N` bits of precision and storage
- `N` must be a power of 2 greater than or equal to 8 (e.g., 8, 16, 32, ..., 4096).
- If an invalid `N` is passed it'll cause an error to occur

The difference is that `ibits(N)` is for signed integers using [two's complement](https://en.wikipedia.org/wiki/Two%27s_complement) encoding, while `ubits(N)` is for unsigned integers.

Both the size and alignment of `ibits(N)` and `ubits(N)` are exactly `N / 8` bytes. `anyi` has no size nor alignment, it's a compile-time only type.

#### Any integer
`anyi` is a compile-time only type. The integer held in it has no bounds or memory. Because it's a compile-time only type, it can't be bounded to variables:
```py
X :: anyi = 123; # valid
x : anyi = 123; # error: variable can't have the type 'anyi'
```

`anyi` can be passed to any integer:
```py
X :: anyi = 123;
a : u8 = X;
b : i64 = X;
```

#### Integer aliases
There are some integer aliases of `ibits(N)` and `ubits(N)` for ease of use. All of them are distinct [type aliases](#Type-aliases):
```py
i8    ::= type(ibits(8));
i16   ::= type(ibits(16));
i32   ::= type(ibits(32));
i64   ::= type(ibits(64));
isize ::= type(ibits(@size_of *``));
u8    ::= type(ubits(8));
u16   ::= type(ubits(16));
u32   ::= type(ubits(32));
u64   ::= type(ubits(64));
usize ::= type(ubits(@size_of *``));
```

#### Integer literals
There are four different types of integer literals:
1. Decimal: `123`
2. Hexadecimal: `0x123abc`
3. Binary: `0b011110`
4. Octal: `0o644`

All of them can be prefixed with `-` for negative values.

These literals returns an `anyi`, thus you can use them independently of the variable type:
```py
a : u8 = -123;
b : i16 = -0xabc;
c : u32 = 0o567;
d : isize = -0b011110;
```

Integer literals can use `_` as a separator:
```py
a : u32 = 4_294_967_295; # valid
```

#### Integer inference
You can infer the type of a variable from `anyi` literals, but it will require context to infer it. If the variable is used like, for example, an `u8` the variable type will be `u8`:
```py
x := 123;
y : u8 = x; # because of this context, 'x' is 'u8'
```

If the type of a variable can't be determined from context the fallback follow in the following order:
1. Type alias `i32`, if it's in between the signed 32-bit range
2. Type alias `i64`, if it's in between the signed 64-bit range

If none of them apply the minimum `ibits(N)` that can hold the value is going to be used:
```py
foo := (T : imp type, _ : T) =>;
x := 10;
y := 10_000_000_000;
z := 10_000_000_000_000_000_000;
foo(x); # 'x' will be inferred as 'i32'
foo(y); # 'y' will be inferred as 'i64'
foo(z); # 'z' will be inferred as 'ibits(128)'
```

When integer literals are used on constants the type of the constant will be inferred as `anyi`:
```py
X ::= 123; # @type_of(X) == anyi
```

Because of this constants can be passed to any integer:
```py
X ::= 123;
_ : i8          = X; # valid
_ : i16         = X; # valid
_ : i32         = X; # valid
_ : i64         = X; # valid
_ : isize       = X; # valid
_ : u8          = X; # valid
_ : u16         = X; # valid
_ : u32         = X; # valid
_ : u64         = X; # valid
_ : usize       = X; # valid
_ : ibits(4096) = X; # valid
_ : ubits(4096) = X; # valid
```

You can't pass `anyi` integers if it's not within the bounds of the integer:
```py
_ : i8 = 128; # error: '128' don't fit in an 'i8'
```

Use explicit or auto casting for that (it'll wrap):
```py
_ : i8 = 128 -> i8; # valid
_ : i8 = 128!; # valid
```

#### Integer operations
The integer operations are:
- _Addition:_ `+`
- _Subtraction:_ `-`
- _Multiplication:_ `*`
- _Division:_ `/`
- _Modulo:_ `%`
- _Equals:_ `==`
- _Not Equals:_ `!=`
- _Less than:_ `<`
- _Greater than:_ `>`
- _Less than or equal to:_ `<=`
- _Greater than or equal to:_ `>=`
- _Bitwise not:_ `~`
- _Bitwise and:_ `&`
- _Bitwise or:_ `|`
- _Bitwise exclusive or:_ `^`
- _Bit shift left:_ `<<`
- _Bit shift right:_ `>>`

And the assignment operations on integer variables:
- _Assignment:_ `=`
- _Plus equals:_ `+=`
- _Minus equals:_ `-=`
- _Multiply equals:_ `*=`
- _Divide equals:_ `/=`
- _Modulo equals:_ `%=`
- _Bitwise exclusive or equals:_ `^=`
- _Bitwise or equals:_ `|=`
- _Bitwise and equals:_ `&=`
- _Bit shift left equals:_ `<<=`
- _Incremeant prefix/suffix:_ `++`
- _Decremeant prefix/suffix:_ `--`

All of this operations can only be performed on the same type. So this is invalid:
```py
a := 1 -> u8;
b := 2 -> i32;
c := a + b; # error: '+' operation on distinct types 'u8' and 'i32'
```

Because the aliases (`i8`, `u8`, `i64`, `u64`, etc) are [distinct aliases](#Type-aliases), you can't perform operations on their `ibits(N)`/`ubits(N)` counter parts:
```py
a := 1 -> u8;
b := 2 -> ubits(8);
c := a + b; # error: '+' operation on distinct types 'u8' and 'ubits(8)'
```

Obviously integer x floating point operations are also invalid:
```py
a := 1 + 2.0; # error: '+' operation on distinct types 'i32' and 'f32'
```

### Floating points
There are only three language-defined floating point types:
1. `f32`
2. `f64`
3. `anyf`

`f32` is a single precision floating point number and `f64` is double precision.

The size and alignment of `f32` is 4 bytes and of `f64` is 8 bytes. `anyf` has no size nor alignment, it's a compile-time only type.

`anyf` is basically the same as [anyi](#Any-integer), but for floats:
```py
X :: anyf = 1.0; # valid
x : anyf = 1.0; # error: variable can't have the type 'anyf'
```

Floating point literals are `anyf` and can be passed to both `f32` and `f64`:
```py
a : f32 = 1.0; # valid
b : f64 = 1.0; # valid
```

The inference of floating points works exactly like the [integers](#Integer-inference):
```py
x := 1.0;
y : f64 = x; # 'x' is inferred as 'f64' because of this context
```

And if a variable type can't be achieved from context. It falls back to `f32` by default, or `f64` if more precision is needed to handle the literal:
```py
foo := (T : imp type, _ : T) =>;
x := 3.1415927;
y := 3.141592653589793;
foo(x); # 'x' will be inferred as 'f32'
foo(y); # 'y' will be inferred as 'f64'
```

You can also have scientific notation floating point literals:
```py
x := 1.2e4;
y := 1e-24;
```

#### Floating point operations
The floating point operations are:
- _Addition:_ `+`
- _Subtraction:_ `-`
- _Multiplication:_ `*`
- _Division:_ `/`
- _Equals:_ `==`
- _Not Equals:_ `!=`
- _Less than:_ `<`
- _Greater than:_ `>`
- _Less than or equal to:_ `<=`
- _Greater than or equal to:_ `>=`

And the assignment operations on floating point variables:
- _Assignment:_ `=`
- _Plus equals:_ `+=`
- _Minus equals:_ `-=`
- _Multiply equals:_ `*=`
- _Divide equals:_ `/=`
- _Incremeant prefix/suffix:_ `++`
- _Decremeant prefix/suffix:_ `--`

You can't do `f32` x `f64` operations:
```py
a := 1.0 -> f32;
b := 2.0 -> f64;
c := a + b; # error: '+' operation on distinct types 'f32' and 'f64'
```

As stated before, integer x floating point operations are also invalid:
```py
a := 1 + 2.0; # error: '+' operation on distinct types 'i32' and 'f32'
```

### Booleans
Booleans are really simple, they just can have two states: `true` or `false`:
- `true`: Equivalent to 1 in integers
- `false`: Equivalent to 0 in integers

The operators `==`, `<`, `>`, `!=`, `>=`, `<=` and prefix `!` always returns booleans.

The type of a boolean is `bool`:
```py
b : bool;
```

And boolean inference is really simple:
```py
x, y := 10;
b0 := true; # @type_of(b0) == bool
b1 := x == y; # @type_of(b1) == bool
```

Both the size and alignment of a boolean is 1 byte.

### The nothing type
Differently from other languages `void` or `()` isn't a builtin type for representing nothing. Actually the type meaning "nothing" is literally the omission of a type, that is why in most places where a type must be put you can ommit it and get the nothing type.

In some places the ommission of a type can generate ambiguity (e.g. [function types](#Functions-are-just-values) and [structs](#Structs)), in those cases you can use the [empty identifier](#Identifier-naming-rules):
```py
foo :: (a, b: i32); # 'foo' is a struct
bar :: (a, b: i32) ``; # 'bar' is a function that returns nothing
```

It is possible to make variables with the nothing type:
```py
x : ``; # typed as nothing
x = (); # valid '()' is an expression that returns nothing
```

Nothing variables only have one value, and that is nothing. This is because nothing typed variables don't hold any value nor have size, but they do have an address. Because of this the next variable created will actually have the same address as the nothing typed variable:
```py
x := ();
y := 10;
z := &x == &y; # z = true
```

The main use for the nothing type is when a function needs to return nothing or [making a pointer that points to anything](#Nothing-pointer).

Both the size and alignment of the nothing type is 0 bytes.

### Characters
Characters are actually just numbers, there are two types:
- `char8`: 8-bit character
- `char32`: 32-bit character

There are two types of character literals: `'<char>'` for UTF-32 characters (`char32`) and `a'<char>'` for ASCII characters (`char8`)

A character literal can never be empty:
```py
c := ''; # error: empty character literal
```

The correct way to get the literal for a quote character is `'''`:
```py
c := '''; # c is a quote character
```

And for backslashes is `'\'`:
```py
c := '\'; # c is a backslash character
```

And escape sequences are handled normally with backslashes:
```py
c := '\n'; # c is a new line character
```

### Strings
There are four different types of strings:
- `str`: UTF-8 string view
- `astr`: ASCII string view
- `nstr`: NUL-terminated UTF-8 string
- `cstr`: NUL-terminated ASCII string

Both `str` and `astr` are [slices](#Slices) of `u8`. They're internally defined as simply distinct type aliases of `[..]u8`:
```py
str ::= type([..]u8);
astr ::= type([..]u8);
```

`str` and `astr` don't own any memory. They're always just views into buffers. `str` is an UTF-8 string and `astr` is an ASCII string.

And both `nstr` and `cstr` are [pointers](#Pointers) to `u8`s. They're internally defined as simply distrinct type aliases of `*u8`:
```py
nstr ::= type(*u8);
cstr ::= type(*u8);
```

`nstr` and `cstr` also don't own any memory. They're always just views into NUL-terminated buffers. `nstr` is an UTF-8 string and `cstr` is an ASCII string.

All of the string types are not defined at the [base module](#The-base-module), they're actually builtin.

You can use the `len` overload on `str` and `astr` normally as you would on any slice. You can also use it on `nstr` and `cstr`, but it's unsafe.

`str` and `astr` have the size of a pointer times 2 bytes and the alignment of a pointer.

`nstr` and `cstr` have both the size and alignment as a pointer.

#### String literals
Each string has it's own literal:
- `u"text"` will generate an `str`
- `a"text"` will generate an `astr`
- `n"text"` will generate an `nstr`
- `c"text"` will generate an `cstr`

You can also use the basic string literal `"text"` on all of the strings.

The inferred type of a basic string literal will always be `str`:
```py
text := "text"; # @type_of(text) == str
```

Unless it's used on constants, then the type will be `anys` and can be passed to any string:
```py
TEXT ::= "text";
t0 : str = TEXT; # valid
t1 : astr = TEXT; # valid
t2 : nstr = TEXT; # valid
t3 : cstr = TEXT; # valid
```

The inferred type from `anys` will always be `str` as well:
```py
TEXT ::= "text";
text := TEXT; # @type_of(text) == str
```

#### Raw string literals
Theres a way to transform a series of tokens into string literals directly: use `$` followed by an identifier, then you need to write the same identifier again. Everything in between the identifiers will become part of the string, the first and last white spaces do not count:
```py
_ := $end hello, world! end;
```

The above raw string literal is equivalent to:
```py
_ := "hello, world!";
```

The type of strings generated from raw string literals is always `str`:
```py
_ : cstr = $end hello, world! end; # error: expected value of type 'cstr' but received 'str'
```

Because the string only ends when the specified identifier is reached you don't need any escaping:
```py
_ := $end
My name is: "John Doe"
I'm '20' years old
end
```

The above raw string literal is equivalent to:
```py
_  := "My name is: \"John Doe\"\nI'm '20' years old"
```

You can add constant binds to the raw string. In between the `$` and the identifier add parenthesis, inside the parenthesis you can create constants that works on the string. To reference the constant binding put the specified identifier in between `$`:
```py
_ := $(NAME ::= "John Doe", AGE ::= 20) end
His name is $NAME$. $NAME$ is $AGE$ years old.
$AGE$ is a good age. "$NAME$" is a good name.
end
```

The above raw string literal is equivalent to:
```py
_ := "his name is John Doe. John Doe is 20 years old.\n20 is a good age. \"John Doe\" is a good name";
```

If there is no definition of the constant bind being referenced, it'll just format to `$<identifier>$`:
```py
_ := $end $TEN$ end # equivalent to "$TEN$"
```

You can bind any constant into the raw string constants:
```py
TEN ::= 10;
_ := $(X ::= TEN) end $X$ $X$ $X$ end; # equivalent to "10 10 10"
```

If you want the constant bind and the constant itself to share the same name you can do this as a shortcut:
```py
TEN ::= 10;
_ := $(TEN, ELEVEN ::= 11) end $TEN$ $TEN$ $TEN$ $ELEVEN$ end; # equivalent to "10 10 10 11"
```

You can also bind every constant defined on _the current scope_ using `(*)`:
```py
TEN ::= 11;
FOO ::= "foo";
_ := $(*) end $TEN$_$FOO$ end; # equivalent to "10_foo"
```

Remember `(*)` only works for definitions on the current scope:
```py
TEN ::= 10;
{
  FOO ::= "foo"; 
  _ := $(*) end $TEN$_$FOO$ end; # equivalent to "$TEN$_foo"
};
```

Use `(*)` only in tight packed scopes because the risk of an override of an actual word and a predefined constant is high:
```py
hello ::= "world";
_ := $(*) end $hello$, $world$! end; # equivalent to "world, $world$!"
```

This can be a little dumb with this example, but it can be dangerous if you actually want a thing warpped around `$`s to be part of the string.

### Ranges
Ranges are a range between two integers: the starting index and the ending index. The starting index always have to be less then or equal to the ending index:
```py
0..4;
9..9;
30..20; # error: starting index has to be smaller then or equal to the ending index
```

Ranges with the `..` syntax are inclusive. This means that the ending index is included if the range is used on a [for loop](#For-loop) or [slice](#Slices).

For an exclusive range use `..<`. Exclusive ranges can't haver the starting index and ending index being the same value:
```py
0..<4;
9..<9; # error: starting index has to be smaller then the ending index
30..<20; # error: starting index has to be smaller then the ending index
```

Ranges can be assigned to constants:
```py
RANGE ::= 2..5;
```

You can access the starting index and the ending index of range with the `.start` and `.end` members:
```py
RANGE ::= 2..5;
starting_index := RANGE.start;
ending_index   := RANGE.end;
```

### Type inference by value table
| Value            | Decription       | Constant inferred type              | Variable inferred type                                            |
|------------------|------------------|-------------------------------------|-------------------------------------------------------------------|
| 1234             | Integer literal  | anyi (can be passed to any integer) | Inferred by first usage (u8, i16, etc). If needed defaults to i32 |
| 0x123abc         | Hex literal      | anyi (can be passed to any integer) | Inferred by first usage (u8, i16, etc). If needed defaults to i32 |
| 0b010101         | Bin literal      | anyi (can be passed to any integer) | Inferred by first usage (u8, i16, etc). If needed defaults to i32 |
| 0o567            | Oct literal      | anyi (can be passed to any integer) | Inferred by first usage (u8, i16, etc). If needed defaults to i32 |
| 12.34            | Float literal    | anyf (can be passed to any float)   | Inferred by first usage (f32 or f64). If needed defaults to f32   |
| true/false       | Boolean literal  | bool                                | bool                                                              |
| null             | Null constant    | option                              | option                                                            |
| 'a'              | uchar literal    | uchar                               | uchar                                                             |
| c'a'             | achar literal    | achar                               | achar                                                             |
| "text"           | Any string       | anys                                | str                                                               |
| u"text"          | UTF-8 string     | str                                 | str                                                               |
| a"text"          | ASCII string     | astr                                | astr                                                              |
| n"text"          | NUL-UTF-8 string | nstr                                | nstr                                                              |
| c"text"          | NUL-ASCII string | cstr                                | cstr                                                              |
| i32, f32, Struct | Types directly   | type                                | type (Compile-time only)                                          |
| \[1, 2, 3\]      | Array literal    | \[amount-of-elements\]inferred      | \[amount-of-elements\]inferred                                    |
| \[3\]21          | Array literal    | \[amount-of-elements\]inferred      | \[amount-of-elements\]inferred                                    |
| vec\[1, 2, 3\]   | Vector literal   | vec\[amount-of-elements\]inferred   | vec\[amount-of-elements\]inferred                                 |
| vec\[3\]21       | Vector literal   | vec\[amount-of-elements\]inferred   | vec\[amount-of-elements\]inferred                                 |
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
- _Alts:_ Defaults to the specified tag default value
- _Unions:_ Defaults all data to 0
- _Arrays:_ Defaults to the default of their type

## Pointers
You can grab a pointer to a variable using the `&` operator:
```py
a   := 0;
ptr := &a;
```

The type of a pointer is represented by a `*` followed by the type that the pointer points to:
```py
*i32 # pointer to an i32
*str # pointer to a str
**u8 # pointer to a pointer to an u8
```

The size and alignment of a pointer depends on the target platform. On 64-bit it's 8 bytes, on 32-bit it's 4 bytes and so on.

### Dereferencing
To dereference a pointer use the `*` operator, similarly to C:
```py
a   := 12;
ptr := &a;
b   := *ptr; # b = 12
```

Accessing a member from a pointer to a [struct](#Structs) causes an implicit dereference:
```py
a   := Some_Struct(.some_i32 = 12);
ptr := &a;
b   := ptr.some_i32; # equivalent to 'b := (*ptr).some_i32;'
```

Accessing the member from a pointer just to get it's address do not cause a dereference:
```py
a   := Some_Struct(.some_i32 = 12);
ptr := &a;
ptr_to_some_i32 := &ptr.some_i32; # no dereference
```

### Pointer mutability
Similarly to [variables](#Variable-mutability), pointers are immutable by default.

If you want to assign to the variable the pointers points to a mutable pointer is needed.

Use the `&mut` operator on a variable to take the mutable address from it. You can only make mutable pointers from mutable variables:
```py
a       := 0;
ptr     := &mut a; # invalid, 'a' isn't mutable
b       := mut 0;
ptr_mut := &mut a; # valid, 'b' is mutable
```

Now assigning to dereferenced values is possible:
```py
a   := mut 0;
ptr := &a;
*ptr = 10; # error: 'ptr' is not a mutable pointer
mut_ptr := &mut a;
*mut_ptr = 10; # valid, now 'a' = 10
```

Do not confuse `*mut` with `mut *`. `*mut` is a pointer that can change the value inside the address it's pointing to. `mut *` is a pointer that can change the actual address it points to:
```py
a,b := mut 0;

p0 : *mut i32 = &a;
*p0 = 10; # valid, it's a mutable pointer
p0  = &b; # invalid, 'p0' is a immutable variable

p1 : mut *i32 = &a;
*p1 = 10; # invalid, it's an immutable pointer
p1  = &b; # valid, 'p1' is a mutable variable

p2 : mut *mut i32 = &a;
*p2 = 10; # valid, 'p2' is a mutable pointer
p2  = &b; # valid, 'p2' is a mutable variable
```

Same thing for `&mut` vs `mut &`:
```py
a,b := mut 0;

p0  := &mut a; # same as 
*p0  = 10; # valid, it's a mutable pointer
p0   = &b; # invalid, 'p0' is a immutable variable

p1  := mut &a;
*p1  = 10; # invalid, it's an immutable pointer
p1   = &b; # valid, 'p1' is a mutable variable

p2  := mut &mut a;
*p2  = 10; # valid, 'p2' is a mutable pointer
p2   = &b; # valid, 'p2' is a mutable variable
```

### Pointer arithimatic
It's not possible to use the `[]` operator to index through a pointer.

But you can achieve something similar with pointer arithimatic:
```py
nums := [1, 2, 3];
ptr  := &mut something;
x    := *(ptr + 1); # equivalent to nums[1]
```

The only operations possible with pointers are: `+`, `-`, `+=`, `-=`, `++` prefix and suffix, `--` prefix and suffix.

Pointer's do not have bounds checking. It's not even possible because a pointer is just a pointer, no extra metadata.

### Pointer definition
Differently from other variable types, pointers cannot be initialized to nothing. This is because pointers doesn't have a default value and must always point to memory:
```py
ptr : *i32; # error: uninitialized pointer
```

It's fine to [forward assign](#Variable-mutability), though:
```py
a := 0;
ptr : *i32;
ptr = &a; # valid
```

This actually enables several advantages compared to pointers that permit something like `NULL` or `nullptr`:
- No need to check for invalid pointers
- Less branching
- Safer, every pointer is garanteed to be valid

Because of that reason it's not possible to initialize pointers to garbage:
```py
ptr : *i32 = ---; # error: uninitialized pointer
```

Often you actually don't need a pointer that points to nothing. Though, sometimes when interacting with the OS, C APIs or something of that sort a nullable pointer is needed. Use an [optinal pointer](#Optional-pointer) on that occasions:
```py
ptr : ?*i32; # valid, initialized to 'null'
```

Optional pointers can be initialized to garbage:
```py
ptr : ?*i32 = ---; # valid: initialized to garbage
```

Do not abuse of optional pointers. Only use when strictly necessary.

### Nothing pointer
[The nothing type](#The-nothing-type) is a type that holds no value. You can indeed have a pointer to _nothing_, but because pointers must always point to something, you can treat it like a pointer to _anything_ ([auto casting](#Auto-casting) on nothing pointers is permitted):
```py
a := i32;
b := f64;
c := Some_Struct;
ptr : mut *``; # pointer to the nothing type
ptr = &a!;
ptr = &b!;
ptr = &c!;
```

But when you want to convert from a nothing pointer to a `*T` [casting](#Pointer-casting) is needed:
```py
a := i32;
b := f64;
ptr : mut *``;
ptr = &a;
ptr_to_i32 : *i32 = ptr -> *i32; # valid
ptr = &b;
ptr_to_f32 : *f32 = ptr; # error: casting needed
```

You can dereference a nothing pointers, but this will result in a no-op in most cases. That is because nothing pointers have no size, you can't really write to something of size 0. Reading from it will just result in a nothing value.

### Pointer comparison
Pointer comparison is possible between pointers of same type or nothing:
```py
a  := i8;
b  := i8;
c  := u8;
p0 := *i8 &a;
p1 := *u8 &c;
p2 := *i8 &b;
p3 := *`` &a!;
_  := p0 == p1; # error
_  := p0 == p2; # valid, _ == false
_  := p0 == p3; # valid, _ == true
```

### Taking the address of literals
It's possible to take the address of literals:
```py
ptr := &10;
```

This is sugar for:
```py
tmp := 10;
ptr := &tmp;
```

So the lifetime of the invisible variable created is the same as the variable it's assigned to.

If you take the address of a literal at [module-scope](#Scoping) rather than block-scope, the memory for the variable will be allocated on static memory.

Mutable addresses are also possible:
```py
ptr := &mut 10;
```

Sugar for:
```py
tmp := mut 10;
ptr := &tmp;
```

It works for any literal.

## Arrays
Arrays are a buffer of objects of the same type with a compile-time known length.

To define an array open and close square brackets `[]`, in between the square brackets put a positive integer literal or positive integer constant. Follow the brackets by the desired type of the array elements:
```py
arr : [10]i32;
```

All values inside of the array are defaulted if defined that way.

It's possible to use array literals for non-default values:
```py
arr := [1, 2, 3]; # arr[0] == 1, arr[1] == 2, arr[2] == 3. Length of the array is 3
arr_explicit : [3]i32 = [4, 5, 6]; # same thing, but the array type is explicit
```

This type of array literal can have a type before it to ensure what the array type will be:
```py
arr := i32[1, 2, 3];
arr_explicit : [3]i32 = i32[4, 5, 6]; # same thing, but the array type is explicit
```

Another way of writing an array literal is `[amount]default-value`:
```py
arr0 := [3]1.5; # arr0[0] == 1.5, arr0[1] == 1.5, arr0[2] == 1.5. Length of the array is 3
arr0_explicit : [3]f32 = [3]1.5; # same thing, but the array type is explicit
arr1 := f64[3]1.5; # arr1[0] == 1.5f64, arr1[1] == 1.5f64, arr1[2] == 1.5f64. Length of the array is 3
arr1_explicit : [3]f64 = f64[3]1.5; # same thing, but the array type is explicit
```

This is good for explicit typing the elements.

You can also initialize arrays to garbage:
```py
arr : [3]u64 = ---;
```

The size of an array is the size of its elements type times the length of the array. And the alignment is the alignment of its elements type.

### Copying arrays
You can use a previously defined array to create a new one:
```py
a := [1, 2, 3];
b := a; # 'b' is a new array
```

### Array mutability
Because arrays actually store the data for modifying the values you just need to set the array as `mut`:
```py
arr0   := mut [1, 2, 3];
arr0[0] = 4; # valid
arr1   := [1, 2, 3];
arr1[0] = 4; # invalid, array is mutable
```

### builtin 'len'
If an array length is needed you can use the builtin `len()`:
```py
arr     := [1, 2, 3, 4, 5];
arr_len := @len(arr);
```

`len()` is actually an [overload](#Overloads), not a function. But specifically the array function overload is an ensured-compile-time one, so the `@` operator is necessary.

### Passing arrays as parameters
Because arrays must have known compile-time length they can't be passed directly to function variable parameters.

There are four ways of passing an array as parameters:
1. Same length arrays
2. Pointers
3. Constant arguments
4. [Slices](#Slices)

#### Passing same length arrays
You technically can pass arrays as parameters if you specify the length:
```py
foo ::= (_arr : [3]u32) =>;
arr0 := [1, 2, 3];
arr1 := [1, 2, 3, 4];
foo(arr0); # valid
foo(arr1); # error: expected [3]u32 found [4]u32
```

Following the [passing as reference rule](#Immutable-arguments-are-references) the array will not be copied.

To avoid several performance and semantic problems, arrays passed directly can't be mutable:
```py
foo ::= (_arr : mut [3]u32) =>; # error: array argument can't be mutable
```

Insuring that arrays of the same length are passed to arguments can be useful sometimes. But most of the time a function wants to accept any array length, so this method isn't good for that.

#### Passing arrays as pointers
Arrays do not decay into pointers like in C, so this is invalid:
```py
foo ::= (_arr : *u32) =>;
nums := [1, 2, 3, 4];
foo(nums); # error: expected *u32 found [3]u32
```

Taking the address of just the array will give you a pointer to the array type:
```py
foo ::= (_arr : *u32) =>;
nums := [1, 2, 3, 4];
foo(&nums); # error: expected *u32 found *[3]u32
```

You could use [casting](#Pointer-casting) to resolve this problem:
```py
foo ::= (_arr : *u32) =>;
nums := [1, 2, 3, 4];
foo(&nums -> *u32); # valid
```

Or preferably take the address of the first element:
```py
foo ::= (_arr : *u32) =>;
nums := [1, 2, 3, 4];
foo(&nums[0]); # valid
```

The problem with passing arrays as pointers is the lost of the array length.

To overcome this you could pass an extra argument to the function:
```py
foo ::= (_arr : *u32, _arr_len : usize) =>;
nums := [1, 2, 3, 4];
foo(&nums[0], @len(nums)); # valid
```

It'll work, but this is extremely unsafe and cumbersome. Therefore, not recommended

#### Passing array to constant arguments
You can pass arrays of any length using a constant parameter for the length:
```py
foo ::= (N :: imp usize, _arr = [N]u32) =>;
nums := [1, 2, 3, 4];
foo(nums); # valid
```

This is better than passing by pointer. The problem is that for every new array length that is passed a new version of the function is created. It's a viable option though, the performance will actually be great with compile-time catches on bound checks and compile-time known length.

## Slices
Slices are views into arrays. They consist of a length and a pointer.

To slice an an array use a [range](#Ranges) inside the index `[]`:
```py
arr   := [6, 5, 4, 3, 2, 1];
slice := arr[2..4]; # slice[0] == 4, slice[1] == 3, slice[2] == 2, len(slice) == 3
```

If you want to make a slice from the beginning to the end of the array use an empty range:
```py
arr   := [1, 2, 3];
slice := arr[..];
```

Or just set only the starting index or ending index:
```py
arr    := [1, 2, 3, 4, 5, 6];
slice0 := arr[..3]; # 0 starting index is implied
slice1 := arr[2..]; # @len(arr)-1 ending index is implied
```

You can use variables when slicing, but keep in mind that does have some runtime overhead for bounds checking:
```py
arr   := [1, 2, 3, 4, 5, 6];
n     := get_some_i32(); # supposed this returns 3 for some reason
slice := arr[..n]; # slice[0] == 1, slice[1] == 2, slice[2] == 3, len(slice) == 3
```

The type of a slice is `[..]<type>`, so use it to create a slice with an explicit type:
```py
arr    := [1, 2, 3];
slice0 : [..]u32 = arr[..];
slice1 : [..]i32 = arr[..]; # error: can't make slice u32 array into i32 
```

With explicit typed slices you can pass the array directly:
```py
arr   := [1, 2, 3];
slice : [..]u32 = arr; # valid
```

Similarly to pointers (because slices are basically just pointers), they can't be null and always have to point to some array:
```py
slice : [..]i32; # invalid
```

The size of a slice is the size of a pointer times 2. And the alignment is the alignment of a pointer.

### Getting the length of a slice 
Slices have a function in the builtin `len()` overload:
```py
arr    := [1, 2, 3];
slice  := arr[..];
amount := @len(slice);
```

### Passing array as slice
This concludes the fourth method of [passing an array as a paremeter](#Passing-arrays-as-parameters). You simply just need to pass the array as a slice. This is the most preferred method for general use cases:
```py
foo ::= (_arr : [..]u32) =>;
nums := [1, 2, 3, 4];
foo(nums); # valid
```

### Array literals on slices
Array literals can be sliced directly:
```py
slice : [..]u32 = [1, 2, 3]; # makes slice from array literal
```

The slice still doesn't hold the actual data of the array. What this actually does is create an implicit array that will live through the current stack frame.

This can be useful for some functions:
```py
sum ::= (xs : [..]i32) i32 => {
  res := 0;
  for x in xs => res += x;
  ret res;
};
x := sum([1, 2, 3, 4]); # x = 10
y := sum[1, 2, 3, 4]; # using the one paremeter call convention
```

### Slice mutability
The slices mutability is more akin to pointers than arrays. If you wan't to modify the values of an array through a slice you need a mutable slice using `[..]mut`:
```py
slice0 : [..]mut u32 = [1, 2, 3];
slice0[1] = 4; # valid
slice1 : [..]u32 = [1, 2, 3];
slice1[1] = 4; # invalid
```

If the slice variable itself is marked as mutable, it means you can change the slice not the values:
```py
slice : mut [..]u32 = [1, 2, 3];
slice = [4, 5]; # valid, new array assigned to slice
slice[1] = 4; # invalid
```

You can only pass mutable arrays to mutable slices:
```py
arr0 := mut [1, 2, 3];
arr1 := [1, 2, 3];
slice0 : [..]mut u32 = arr0; # valid
slice1 : [..]mut u32 = arr1; # invalid
```

### Manual slicing
Slices have two members `.buf` and `.len`. `.buf` is a pointer to the slice type and `.len` is the length of the memory buffer. Tou can always access these members:
```py
arr := [1, 2, 3];
slice := arr[..];
slice_buf := slice.buf; # @type_of(slice_buf) == *i32
slice_len := slice.len; # slice_len == 3
```

You can use a [constructor call](#Struct-constructors) on a slice type directly:
```py
arr := [1, 2, 3];
slice := [..]i32(&arr[0], @len(arr));
```

Normal slices have this structure:
```py
[..]T ::= (buf : *T, len : usize);
```

And mutable slices have this:
```py
[..]mut T ::= (buf : *mut T, len : usize);
```

And yes, if the variable slice is mutable you can modify these values directly:
```py
arr0 := [1, 2, 3];
arr1 := [4, 5, 6, 7];
slice := mut arr0[..];
slice.buf = &arr1[0];
slice.len = @len(arr1);
```

## Vectors
Vectors are similar to [arrays](#Arrays), but you can do operations on them, and they have a high chance of being optimized with SIMD.

To define a vector use an [array type](#Arrays) prefixed with the keyword `vec`, this is the type of a vector:
```py
vector : vec[2]f32;
```

Keep in mind that it looks like an array, but it's not.

The length of a vector needs to be at least 2:
```py
vector : vec[1]f32; # error: vector of lenth 1
```

To construct a vector use an [array literal](#Arrays) prefixed with the keyword `vec`, this is a vector literal:
```py
a := vec[1.0, 2.0, 3.0];
b := vec[3]0.5;
c := vec f32[1.0, 2.0, 3.0];
d := vec f32[3]0.5;
```

You can get the length of a vector using the builtin `len` overload:
```py
v := vec[1, 2, 3, 4];
v_len := @len(v);
```

### Valid vector element types
The element type of a vector can only be one of the following:
- `bool`
- `f32`
- `f64`
- `i8` or `ibits(8)`
- `u8` or `ubits(8)`
- `i16` or `ibits(16)`
- `u16` or `ubits(16)`
- `i32` or `ibits(32)`
- `u32` or `ubits(32)`
- `i64` or `ibits(64)`
- `u64` or `ubits(64)`
- `isize` or `ibits(@size_of(*``))`
- `usize` or `ubits(@size_of(*``))`

If an invalid type is used an error will occur:
```py
v : vec[3]usize; # error: 'usize' isn't a valid vector element type
```

### Operation on vectors
All operations available for scalars (the four basic operations `+`, `-`, `*`, `/`, bitwise operations `>>`, `<<`, `|`, `&`, `~`, and boolean operations `!=`, `==`, `>`, `<`, `>=`, `<=`) are available for vectors as well:
```py
v0 := vec[1.0, 2.0];
v1 := vec[3.0, 4.0];
a := mut v0 + v1;
b := mut v0 - v1;
c := mut v0 * v1;
d := mut v0 / v1;
a += v0;
b -= v0;
c *= v0;
d /= v0;
# and so on...
```

The operations are all per-component operations, including the boolean ones. Boolean operations will return a new vector of type `vec[N]bool`:
```py
res := vec[1.0, 2.0, 3.0] == vec[3.0, 2.0, 1.0]; # res == vec[false, true, false]
```

There are two builtin methods defined at the [base module](#The-base-module) for dealing with `vec[N]bool` in specific: `.all()` and `.some()`:
```py
res := (vec[1.0, 2.0, 3.0] == vec[3.0, 2.0, 1.0]).all(); # res == false, not all of the components are 'true'
res := (vec[1.0, 2.0, 3.0] == vec[3.0, 2.0, 1.0]).some(); # res == true, some of the components are 'true'
```

As the rest of Stark, all operations are strongly typed:
```py
c := vec[1.0, 2.0, 3.0] + vec[4, 5, 6]; # error: type mismatched operation
```

Operations between vectors and scalars are also possible:
```py
v := vec[1.0, 2.0];
a := mut v + 2.0;
b := mut v - 2.0;
c := mut v * 2.0;
d := mut v / 2.0;
a += 2.0;
b -= 2.0;
c *= 2.0;
d /= 2.0;
# and so on...
```

### Accessing vector components and swizzling
You can access the member of a vector with the `[]` operator:
```py
v : mut vec[5]f32;
v[0] = 1;
v[1] = 2;
v[2] = 3;
v[3] = 4;
v[4] = 5;
```

And you can swizzle the vector components in any order using the `[idx0 idx1 idx2 ...]` syntax:
```py
v := vec[1.0, 2.0, 3.0, 4.0, 5.0];
_ := v[3 2 0 4]; # _ == vec[4.0, 3.0, 1.0, 5.0]
_ := v[0 0 0 0 0 0 0]; # _ == vec[1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]
```

Both the simple `[]` accessor and the swizzling will be bound checked.

You can also access the first 4 elements with the `.xyzw` and `.rgba` accessors:
```py
v : mut vec[4]f32;
v.x = 1; # same as 'v[0] = 1'
v.y = 2; # same as 'v[1] = 2'
v.z = 3; # same as 'v[2] = 3'
v.w = 4; # same as 'v[3] = 4'
v.r += 1; # same as 'v[0] += 1'
v.g += 2; # same as 'v[1] += 2'
v.b += 3; # same as 'v[2] += 3'
v.a += 4; # same as 'v[3] += 4'
```

Swizzling is also possible using these types of accessors:
```py
v0 : mut vec[4]f32;
v0.xy = vec[1.0, 2.0];
v0.zw = vec[3.0, 4.0];
v1 := mut v0.xxyyzzww; # v1 is of type 'vec[8]f32' with the value 'vec[1.0, 1.0, 2.0, 2.0, 3.0, 3.0, 4.0, 4.0]' 
v0.rgb = [5.0, 6.0, 7.0];
v0.ar *= 2.0;
```

Mixing `.xyzw` with `.rgba` is invalid:
```py
v : mut vec(f32, 4);
v.xyba = [1.0, 2.0, 3.0, 4.0]; # error: invalid vector accessor '.xyba'
```

You can even use ranges (this will not create a [slice](#Slices), but a new vector):
```py
v : vec[64]i8;
_ := v[10..30];
```

Normal vectors will have the size of the element type times the vector length, and the aligment of the element type.

### SIMD vectors
It's possible to add a modifier called `simd` to a vector. This will make so that the vector will have proper alignment and be safe to use for SIMD instructions.

A `simd` vector can only have a specific length depending on the type and the register size:
| Type  | Valid sizes (128, 256, 512) |
|-------|-----------------------------|
| bool  | 16, 32, 64                  |
| i8    | 16, 32, 64                  |
| u8    | 16, 32, 64                  |
| i16   | 8, 16, 32                   |
| u16   | 8, 16, 32                   |
| i32   | 4, 8, 16                    |
| u32   | 4, 8, 16                    |
| f32   | 4, 8, 16                    |
| i64   | 2, 4, 8                     |
| u64   | 2, 4, 8                     |
| f64   | 2, 4, 8                     |
| isize | 2 or 4, 4 or 8, 8 or 16     |
| usize | 2 or 4, 4 or 8, 8 or 16     |

If you use an invalid length type match an error will occur:
```py
v : simd vec[2]f32; # error: invalid SIMD vector length
```

Not all architecture supports SIMD registers of 256 or 512 bit, so they will not always be available. If you're in an architecture that only support 128-bit registers if you try to create a vector that would only fit inside a 256 or 512 bit register an error will occur:
```py
# architecture only supports 128-bit registers
v : simd vec[8]f32; # error: SIMD vector length of '8' is not supported by the current target architecture
```

If the architecture you're targetting does not support SIMD instructions then using `simd` vectors will generate an error:
```py
# SIMD isn't supported on target architecture
pos : simd vec[4]f32; # error: SIMD vectors aren't supported by current target architecture
```

`simd` vectors are aligned to their byte width:
- If it's a 128-bit vector the alignment is 16
- If it's a 256-bit vector the alignment is 32
- If it's a 512-bit vector the alignment is 64

You can also do operations between SIMD vectors, and as expected they're SIMD operations:
```py
v := simd vec[1, 2, 3, 4] + simd vec[5, 6, 7, 8]; # valid
```

## Structs
The definition of a struct is very similar to the [definition of a function](#Function-definition-and-calling-conventions). The difference is that you do not use the guard body symbol `=>`:
```py
Some_Struct ::= (member0 : type0, member1 : type1, member2 : type2, ...);
```

It can facilitate to think of structs as "bodyless functions" because a lot of the arguments rules apply for them. But there are some differences.

Members of a struct are, mostly, variables. So all of this declarations are valid:
```py
Some_Struct ::= (
  a       : u32,       # explicit type with default initialization (0 in this case)
  b, c    : u32,       # multi-member definition
  d       : u32 = 1,   # explicit type with value
  e, f, g : u32 = 2,   # multi-member definition with explicit type and value
  j       : u32 = ---, # garbage initialization
);
```

Similarly to function arguments, the only exception is inferred types by value:
```py
Some_Struct ::= (
  a := 3, # error: member with no explicit type
);
```

You also can't have recursive members:
```py
Point ::= (point : Point, x, y : i32); # Error: recursive member
```

Structs with no members are valid:
```py
Empty ::= (,);
```

`Empty` will have size and aligment 0.

The comma is necessary for an empty struct, without it would just be an expression that returns [the nothing type](#The-nothing-type).

### Anonymous structs
All structs are anonymous, the ones with names are just bounded to constants.

You can create a variable with a struct literal directly:
```py
a : (x : i32, y : f32);
```

Keep in mind that all structs types are distinct, even if the memory and fields are the same:
```py
a : (x : i32, y : f32);
b : (x : i32, y : f32) = a; # error: 'a' is not of the same type as 'b'
```

See the [casting section](#Struct-casting) for workarounds.

### Accessing field members
There are two ways of accessing struct fields members: by name or by position:
```py
Vec2 : (x, y : f32);
pos := Vec2(1.0, 2.0);
a := pos.x; # a == 1
b := pos.1; # b == 2
```

Accessing by field name or by field position is just a way to access the same data:
```py
_ := &pos.x == pos.0; # true, they are the same thing
_ := &pos.y == pos.1; # true, they are the same thing
```

### Struct constructors
Regular struct instantiation is also similar to [regular function calls](#Function-definition-and-calling-conventions):
```py
Point ::= (x, y : i32);
p := Point(10, 20);
```

This type of instantiation is a "contructor".

One parameter like calls aren't valid when calling a constructor:
```py
Foo ::= (x : u32);
a := Foo 10; # invalid
```

And of course, you can explicitly type an instance:
```py
Foo ::= (x : u32);
a : Foo = Foo(10);
```

An instance with no value will be constructed with all the default values specified on the struct:
```py
Foo ::= (x : u32, y : u32 = 6);
a : Foo; # 'a' is created with default values: 'a.x = 0' and 'a.y = 6'
```

Unordered fields on constructor calls is also possible and follow the same rules as [functions unordered parameters](#Unordered-parameters):
```py
Foo ::= (a : i32, b : i32 = 10, c, d : i32);
_ := Foo(.c = 10, .a = 4, .b = -15, .d = 4);
_ := Foo(9, .c = 11, .d = 12);
_ := Foo(9, .c = 11, 12); # a == 9, b == 10, c == 11, d == 12
_ := Foo(9, .c = 11, .b = 12); # error: invalid order
```

You can also use the field position instead of the field name:
```py
Foo ::= (a : i32, b : i32 = 10, c, d : i32);
_ := Foo(.2 = 10, .0 = 4, .1 = -15, .3 = 4);
_ := Foo(9, .2 = 11, .3 = 12);
_ := Foo(9, .2 = 11, 12); # a == 9, b == 10, c == 11, d == 12
_ := Foo(9, .2 = 11, .1 = 12); # error: invalid order
```

Mixing field position with field names is possible:
```py
Foo ::= (a : i32, b : i32 = 10, c, d : i32);
_ := Foo(.0 = 1, .b = 2); # valid
```

But trying to set a positional field that was already set by a named field is obviously invalid:
```py
_ := Foo(.a = 1, .0 = 2); # error: constructor setting field 'a' twice
```

You can leave out any fields you want when calling a constructor. The field will use its default value:
```py
Foo ::= (a : i32, b : i32 = 10, c, d : i32);
_ := Foo(9); # a == 9, b == 10, c == 0, d == 0
_ := Foo(.d = 12); # a == 0, b == 10, c == 0, d == 12
_ := Foo(9, .c = 11); # a == 9, b == 10, c == 11, d == 0
```

Passing no arguments to the constructor will initialize all of the members to their proper default values:
```py
Foo ::= (a : i32, b : i32 = 10, c, d : i32);
_ := Foo(); # a == 0, b == 10, c == 0, d == 0
```

It's similar to defining a variable without a constructor:
```py
Foo ::= (a : i32, b : i32 = 10, c, d : i32);
_ := Foo(); # a == 0, b == 10, c == 0, d == 0
_ :  Foo;   # a == 0, b == 10, c == 0, d == 0
```

The difference is that you're explicitly assigning, so forward assignment is not an option:
```py
Foo ::= (a : i32, b : i32 = 10, c, d : i32);
a := Foo(); # a == 0, b == 10, c == 0, d == 0
a  = Foo(1, 2, 3, 4); # error: 'a' is immutable
```

### Unnamed members
Struct members can actually not have names:
```py
Some_Struct ::= (u32, named_member : f32);
```

If that's the case you can only access the field member via position:
```py
Some_Struct ::= (u32, i32);
foo := Some_Struct(1, -2);
a   := foo.0;
b   := foo.1;
```

Unamed members can have default values:
```py
Some_Struct ::= (u32 = 1, i32 = -2);
foo := Some_Struct();
a   := foo.0; # a == 1u32
b   := foo.1; # b == -2i32
```

To define a struct with only one unnamed field in it a trailing comma is necessary:
```py
Foo ::= (u32,); # Foo is a struct with one field
Bar ::= (u32); # Bar is actually a type alias for 'u32'
```

### Tuple constructor
If you infer the type of a variable with a list of expressions the type of said variable will be an implicitly created struct with nameless fields, this is a tuple constructor:
```py
foo := (1, 1.0, "hi"); # 'foo' will have the type of '(i32, f32, str)'
```

Keep in mind that the [padding rules](#Struct-padding) still applies:
```py
foo := (1, "hi"); # error: misaligned fields
```

To make a tuple constructor with just one field a trailing comma is necessary. Without it, it would be just a grouped expression:
```py
foo := (1,); # 'foo' will have the type of '(i32,)'
bar := (1);  # 'bar' will have the type of 'i32'
```

Same thing for an empty tuple constructor:
```py
foo := (,); # 'foo' will have the type of '(,)'
bar := ();  # 'foo' will have the type of the nothing type
```

To make the field have a name on a tuple constructor use `.<name> = <value>`:
```py
foo := (.x = 1, .y = 1.0, .name = "hi"); # 'foo' will have the type of '(x : i32, y : f32, name : str)'
```

And you obviously can mix and match the two:
```py
foo := (1, 1.0, .name = "hi"); # 'foo' will have the type of '(i32, f32, name : str)'
```

You can make unordered fields using `.<position> = <value>`:
```py
foo := (.3 = 1 -> u8, .2 = 2 -> u16, .1 = 3 -> u32, .0 = 1 -> u64); # 'foo' will have the type '(u64, u32, u16, u8)'
```

You can mix unordered fields with ordered ones:
```py
foo := (1, 2, .3 = 4, .2 = 3);
```

If you mix the two you cannot use the unordered field that was alread set by an ordered one:
```py
foo := (1, 2, .0 = 3); # error: setting field at position 0 twice
```

You can't skip fields:
```py
foo := (1, .2 = 3); # error: tuple constructor missing field at position 1
```

You can go back to using ordered fields after using unordered fields
```py
foo := (1, .2 = 3, .1 = 2, .3 = 4, 5, 6); # fine
```

And mixing named fields with unordered fields is possible:
```py
foo := (.name = "Jon", .1 = "Doe"); # fine
```

If you're using a tuple constructor to create a struct with only one field, and that field is named or by position you don't need the trailing comma:
```py
foo := (.x = 1); # 'foo' will have the type of '(x : i32)'
bar := (.0 = 2); # 'bar' will have the type of '(i32)'
```

Mixing tuple constructor and [struct destructuring](#Struct-destructuring) lets you create multiple variables with multiple types and values in one line:
```py
(a, b, c, d) := ("one", 2, 3.0, 4 -> u8);
```

#### Tuple constructor on pre-defined structs
If a variable already has an explicit type you can use a tuple constructure instead of the normal struct constructor:
```py
Point3D ::= (x, y, z : i32);
p0 : Point3D = (1, 2, 3); # valid, tuple constructor on pre-defined struct
p1 : Point3D = Point3D(1, 2, 3); # same thing, but using struct constructor
```

The tuple constructor will act exactly like the struct constructor. You can simply think of it as a struct constructor without the struct name.

The main difference is that `.<name> = <value>` and `.<position> = <value>` acts as unordered field initializations, like on normal struct constructors:
```py
p : Point3D;
p = (.z = 3, .0 = 1, .y = 2); # same as 'Point3D(.z = 3, .0 = 1, .y = 2)'
```

And you can actually skip positions:
```py
p : Point3D;
p = (1, .2 = 3); # valid
```

You can still let the default initialization of some fields:
```py
p : Point3D;
p = (1, 2); # 'z' is initialized to 0
```

Because this is a tuple constructor you still need the trailing comma for zero or one field argument:
```py
p0, p1, p2, p3 : Point3D;
p0 = (1,); # valid, 'x == 1', the rest is zeroed out
p1 = (,); # valid, every field is equal to 0
p2 = (1); # error: assigning expression with an 'i32' value to a 'Point3D' variable
p3 = (); # error: assigning expression with that returns nothing to a 'Point3D' variable
```

This type of constructor is convenient on all sort of ways, from function return values to [one-parem function calls](#Function-definition-and-calling-conventions):
```py
Vec2 ::= (x, y : f32);
doublev ::= (v : Vec2) Vec2 => (v.x * 2.0, v.y * 2.0);
_ := doublev(1.0, 2.0); # using tuple constructor on one-paremeter function call directly. equivalent to 'doublev Vec2(1.0, 2.0)'
```

### Struct mutability
Forward assigning is still possible with immutable structs:
```py
Foo ::= (u32, u32);
a : Foo; # not initialized yet
a = Foo(10, 20); # valid, initialized here
```

For the forward assigning to work, not only the whole struct cannot be read, but members as well:
```py
Foo ::= (u32, u32);
a : Foo; # not initialized yet
b := a.x; # 'b == 0'
a = Foo(10, 20); # invalid, 'a' is already initialized
```

Forward declaring works only for the whole struct, if you try to assign to a member the struct is assumed to already be properly initialiazed:
```py
Foo ::= (u32, u32);
a : Foo;
a.x = 10; # invalid, 'a' is immutable
```

For this to be possible, use a mutable instance:
```py
Foo ::= (u32 = 1, u32 = 2);
a : mut Foo; # initializes struct to 'x = 1' and 'y = 2'
a.x = 10; # valid, 'a' is mutable
```

As most things in stark, structs are anonymous. So it is possible to use the struct literal directly on a variable type, argument type, etc:
```py
foo : (x, y, z : i32); # Foo has the type '(x, y, z = i32)'
foo = (1, 2, 3); # valid
```

You can use the contructor on the struct literal directly:
```py
foo := (x, y, z : i32)(.z = 1, .x = 2, .y = 3); # valid
```

### 'imm' and 'renege':
Members mutabilities are based on the instance mutability. You can't use the `mut` attribute on field members:
```py
Some_Struct ::= (
  x : mut u32; # error: members can't have the 'mut' modifier
);
```

But if you want to enforce _immutability_ there is the `imm` modifier:
```py
Some_Struct ::= (x : i32, y : imm u32);
a : mut Some_Struct;
a.x = 10; # valid, 'a' is mutable
a.y = 20; # error: 'a.y' field is immutable
```

This modifier can only be used on struct members:
```py
a : imm i32; # error: 'imm' modifier is only valid on struct members
```

To be able to modify the field member use the `renege` keyword:
```py
Some_Struct ::= (x : i32, y : imm u32);
a : mut Some_Struct;
a.y renege = 20; # valid
```

`renege` will only work on mutable instances:
```py
Some_Struct ::= (x : i32, y : imm u32);
a : Some_Struct;
a.y renege = 20; # error: 'a' is immutable
```

You can mark a whole struct as `imm`:
```py
Some_Struct ::= imm (x : i32, y : u32);
a : mut Some_Struct;
a.x = 10; # error: 'a.x' field is immutable
a.y = 20; # error: 'a.y' field is immutable
```

### Constant members
Constant members are extremely similar to [functions constant arguments](#Variable-and-constant-arguments): They have to come before any variable member and they can't have default values:
```py
Vec2 ::= (x, y : T, T :: type); # invalid
Vec2 ::= (T :: type = u32, x, y : T); # invalid
Vec2 ::= (T :: type, x, y : T); # valid
```

Instantiating a struct with a constant member follow all the previous rules established:
```py
Foo ::= (T :: type is number, a : T, b : T = 10, c, d : T);
_ := Foo(.c = 10, .a = 4, .b = -15, .d = 4, .T = i32);
_ := Foo(i32, 9, .c = 11, .d = 12);
_ := Foo(i32, 9, .c = 11, 12); # a == 9, b == 10, c == 11, d == 12
_ := Foo(i32, 9, .c = 11, .b = 12); # error: invalid order
_ := Foo(i32); # a == 0, b == 10, c == 0, d == 0
_ := Foo(i32, .d = 12); # a == 0, b == 10, c == 0, d == 12
_ := Foo(i32, 9, .c = 11); # a == 9, b == 10, c == 11, d == 0
```

The `imp` attribute can be used on constant members:
```py
Vec2 ::= (T :: imp type is number, x, y : T);
_ := Vec2(1, 2);     # T == i32
_ := Vec2(1.0, 2.0); # T == f32
```

And follow the same rules as `imp` arguments:
```py
Vec2 :: (T :: imp type is number, x, y : f32); # error: 'T' needs to be used in other members
```

The type of a variable with a constant members is `Struct(<constant-arguments>)`, this is called a 'struct variant':
```py
Vec2 ::= (T :: imp type is number, x, y : T);
a := Vec2(1, 2);          # type_of(a) == Vec2(i32)
b := Vec2(1.2, 2.1);      # type_of(b) == Vec2(f32)
c : Vec2(i32) = Vec2(3, 4); # Explicitly typed
```

Structs with `imp` constant members cannot have an empty constructor:
```py
Vec2 ::= (T :: imp type is number, x, y : T);
a := Vec2(); # error: 'T' needs to be assigned
```

Assign the constant member explicitly if this is desired behaviour:
```py
Vec2 ::= (T :: imp type is number, x, y : T);
a := Vec2(.T = i32); # valid
```

Or set the variable type explicitly:
```py
Vec2 ::= (T :: imp type is number, x, y : T);
a : Vec2(i32) = Vec2(); # valid
```

It's possible to access constant members:
```py
Vec2 ::= (T :: imp type is number, x, y : T);
a := Vec2(1, 2);
b : a.T = Vec2(); # valid: 
```

But is not possible to modify them:
```py
Vec2 ::= (T :: imp type is number, x, y : T);
a := Vec2(1, 2);
a.T = f32; # error: 'T' is a constant
```

You can also access a constant member from a struct variant:
```py
Vec2 ::= (T :: imp type is number, x, y : T);
a := Vec2(1, 2);
b : Vec2(f32).T; # valid, 'b' type is 'f32'
```

It's possible to call a constructor from a constant argument. Think of the constant argument as simply an alias:
```py
Vec2 ::= (T :: imp type is number, x, y : T);
a := Vec2(Vec2(1, 2), Vec2(3, 4)); # a is of type 'Vec2(Vec2(i32))'
b := a.T(); # valid, 'b' type is 'Vec2(i32)'
```

To pass structs with constant members to functions the struct variant must be explicit:
```py
Vec2 ::= (T :: imp type is number, x, y : T);
addv ::= (a, b : Vec2(i32)) Vec2(i32) => Vec2(a.x+b.x, a.y+b.y);
_ := addv(Vec2(1, 2), Vec2(3, 4)); # valid
_ := addv(Vec2(1.0, 2.0), Vec2(3.0, 4.0)); # invalid
```

If generic behavior is desired use constant arguments:
```py
Vec2 ::= (T :: imp type is number, x, y : T);
addv ::= (T :: imp type is number, a, b : Vec2(T)) Vec2(T) => Vec2(a.x+b.x, a.y+b.y);
_ := addv(Vec2(1, 2), Vec2(3, 4)); # valid
_ := addv(Vec2(1.0, 2.0), Vec2(3.0, 4.0)); # valid
```

#### Compile-time only structs
If a struct has only constant members it becomes a compile-time only type:
```py
Vec2 ::= (T :: imp type is number, a, b :: T);
a := Vec2(1, 2); # error: 'Vec2' is a compile-time only type and can't be assigned to a variable
```

This is useful for something like this:
```py
Plugin ::= (
  T    :: imp type,
  init :: () T,
  run  :: (inst : T) ``,
);
load_plugin ::= (P :: Plugin) => {
  inst := P.init();
  P.run(inst);
}
```

### Static constants
Constant members are per instance constants and cannot have a default value. Static constants are per struct constants and they need to have a value.

To define a static constant use the following syntax:
```py
<struct>.<constant-name> :: [type] = <value>;
```

This constant can be accessed only via the struct name, not by it's instances:
```py
Point ::= (x, y : i32);
Point.ONE ::= Point(1, 1);
p0 := Point.ONE; # valid
p1 := p0.ONE; # error: 'ONE' is not a member of 'Point'
```

You can access static constants via struct variants:
```py
Vec2 ::= (T :: type is number, x, y : T);
Vec2.ONE ::= Vec2(1, 1);
v0 := Vec2(f32).ONE; # in this case a Vec2(i32) is being generated from a Vec2(f32) variant
```

Inside static constants you can use `~.<constant-member>` to access constant members of the current struct variant:
```py
Vec2 ::= (T :: type is number, x, y : T);
Vec2.ONE ::= Vec2(1 -> ~.T, 1 -> ~.T);
v0 := Vec2(f32).ONE; # generates an Vec2(f32) with x and y as 1
```

If a static constant uses `~.<constant-member>`, it must be called only from struct variants. Accessing it via the base struct will cause an error:
```py
Vec2 ::= (T :: type is number, x, y : T);
Vec2.ONE ::= Vec2(1 -> ~.T, 1 -> ~.T);
v0 := Vec2.ONE; # error: 'ONE' can only be accessed via 'Vec2' variants
```

Struct static variables aren't a thing.

#### Methods
Methods are static constant functions that accept a, mutable or immutable pointer, reference, view or direct value to the struct of the static constant itself as it's first variable argument:
```py
Vec2 ::= (x, y = f32);
Vec2.add ::= (self : Vec2, other : Vec2) => Vec2(self.x+other.x, self.y+other.y);
a := Vec2(1, 2);
b := Vec2(3, 4);
c := Vec2.add(b);
```

Methods are the only static constants that can be accessed via instances. This will actually pass the instance as the method first paremeter:
```py
Vec2 ::= (x, y : f32);
Vec2.add ::= (self : Vec2, other : Vec2) => Vec2(self.x+other.x, self.y+other.y);
a := Vec2(1, 2);
b := Vec2(3, 4);
c := a.add(b); # sugar for Vec2.add(a, b)
```

If the first argument is a pointer or something like that an implicit address will be taken:
```py
Vec2 ::= (x, y : f32);
Vec2.addeq ::= (self : *mut Vec2, other : Vec2) => *self = Vec2(self.x+other.x, self.y+other.y);
a := Vec2(1, 2);
b := Vec2(3, 4);
a.addeq(b); # sugar for Vec2.addeq(&a, b)
```

`self` is just an argument so it can use any of the argument definition established previously:
```py
Vec2 ::= (x, y : f32);
Vec2.add0 : (self : Vec2, other : Vec2) => ...; # valid
Vec2.add1 : (self, other : Vec2) => ...; # valid
```

It's a convention to call the `self` argument as `self`, it actually can be named as anything:
```py
Vec2 ::= (x, y : f32);
Vec2.add ::= (this, other = Vec2) => Vec2(this.x+other.x, this.y+other.y);
```

Methods and static constants as a whole are mainly used on structs, but they actually can be defined for _any_ type:
```py
i32.square ::= (self : i32) => self*self;
twenty_five := 5.square();
```

If the method takes only one argument besides the `self` argument the one-parameter call convention is possible:
```py
i32.add ::= (self, other : i32) => self+other;
twenty_five = 20.add 5;
```

So this is completely possible with the language rules:
```py
Vec2 ::= (x, y : f32);
Vec2.add ::= (self, other : Vec2) Vec2 => (self.x+other.x, self.y+other.y);
_ := Vec2(1.0, 2.0).add(3.0, 4.0);
```

If a method have the same name as a constant member or variable the member takes priority, so methods have to be called as standard static constant functions:
```py
Foo ::= (x : i32, one : i32 = 1);
Foo.one ::= (_self : Foo) i32 => 1;
a   := Foo(.one = 2);
one := mut i32;
one = a.one; # one == 2
one = Foo.one(a); # one == 1
```

Keep in mind that you technically can use [function pointers](#Function-pointers) to have functions directly inside structs:
```py
Vec2 ::= (
  x, y : f32,
  add : imm *(self : Vec2, other : Vec2) Vec2 =
    &((self, other = Vec2) Vec2 => (self.x+other.x, self.y+other.y)), # the 'Vec2' in here doesn't count as a recursive variable
)
```

This is not recommended if the intention is method behavior because: it'll waste memory, it'll be slower than methods because of function pointer derreferencing, the method-like call will not work, it's extremely cumbersome to write because members can't have type inference, and the member is not garanteed to always have the same beahvior even with `imm`.

#### Generics and static constants
When you define a function with a constant argument, what you're actually doing is creating a template. So if you try to access a static constant from a generic type of a function the function will simply assume that the type has the static constant. The same thing applies on method-like call:
```py
foo ::= (T :: type) u64 => {
  a := T.FOO;
  ret something() * T.FOO; # because of the usage 'T.FOO' is implied to be passable to an u64 
};
bar ::= (T :: imp type, a : T) => {
  a.something(T.FOO); # 'T.something' method is expected to be able to receive 'T.FOO' as it's first argument
};
Foo     ::= (a, b : i32);
Foo.FOO ::= 1;
Foo.something ::= (x : i32) => ...;
_ := foo(Foo); # valid
_ := foo(i32); # error: i32 doesn't have static constant 'FOO'
bar(Foo(1, 2)); # valid
bar(1); # error: i32 doesn't have static constant 'FOO' nor 'something'
```

So functions with generic types actually are duck typed at compile-time.

### 'take' attribute
There are times when you want to access the members of a member struct directly from the base struct. Use the `take` modifier for that:
```py
Vec2 ::= (x, y : f32);
Entity ::= (
  position : take Vec2,
  id : u32,
);
ent : mut Entity;
ent.x = 10; # valid, same as 'ent.position.x = 10'
```

If member names conflict between a base member and a member of a struct member, the base member takes priority:
```py
Vec2 ::= (x, y : f32);
Entity ::= (
  position : take Vec2,
  x : u32,
  id : u32,
);
ent : mut Entity;
ent.x = 10; # this modifies the 'Entity' 'x' not the position 'x'
```

If conflict between two `take` members arrises you simply can't use the shortcut:
```py
Vec2 ::= (x, y : f32);
Entity ::= (
  position : take Vec2,
  velocity : take Vec2,
  id : u32,
);
ent : mut Entity;
ent.x = 10; # error: ambiguous 'x' between 'position' and 'velocity' members
```

Methods from a `take` member have to be called through the member directly:
```py
Vec2 ::= (x, y : f32);
Vec2.addeq ::= (self : *mut Vec2, other : Vec2) => *self = Vec2(self.x+other.x, self.y+other.y);
Entity ::= (position : take Vec2, id : u32);
ent : mut Entity;
ent.position.addeq(Vec2(1, 2, 3)); # valid
ent.addeq(Vec2(1, 2, 3)); # error: 'add_eq' isn't a static function for the type 'Entity'
```

`take` also works with [alts](#Alts), [unions](#Unions), and basically any type with members.

### Structure of Arrays
Most languages have the Array of Structures layout as the default way to store buffers of structs.

With the AoS layout a struct that looks like this:
```py
Foo ::= (x, y, z : i32);
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

In Stark when creating an array of some struct you can use the `soa` modifier, that way this layout is achieve automatically:
```py
Foo ::= (x, y, z : i32);
foo_buf : soa [100]Foo;
for i in 0..@len(foo_buf) => foo_buf[i].x = i;
```

That way structure of arrays are handled in a extremely similar way to the familiar AoS.

Nested structures will also follow the SoA layout. This:
```py
Vec2   ::= (x, y : f32);
Entity ::= (position : Vec2, id : u32);
ent_buf : soa [100]Entity;
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

If a member struct is not supposed to unroll like this add the `crate` modifier to it:
```py
Vec2   ::= (x, y : f32);
Entity ::= (position : crate Vec2, id : u32);
ent_buf : soa [100]Foo;
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

Keep in mind that `soa` arrays are a different type than normal arrays. You can't pass them to normal functions:
```py
Vec2 ::= (x, y : f32);
do_stuff ::= (N :: imp usize, v : [N]Vec2) => ...;
buf : soa [100]Foo;
do_stuff(buf); # error: expected '[100]Foo' got 'soa [100]Foo'
```

#### SoA slices
You can take a slice from an `soa` array:
```py
Vec2 ::= (x, y : f32);
buf : soa [100]Foo;
slice := buf[..];
```

The type of an `soa` array slice is `soa [..]T`:
```py
Vec2 ::= (x, y : f32);
buf : soa [100]Foo;
slice : soa [..]Foo = buf[..];
```

And `soa` slices are also a different type from normal slices:
```py
Vec2 ::= (x, y : f32);
do_stuff ::= (v : [..]Vec2) => ...;
buf : soa [100]Foo;
do_stuff(buf); # error: expected '[..]Foo' got 'soa [..]Foo'
```

#### Views
There is some pitfalls for using SoA over AoS, the major one is passing pointers to individual elements of the array. This is actually not possible in Stark:
```py
Foo ::= (x, y, z : i32);
foo_buf : soa [100]Foo;
ptr_to_some_foo := &foo_buf[0]; # error: taking address of soa buffer element
```

If this is inteded behavior what you really want is a view, not a pointer. Views are a collection of pointers into an SoA buffer, to take a view of a struct element use the `[&]` operator:
```py
Foo ::= (x, y, z : i32);
foo_buf : soa [100]Foo;
view_to_some_foo := [&]foo_buf[0]; # valid
```

The "all values are immutable by default" rule applies to views, use `[&]mut` to take a mutable view:
```py
Foo ::= (x, y, z : i32);
foo_buf : mut soa [100]Foo;
view_to_some_foo := [&]mut foo_buf[0];
view_to_some_foo.x = 10; # foo_buf[0].x == 10
```

The type of a view is `[*]T` for immutable and `[*]mut T` for mutable:
```py
view0 : [*]Foo     = [&]foo_buf[0];
view1 : [*]mut Foo = [&]mut foo_buf[0];
```

It's possible to derreference a view:
```py
view := [&]mut foo_buf[0];
*view = Foo(1, 2, 3);
foo := *view; # 'foo' is of type 'Foo'
```

Keep in mind that SoA elements can't use the [arguments as references rule](#Immutable-arguments-are-references) for optimization. The values will have to be copied.

It is also only possible to use methods from SoA elements that receive a view or a value:
```py
Vec2             ::= (x, y : f32);
Vec2.add         ::= (self = Vec2, other : Vec2) Vec2 => Vec2(self.x+other.x, self.y+other.y);
Vec2.add_eq_view ::= (self : [*]mut Vec2, other : Vec2) => *self = Vec2(self.x+other.x, self.y+other.y);
Vec2.add_eq_ptr  ::= (self : *mut Vec2, other : Vec2) => *self = Vec2(self.x+other.x, self.y+other.y);
Entity           ::= (position : Vec2, id : u32);
ent : mut soa [10]Entity;
new_pos := ent[0].position.add Vec2(1, 2, 3); # valid
ent[0].position.add_eq_view new_pos; # valid
ent[0].position.add_eq_ptr new_pos; # error: 'ent[0]' can't be passed as a pointer
```

### Struct padding
All types have an alignment. It's generally 1, 2, 4 or 8 bytes, but it can be more (like on [integers](#Integers) or [SIMD vectors](#SIMD-vectors)).

You can use the `align_of` function of [the base module](#The-base-module) to check the alignment of a certain type:
```py
a := @align_of(u8);             # a == 1
b := @align_of(i64);            # b == 8
c := @align_of(ubits(256));     # c == 32
d := @align_of(simd vec[4]f32); # d == 16
```

The alignment of a struct will be equal to the alignment of the member with biggest alignment:
```py
Foo ::= (x : u64, y : u32);
a := @align_of(Foo) == @align_of(u64); # a == true, @align_of(Foo) == 8
```

If the last field member does not guarantee that the size of the struct will be a multiple of the struct alignment, the necessary padding will be implicitly added:
```py
Foo0 ::= (x : u64, y : u32); # implicit padding of 4 bytes at the last field
Foo1 ::= (x : u64, y : u32, z : u8); # implicit padding of 3 bytes at the last field
```

The only member that has implicit padding is the last member, if a misalignment occurs within the internal members an error will be thrown to prevent hidden padding or undefined layout:
```py
Foo ::= (y : u32, x : u64); # error: misalignment between 'x' and 'y' members
```

You can add explicit padding using the [`_`](#Unused-variables-and-the-_-identifier) identifier:
```py
Foo ::= (y : u32, _ : [4]u8, x : u64); # valid, explicit alignment
```

And of course, `_` can be defined more than once and it'll not be accessable for read or writting:
```py
Foo ::= (x : u32, _ : [4]u8, y : u8, _ : [7]u8, z : u64);
foo : mut Foo;
a := foo._[0]; # error: '_' field can't be read from
foo._[0] = 1; # error: '_' field can't be written to
```

Implicit padding on internal field members is not permitted to avoid things like this:
```py
Foo ::= (a : u16, b : u32, c : u16);
a := @size_of(Foo); # a == 12
```

Because the implicit padding isn't allowed your structs are guaranteed to have the most optimal layout:
```py
Foo ::= (b : u32, a, c : u16); # the compiler will guide you to this layout through errors
a := @size_of(Foo); # a == 8
```

#### Packed structs
You can permit misaligned fields and disable struct modifier `pack`:
```py
Some_Unpacked_File_Header ::= (tag : [4]char8, big_id : u64, size : u16);      # error: misalignment between 'tag' and 'big_id'
Some_Packed_File_Header   ::= pack (tag : [4]char8, big_id : u64, size : u16); # valid
```

Packed structs will also disable the implicit padding of the last member:
```py
Some_Unpacked_File_Header ::= (big_id : u64, tag : [4]char8, size : u16);
Some_Packed_File_Header   ::= pack (big_id : u64, tag : [4]achar, size : u16);
a := @size_of(Some_Unpacked_File_Header); # a == 16
b := @size_of(Some_Packed_File_Header);   # b == 14
```

#### Explicit alignment
You can explicitly set the alignment of a struct with the `alignas(N)` modifier:
```py
Foo ::= alignas(4) (x : u64, y : u32);
```

`N` is in bytes and has to be a power of 2:
```py
Foo ::= alignas(3) (x : u64, y : u32); # error: invalid alignment
```

You can mix the `pack` and `alignas(N)` modifiers:
```py
Foo ::= alignas(4) pack (x : u64, y : u32); # valid
```

## Destructuring
### Struct destructuring
You actually can instantiate a struct with all of it's members being actual variables in the main scope. This is called destructuring:
```py
Vec2 ::= (x, y : f32);
(x, y) := Vec2(1.0, 2.0);
```

The variables created can have any name you want:
```py
(a, b) := Vec2(1.0, 2.0);
```

And you still can use explicit typing:
```py
(a, b) : Vec2 = Vec2(1.0, 2.0);
```

Because of this you can destructure using the default values of fields easily:
```py
Foo ::= (name : str = "Jon Doe", id : u32, age : u32 = 20);
(name, id, age) : Foo; # name == "Jon Doe", id == 0, age == 20
```

You can skip fields using the ['_' identifier](#Unused-variables-and-the-_-identifier):
```py
Vec3 ::= (x, y, z : f32);
(_, y, _) := Vec3(1.0, 2.0, 3.0);
```

You can also destructure with previously defined variables:
```py
a : f32;
(a, b, c) := Vec3(1.0, 2.0, 3.0); # valid, using previously defined 'a'
```

You can control the mutability of new defined variables through the destructuring:
```py
(a, b, c) := mut Vec3(1.0, 2.0, 3.0); # 'a', 'b' and 'c' are mutable
```

All of the individual fields will be treated as mutable values, thus you can't pass them to pre-defined immutable variables:
```py
a : f32;
(a, b, c) := mut Vec3(1.0, 2.0, 3.0); # error: passing mutable value to immutable variable 'a'
```

This:
```py
(a, b, c) := mut Vec3(1.0, 2.0, 3.0);
(a, b, c) := mut Vec3(4.0, 5.0, 6.0);
```

Is not shadowing. As stated before you can use pre-defined variables when destructuring and that's what's happening here. `a`, `b`, `c` are defined on the first destructuring and then reused on the next one.

You can partially destructure a struct, this'll implicitly create a struct with the remaining fields and the last variable will hold that struct:
```py
Foo ::= (a : i32, b : f32, c : u8, d : str);
(a, b, rest) := Foo(1, 2.0, 3, "four");
c := rest.c; # c == 3
d := rest.d; # d == 'four'
e := rest.a; # error: variable 'rest' doesn't have field 'a'
```

Because of this you can discard all of the remainder fields of a structure using `_`:
```py
(a, b, _) := Foo(1, 2.0, 3, "four"); # only grabbed '1' and '2.0'
```

You can destructure by field names directly, this way you don't need to match positions:
```py
(.y, .x) := Vec2(1.0, 2.0);
a := y; # a == 2.0
b := x; # b == 1.0
```

Even on field named destructuring you can use pre-defined variables:
```py
y : f32;
(.y, .x) := Vec2(1.0, 2.0); # using the 'y' defined above, 'x' is defined here
```

You can mix field named and positional destructuring:
```py
(a, b, .w, .z) := Vec4(1.0, 2.0, 3.0, 4.0); # valid
```

But after using the field name syntax you can use positional destructuring anymore:
```py
(a, .y, b, c) := Vec4(1.0, 2.0, 3.0, 4.0); # error: positional destructuring after named destructuring
```

Unless you use it to grab the remainder of the structure:
```py
(a, .w, b) := Vec4(1.0, 2.0, 3.0, 4.0); # valid, 'b' hold fields 'y' and 'z'
```

This way you can destructure only an specific field:
```py
(.z, _) := Vec4(1.0, 2.0, 3.0, 4.0);
```

Nested destructuring is possible:
```py
Vec2   ::= (x, y : f32);
Entity ::= (pos : Vec2, id : u32);
((x, y), id) := Entity(Vec2(1.0, 2.0), 3);
```

And it works with [array and vectors](#Array-and-vector-destructuring) well:
```py
Entity ::= (pos : vec[2]f32, tag : [4]u8);
([x, y], [a, b, c, d]) := Entity(vec[1.0, 2.0], [3, 4, 5, 6]);
```

#### Patterned destructuring
You can use a literal instead of a variable on the left-hand-side of a destructuring expression. This is a patterned destructuring:
```py
_ := (0.0, y) := Vec2(1.0, 2.0);
```

The literal has to be of the same type as the field of the struct being destructured and the type has to be comparable (overloads the `==` operator) for it to work.

This in itself will be an expression that returns a boolean:
```py
x_is_zero := (0.0, y) := Vec2(1.0, 2.0); # x_is_zero == false
```

Because that's an expression that returns a boolean instead of nothing you actually need to resolve the result:
```py
(0.0, y) := Vec2(1.0, 2.0); # error: unhandled expression
```

If all of the patterned destructuring fields are literals it's basically a comparison:
```py
a := (1.0, 2.0) := Vec2(1.0, 2.0); # a == true
```

You can use a boolean expressions instead of just a literal on patterned destructuring:
```py
x_is_less_than_ten := (x < 10.0, y) := Vec2(1.0, 2.0); # x_is_less_than_ten == true
```

The variable used on the expression will be assigned to the value of the field and if it's not yet defined it'll be defined:
```py
x_is_less_than_ten := (x < 10.0, y) := Vec2(1.0, 2.0); # x_is_less_than_ten == true
a := x; # 'a == 1.0', 'x' was defined and assigned to on the desctruturing
```

The only valid expressions on those types of destructuring are:
- _Equal to:_ `==`
- _Not equal to:_ `!=`
- _Less than:_ `<`
- _Greater than:_ `>`
- _Less or equal to:_ `<=`
- _Greater or equal to:_ `>=`
- _Not:_ `!`

All of the booleans expressions can be chained by `&&` or `||`. And they can be grouped with parenthesis:
```py
foo := ((x < 10.0 && x > 5.0) || x == 1.0, y) := Vec2(1.0, 7.0); # x_is_range == true
```

The left-hand-side of the boolean expression can be any type of expression that ends up return the proper expected type. But it has to have exactly only one variable. This is the variable that the field will be assigned to:
```py
square ::= (x : f32) f32 => x * x;
_ := (a + 6 == 10.0, b) := Vec2(4.0, 5.0); # valid
_ := (square a == 10.0, b) := Vec2(4.0, 5.0); # valid, 'a' will be detected as the variable that the field will be assigned to
_ := (c + a == 10.0, d) := Vec2(4.0, 5.0); # error: more than one variable on left-hand-side of patterned destructuring boolean expression
_ := (10.0 == a, d) := Vec2(4.0, 5.0); # error: no variable on left-hand-side of patterned destructuring boolean expression
```

This happens to prevent ambiguos assignment of a destructured field.

If you used the named fields directly you can have more than one variable at the left-hand-side:
```py
_ := (.x + a == 10.0, .y) := Vec2(4.0, 5.0); # valid
```

The right-hand-side can have any number of variables, literals and so on:
```py
TEN ::= 10.0;
ten := 10.0;
foo := (x == 10.0, y) := Vec2(4.0, 5.0); # valid
bar := (x == TEN, y)  := Vec2(4.0, 5.0); # valid
baz := (x == ten, y)  := Vec2(4.0, 5.0); # valid
```

You can use pre-defined variables on patterned destructuring:
```py
a := 0.0;
foo := (a == 10.0, y) := Vec2(4.0, 5.0); # valid, 'a == 4.0' and 'foo == false'
```

And destructuring by field names is also still possible as showed above:
```py
foo := (.x == 10.0, .y) := Vec2(4.0, 5.0); # valid
```

You can use `_` on patterned expressions to still discard the value at the end, `_` will represent the current field:
```py
x_is_in_range := (_ > 5.0 && _ < 10.0, y) := Vec2(4.0, 5.0); # x_is_in_range == false
```

### Array and vector destructuring
Destructuring is also available for [arrays](#Arrays) and [vectors](#Vectors):
```py
[a, b, c] := [1, 2, 3]; # valid
[a, b, c] := vec[1, 2, 3]; # valid
```

All the forms of destructuring, including patterned destructuring, that are available for structs are also available for arrays/vectors:
```py
[a, b] := [1, 2]; # valid
_ := [1, b] := [1, 2]; # valid
_ := [a == 1, b] := [1, 2]; # valid
[a, b] := vec[1, 2]; # valid
_ := [1, b] := vec[1, 2]; # valid
_ := [a == 1, b] := vec[1, 2]; # valid
```

It also works for [SIMD vectors](#SIMD-vectors):
```py
[a, b, c, d] := simd vec[1, 2, 3, 4]; # valid
```

The only exception is named fields destructuring.

For arrays you simply can't have it, that's because arrays don't have named fields.

For vectors you can have for the names `x`, `y`, `z`, `w`, `r`, `g`, `b` and `a`:
```py
(.x, .y, .z, .w) := vec[1, 2, 3, 4]; # valid
(.r, .g, .b, .a) := vec[1, 2, 3, 4]; # valid
```

You can still get the remainder elements of an array. The last field will be an array with the remainder elements:
```py
[a, b, arr] := [1, 2, 3, 4]; # 'arr' is an array of type '[2]i32' with the value '[3, 4]'
```

Same thing for vectors:
```py
[x, y, v] := vec[1, 2, 3, 4]; # 'v' is a vector of type 'vec[2]i32' with the value 'vec[3, 4]'
```

## Alts
Alts are a collection of _tags_. Each tag represents a unique form of the alt, associated with a specific type. The _tag_ acts as the discriminator, and the _type_ defines how data must be held. 

To define an alt tag, first put the name of the tag, followed by the desired type to be associated with it (e.g. `tag type`). To add an additional tag use `|`, meaning 'or', as a separator. Every tag must be enclosed by a single pair of parenthesis to be a valid alt definition:
```py
Some_Alt ::= (tag0 type0|tag1 type1|...);
```

A simple example:
```py
Some_Struct ::= (a, b, c : i32);
Numbers ::= (Integer i32|Floating_Point f32|A_Tag Some_Struct);
```

Trailing `|` are allowed:
```py
Some_Struct ::= (a, b, c : i32);
Numbers ::= (Integer i32|Floating_Point f32|A_Tag Some_Struct|);
```

Infact to define an alt with only one tag, trailing `|` are required:
```py
Foo ::= (Integer i32); # Can be ambiguious in some cases
Bar ::= (Integer i32|); # Alt with one tag
```

And of course, direct struct literals are allowed:
```py
Numbers ::= (Integer i32|Floating_Point f32|A_Tag(a, b, c : i32)|);
```

Two different tags can have the same underlying type:
```py
Numbers ::= (Integer i32|Other_Integer i32); # 'Integer' and 'Other_Integer' are two different tags
```

The size of an alt type will be the size of the largest tag type plus the size of the type of the tag numeric representation type. The alignment will be either equal to the tag numeric representation type or the largest tag type, whichever is larger.

### Repeated tag name and tag type
Tags can have names of outside constants and predefined types:
```py
Vec2 ::= (x, y : u32);
Numbers ::= (i32 i32|f32 f32|Vec2 Vec2); # valid
```

This is repetitive. To overcome this problem add `+` as a suffix to the tag name:
```py
Vec2 ::= (x, y : u32);
Numbers ::= (i32+|f32+|Vec2+); # shortcut for (i32 i32|f32 f32|Vec2 Vec2)
```

### Tags numeric representation
Alts tags are just integers IDs. The first tag defaults to 0 and each subsequent tag increment by 1 automatically.

It's possible to set the tag value manually:
```py
Some_Alt ::= (Tag0 i32 = 1|Tag1 u32 = 2|Tag2 f32 = 0);
```

Tags with the same underlying type can have the same numeric value:
```py
Some_Alt ::= (Tag0 i32 = 0|Tag1 i32 = 0); # valid
```

And tags with different underlying types cannot have the same numeric value:
```py
Some_Alt ::= (Tag0 i32 = 0|Tag1 u32 = 0); # error: 'Tag0' and 'Tag1' share same tag number, but have diffent underlying types
```

Also, after setting a tag numeric value manually subsequent tags still have the value incremented manually:
```py
Some_Alt ::= (Tag0 i32|Tag1 u32 = 3|Tag2 f32); # Tag0 == 0, Tag1 == 3, Tag2 == 4
```

Tags can be negative:
```py
Some_Alt ::= (Tag0 i32 = -2|Tag1 u32|Tag2 f32); # Tag0 == -2, Tag1 == -1, Tag2 == 0
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

After exceding `u64`/`i64` ranges the next valid `ibits(N)`/`ubits(N)` will be used:
```py
Some_Alt0 ::= (Tag0 i32 = 18_446_744_073_709_551_616|); # tag numeric type will be 'ubits(128)'
Some_Alt1 ::= (Tag0 i32 = 340_282_366_920_938_463_463_374_607_431_768_211_456|); # tag numeric type will be 'ubits(256)'
```

But if you desire to enforce a specific integer type on the tags you can do so:
```py
Some_Alt ::= (Tag0 i32 = 256) u8; # error: alt with tags of type 'u8' can't hold the tag numeric value of '256'
```

### Alt constructors
You can instantiate an alt using the syntax `<alt>.<tag> <value>`:
```py
Vec2       ::= (x, y : f32);
Some_Alt ::= (Number i32|String str|Vector Vec2);
number := Some_Alt.Number 10;
string := Some_Alt.String "20";
vector := Some_Alt.Vector Vec2(1.0, 2.0);
```

Because each alt tag already holds the proper type, you can [initialize a struct without the explicit type](#Implicit-struct-name-on-instantiation):
```py
vector := Some_Alt.Vector(1.0, 2.0); # valid, same as 'Some_Alt.Vector Vec2(1.0, 2.0)'
```

Keep in mind that the actual type of the alt isn't `<alt>.<tag>`, just `<alt>`. The tag just signals to the compiler how the variable data and type checking should be treated. So a explicitly typed alt would be:
```py
vector : Some_Alt = Some_Alt.Vector(1.0, 2.0);
```

If the variable already has the proper alt type you can use `.<tag>` as a shortcut:
```py
Some_Alt ::= (Number i32|String str);
foo ::= (_ : Some_Alt) =>;
foo(.Number 10); # valid
x : Some_Alt;
x = .String "hello"; # valid
```

#### Alt default tag
The default value of an alt is always the first declared tag (the tag numeric value does not matter):
```py
Some_Alt ::= (Number i32|String str);
num0 : Some_Alt;
num1 := Some_Alt.Number 10;
```

In the code above, both `num0` and `num1` have `Some_Alt.Number` as their tag numeric representation.

The actual data of the tag will be defaulted:
```py
Some_Alt ::= (Number i32|String str);
num : Some_Alt; # 'num' is defaulted to 'Number' and the value is defaulted to '0'
```

### Accessing alt data
To access the data of an alt use `^` as a suffix:
```py
Vec2 ::= (x, y : f32);
Some_Alt ::= (Number i32|String str|Vector Vec2);
vector := Some_Alt.Vector(1.0, 2.0);
number := Some_Alt.Number 10;
x = vector^.x; # valid, x == 1.0
a = number^; # valid, a == 10
```

The `^` suffix can only be used when it's absolutely clear to the compiler what's the tag of the giving alt. If it isn't use a `match` or `if` expression:
```py
match some_alt
  .Int   => some_alt^ = 10,
  .Float => some_alt^ = 10.0;
if some_alt == .Float do some_alt^ *= 0.5;
```

### Alt mutability
The mutability of alts works in [the same way as the rest of the variables](#Variable-mutability). If you wish to change a value from a type within an alt use the `mut` attribute as always:
```py
Vec2 ::= (x, y : f32);
Some_Alt ::= (Number i32|String str|Vector Vec2);
vector := mut Some_Alt.Vector(1.0, 2.0);
vector^.x = 10!; # valid
```

Mutable alts also permit you to directly change its tag:
```py
vector := mut Some_Alt.Vector(1.0, 2.0);
vector = .String "hi"; # valid
```

### Tags comparability
You can compare alts with tags:
```py
vector := Some_Alt.Vector(1.0, 2.0);
vector == Some_Alt.Vector; # true
```

This is not a data comparison, it just compares the tags numeric values.

The alt name can also be omitted:
```py
vector == .Vector; # valid
```

### Getting the tag numeric value
If a tag numeric value is needed use `^` as a prefix:
```py
Some_Alt ::= (Number i32|String str|Vector Vec2);
vector := Some_Alt.Vector(1.0, 2.0);
tag_val := ^vector; # in this case: 'tag_val == 2' and '@type_of(tag_val) == u8' 
```

It can be used directly from alt tags:
```py
Some_Alt ::= (Number i32|String str|Vector Vec2);
tag_val := ^Some_Alt.String; # in this case: 'tag_val == 1' and '@type_of(tag_val) == u8' 
```

You can use a specific variant to get the alt tag number:
```py
Some_Alt ::= (T :: type is number|Number T|String str|Vector Vec2);
tag_val = ^Some_Alt(i32).String; # tag_val == 0
```

But it's not necessary, unless the variant is needed to determine the tags:
```py
Some_Alt0 ::= (T :: type is number|Number T|String str|Vector Vec2);
Some_Alt1 ::= (N :: anyi|Number T = N|String str|Vector Vec2);
tag_val0 = ^Some_Alt0.String; # valid, tag_val0 == 0
tag_val1 = ^Some_Alt1.String; # error: Can't determine the tag of alt without variant
```

The inferred type based on a tag numeric value is equal to the [tag numeric value type](#Tags-numeric-representation). But if the type is not enforced tags numeric values can be passed into any integer that can hold the specified range:
```py
Some_Alt0 ::= (Number i32|String str|Vector Vec2);
tag_val0 := ^Some_Alt0.String; # tag_val0 type is u8, tag_val == 1
tag_val1 : i32 = ^Some_Alt0.String; # tag_val1 type is i32, tag_val == 1
Some_Alt1 ::= (Number i32|String str|Vector Vec2) i32;
tag_val2 := ^Some_Alt1.String; # tag_val2 type is i32, tag_val == 1
tag_val3 : u8 = ^Some_Alt1.String; # error: tags from 'Some_Alt1' are i32
```

Note that the alt name type can't be omitted for getting tags because there aren't a way to know which alt you're refering to:
```py
Some_Alt0 ::= (Number f32|String str|Vector Vec2);
Some_Alt1 ::= (Number i32|String str|Vector Vec2);
tag_val := ^.Number; # error: missing alt name after '^'
```

### Untyped alts
The type of a tag can be [the nothing type](#The-nothing-type), simply ommit the type of the tag for it to work:
```py
Color ::= (Red|Green|Blue);
color := mut Color;
color = .Red;
```

You can have some tags be of the nothing type and some don't:
```py
Some_Alt ::= (Int i32|Float f32|Red|Green|Blue);
```

Keep in mind that these tags:
```py
Number32 ::= (u32|i32|f32);
```

Aren't from the types `u32`, `i32` and `f32`. All of them are tags with the nothing type.

You can use the nothing type with backticks, it'll have the same effect:
```py
Color ::= (Red ``|Green ``|Blue ``);
color := mut Color;
color = .Red;
```

If the nothing type is used the alt will only have sufficient memory for its tag. Thus becoming equivalent to an enum of other languges.

### Passing alts as arguments
You can pass alts to arguments as expected:
```py
Some_Alt   ::= (Int i32|Float f32|String str);
do_something ::= (a : Some_Alt) => ...;
foo := Some_Alt.Int 10;
bar := Some_Alt.Float 10.0;
baz := Some_Alt.String "10";
do_something(foo); # valid
do_something(bar); # valid
do_something(baz); # valid
```

But if you want, you can determine that a function can only accept one specific alt tag:
```py
Some_Alt   ::= (Int i32|Float f32|String str);
do_something ::= (a : Some_Alt.Int) => ...;
foo := Some_Alt.Int 10;
bar := Some_Alt.Float 10.0;
baz := Some_Alt.String "10";
do_something(foo); # valid
do_something(bar); # error: passed 'Some_Alt.Float', but expected 'Some_Alt.Int'
do_something(baz); # error: passed 'Some_Alt.String', but expected 'Some_Alt.Int'
```

With this approach you're still passing an alt type, not just the tag value.

It's also possible to expected just more than one specific tag with a `|`:
```py
Some_Alt   ::= (Int i32|Float f32|String str);
do_something ::= (a : Some_Alt.Int|.Float) => ...; # 'a : Some_Alt.Int|Some_Alt.Float' is also valid
foo := Some_Alt.Int 10;
bar := Some_Alt.Float 10.0;
baz := Some_Alt.String "10";
do_something(foo); # valid
do_something(bar); # valid
do_something(baz); # error: passed 'Some_Alt.String', but expected 'Some_Alt.Int'
```

It's not possible to have a variable be from two different distinct alt types using this though:
```py
Some_Alt0  ::= (Int i32|Float f32);
Some_Alt1  ::= (Int i32|Float f32);
do_something ::= (a : Some_Alt0.Int|Some_Alt1.Float) => ...; # invalid
```

Keep in mind that passing specific tagged functions to arguments is only possible when it's clear to the compiler which tag that alt has. Similar to [the '^' suffix](#Accessing-alt-data).

### Constant alt members
[Similarly to structs](#Constant-members), alts can have constant members. They follow the same rules and are defined in the same way as constant members. The only definition difference is the separator, it's a pipe instead of a comma:
```py
Some_Alt ::= (T :: type|N :: usize|Type T|Buffer [N]T|);
```

To instantiate an alt with constant member you use the syntax `<alt>(const0, const1, ...)`. These are alt variants:
```py
Some_Alt ::= (T :: type|N :: usize|Type T|Buffer [N]T|);
type_i32 := Some_Alt(i32, 3).Type 10;
buff_i32 := Some_Alt(i32, 3).Buffer [1, 2, 3];
type_f32 := Some_Alt(f32, 3).Type 10;
buff_f32 := Some_Alt(f32, 3).Buffer [1, 2, 3];
```

The type of the variable will be the alt variant:
```py
buff_u64 := Some_Alt(f32, 3).Buffer [1, 2, 3];
_ := @type_of(buff_u64) == Some_Alt(f32, 3); # true
```

Alts constant members can't really accept `imp`. The reason for this is because not all constant members may be used on all tag types. 

An alt can only accept struct variants, so if being generic is a desired factor the alt will also have to accept the required constant members:
```py
Vec2 ::= (T :: imp number, x, y : T);
Some_Alt ::= (T :: type is number|Int i32|Vec Vec2(T));
a := Some_Alt(f32).Vec(1.0, 2.0);
```

### Alts static constants
Alts can have [static constants](#Static-constants), and they work exactly like the struct ones:
```py
Some_Alt ::= (Int i32|Float f32);
Some_Alt.ZERO ::= 0;
a := Some_Alt.ZERO;
```

Confliction between tag names and static constant names isn't permitted:
```py
Some_Alt ::= (Int i32|Float f32);
Some_Alt.Int ::= i32; # error: conflict between static constant 'Int' and tag 'Int'
```

You can also have static constants on specific tags:
```py
Some_Alt ::= (Int i32|Float f32);
Some_Alt.Int.ZERO ::=- 0;
Some_Alt.Float.ZERO ::= 0.0;
a := Some_Alt.Int.ZERO; # a == 0
b := Some_Alt.Float.ZERO; # b == 0.0
```

You can still grab static constants from the underlying type of a tag:
```py
i32.FOO ::= 1 -> i32;
Some_Alt ::= (Int i32|Float f32);
a := Some_Alt.Int.FOO; # a == 1
```

If a static constant from a tag and the underlying type conflicts, the tag takes priority:
```py
i32.FOO ::= 1 -> i32;
Some_Alt ::= (Int i32|Float f32);
Some_Alt.Int.FOO ::= 2 -> i32;
a := Some_Alt.Int.FOO; # a == 2
```

[Methods](#Methods) also behave the same:
```py
Some_Alt ::= (Int i32|Float f32);
Some_Alt.do_something ::= (self : Some_Alt) =>...;
a := Some_Alt.Int 10;
b := Some_Alt.Float 10.0;
a.do_something();
b.do_something();
```

Methods of alts also have priority over the underlying type:
```py
i32.square ::= (self : i32) => self*self;
Some_Alt ::= (Int i32|Float f32);
Some_Alt.square ::= (self : Some_Alt) => match self
  .Int   => self^ * self^ * self^,
  .Float => self^ * self^ * self^;
a := Some_Alt.Int 10;
b := a.do_something(); # b == 1000
```

Methods for alts has to have a pointer, view or value to the type of the specified alt. It can't be specific tags for the method call to work:
```py
Some_Alt ::= (Int i32|Float f32);
Some_Alt.do_something ::= (self : Some_Alt.Int) =>...;
a := Some_Alt.Int 10;
a.do_something(); # error: can't do method call on 'Some_Alt.do_something' function
```

But you can do a method call from a function bounded to a specific tag:
```py
Some_Alt ::= (Int i32|Float f32);
Some_Alt.Int.do_something ::= (self : Some_Alt.Int) =>...;
a := Some_Alt.Int 10;
b := Some_Alt.Float 10.0;
a.do_something(); # valid
b.do_something(); # error: 'b' has no method 'do_something'
```

Methods for specific alt tags have to have a pointer, view or value to that specific tag only.

Specific tag methods have priority over whole alt methods:
```py
Some_Alt ::= (Int i32|Float f32);
Some_Alt.do_something ::= (self : Some_Alt) =>...;
Some_Alt.Int.do_something ::= (self : Some_Alt.Int) =>...;
a := Some_Alt.Int 10;
b := Some_Alt.Float 10.0;
a.do_something(); # Some_Alt.Int.do_something(a);
b.do_something(); # Some_Alt.do_something(b);
```

## Unions
Sometimes you want to have the same data, but be able to interpret it in various ways. For this use a union.

To define a union use the following syntax:
```py
Some_Union ::= (member0 : type0|member1 : type1|...);
```

You'll then be able to reinterpret the data at any moment:
```py
Word ::= (value : u16|split : (low, high : u8));
a : mut Word;
a.value = 0x1234;
low  := a.split.low; # low  == 0x34
high := a.split.high # high == 0x12
```

### Union constructors
You can construct a union with a value directly, but only for a specific member, using the syntax `<union>.<member> <value>`:
```py j́
Word ::= (value : u16|split : (low, high : u8));
a := Word.value 0x1234; # valid
b := Word.split(0x34, 0x12); # valid
```

Union members can't have default values:
```py
Word ::= (value : u16 = 0x1234|split : (low, high : u8)); # error: default value on union member
```

This happens because all of the members share the same memory, it wouldn't really make much sense to default it.

If you don't use a constructor all of the data of the union will be defaulted to 0, even if it uses a struct that has another type of default:
```py
Split ::= (low : u8 = 0x78, high : u8 = 0x56);
Word  ::= (value : u16|split : Split);
a : Word;
b := a.split.high; # b == 0
```

You can also provide a `<union>()` constructor, this will ensure that the data for a union is all defaulted to 0:
```py
a := Word();
b := a.split.high; # b == 0
```

The difference between `a : Word` and `a := Word()` is that with the latter you obviously can't forward assign:
```py
a : Word;
a = Word.value 1; # valid
b := Word();
b = Word.value 2; # error: 'b' is immutable
```

Another type of constructor is the `<union> ---`, this will not initialize the data of a union to 0 and it'll actually use stack garbage:
```py
a := Word ---;
b := a.split.high; # b == <some-stack-garbage>
```

If you use the garbage initializer on just a member in specific, the remaining set of bytes will be actually 0 initialized:
```py
Some_Union ::= (a : u8|b : (x, y : u8));
a := Some_Union.a ---;
b = a.b.y; # b == 0
```

### Constant union members
[Similarly to structs](#Constant-members) or [alts](#Constant-alt-members), unions can have constant members. They follow the same rules as previously established:
```py
Some_Alt ::= (T :: type|N :: usize|value : T|bytes : [N]u8|);
```

To instantiate a union with constant members you use the syntax `<alt>(const0, const1, ...)`. These are union variants:
```py
Some_Alt ::= (T :: type|value : T|bytes : [@size_of T]u8|);
a := Some_Alt(i32).value 1;
b := Some_Alt(f32).value 2.0;
```

The type of the variable will be the union variant:
```py
a : Some_Alt(i32);
```

Similar to alts constant members, union ones also can't accept the `imp` modifier:
```py
Some_Alt ::= (T :: imp type|value : T|bytes : [@size_of T]u8|); # error: 'imp' modifier on union constant member
```

For a more in-depth reasoning for this see the [alts constant members section](#Constant-alt-members).

### Unions static constants
Unions can have static constants like [structs](#Static-constants) and [alts](#Alts-static-constants):
```py
Some_Union ::= (i : i32|f : f32);
Some_Union.ZERO ::= 0;
a := Some_Union.ZERO;
```

Confliction between members of a union and static constant names isn't permitted:
```py
Some_Union ::= (i : i32|f : f32);
Some_Union.i ::= 10; # error: conflict between static constant 'i' and member 'i'
```

This is to avoid confusion with union constructors.

[Methods](#Methods) also behave the same:
```py
Some_Union ::= (i : i32|f : f32);
Some_Union.do_something ::= (self : Some_Union) =>...;
a := Some_Union.i 10;
b := Some_Union.f 10.0;
a.do_something();
b.do_something();
```

And you can still call methods on members:
```py
Some_Union ::= (i : i32|f : f32);
i32.square ::= (x : i32) i32 => x*x;
a := Some_Union.i 10;
b := a.i.square(); # valid
```

## Casting
Stark is strongly typed. You can't pass a u8 to a f32 or even an i8 to an i16. But sometimes casting is needed, on those cases use the `->` operator:
```py
a := 1.5;
b := a -> i32; # now 'b' is an i32 with a value of 1
```

Chained casting is valid:
```py
a := 1.5;
b := a -> i32 -> u8; # 'b' is an u8 with value 1
```

### Pointer casting
The `->` operator can also be used on pointers:
```py
a  := i32;
p0 := &a;
p1 := p0 -> *i8; # p1 points to 'a' as if it was a '*i8'
```

It's not possible to cast an immutable pointer into a mutable one:
```py
a  := i32;
p0 := &a;
p1 := p0 -> *mut i8; # error
```

But the opposite is fine:
```py
a  := mut i32;
p0 := &mut a;
p1 := p0 -> *i8; # valid
```

Casting between pointers and integers is possible:
```py
a  := i32;
p0 := &a;
i0 := p0 -> u64; # valid
i1 := p0 -> isize; # valid
i2 := p0 -> i8; # valid
p1 := i0 -> *i32; # valid
```

Keep in mind that casting between pointers, and specially pointer-integer casting, can be extremely unsafe:
```py
p := 0xdeadbeef -> *i32;
a := *p; # program will crash
```

It's not possible to cast an integer into a mutable pointer:
```py
p := 0xdeadbeef -> *mut i32; # error
```

### String casting
You can cast from `cstr` to `astr`:
```py
some_str := c"hello" -> astr;
```

From `cstr` to `str`:
```py
some_str := c"hello" -> str;
```

From `cstr` to `nstr`:
```py
some_str := c"hello" -> nstr;
```

And from `astr` to `str`:
```py
some_str := a"hello" -> str;
```

But any other type of string casting is prohibited. This is because it would require heap allocations for it to properly work on runtime strings.

### Struct casting
If two struct types have the exact same data layout it is possible to cast between them:
```py
Vec3  ::= (x, y, z : f32);
Color ::= (r, g, b : f32);
pos := Vec2(1.0, 2.0, 3.0);
col := pos -> Color; # valid
```

### Function casting
If two functions have the same fundamental structure (same number of arguments, each with the same type and same return type) but only the parameter names differ casting is possible:
```py
square0 ::= (x : i32) i32 => x*x
square1 :: (a : i32) i32 = square0 -> (a : i32) i32; # valid
sum :: (x, y : i32) i32 = square0 -> (x, y : i32) i32; # error: functions differ in layout
```

The same thing works for function pointers:
```py
square0 := &(x : i32) i32 => x*x
square1 : *(a : i32) i32 = square0 -> *(a : i32) i32; # valid
```

Because function pointers are pointers, the casting rules for them are the same as the pointers one:
```py
square0 := &(x : i32) i32 => x*x
square1 : *(a, b : i32) i32 = square0 -> *(a, b : i32) i32; # valid
```

### Auto casting
Sometimes you don't want to explicitly cast to a type. Use the `!` operator as an automatic `-> T` cast to the expected type:
```py
sum ::= (a, b : i32) i32 => a+b;
x = sum(12, 1.6!); # equivalent to 'sum(12, 1.6 -> i32)'
y : u64 = 1.6!;
```

`!` operator is not valid for pointers:
```py
foo ::= (ptr = *i32) => ...;
x := 10;
p : *u8 = &y;
foo(p!); # invalid
```

Auto casting between structs with same layout is possible, this is useful for unnamed [structs](#Structs):
```py
div ::= (a, b : i32) (bool, i32) => if b == 0 do (false, 0) else (true, a/b);
res : mut (bool, i32);
res = div(4, 2); # error: types differ
res = div(4, 2)!; # valid
```

Auto casting between functions works as well (not function pointers):
```py
sum0 ::= (x : i32) i32 => x*x
sum1 :: (a : i32) i32 = sum0!; # valid
```

### Invalid casting
Casting is not valid for [arrays](#Arrays), [slices](#Slices), [anyrt](#The-anyrt-type) and custom types (except for distinct aliases or structs).

## The meta type
A `meta` is a builtin [alt](#Alts) type that represents a node from the Stark AST.

It is a [compile-time-only type](#Compile-time-only-primitive-types) and it follows the same rules as the primitive ones.

To see an in-depth list of all tags held by a `meta` see the [types of metas](#Types-of-metas) section.

### Meta literals
To create a `meta` from code use the builtin `code` function, this function accepts strings, but they really shine if [raw string literals](#Raw-string-literals) are used:
```py
some_code ::= @code $code_end
  add ::= (T : imp type is number, a, b = T) T => a+b;
  sub ::= (T : imp type is number, a, b = T) T => a-b;
  mul ::= (T : imp type is number, a, b = T) T => a*b;
  div ::= (T : imp type is number, a, b = T) T => a/b;
code_end;
```

Because of the way raw string literals works, they can be nested:
```py
some_code ::= @code $code_end
  some_code_inside_some_code ::= @code $code2_end
    square_from_the_code_inside_the_code ::= (T :: imp type is number, x : T) T => x*x;
  code2_end
code_end;
```

The tag of the generated `meta` is `meta.root`.

To put all this meta-code inside your actual source code is very simple: use the `@` operator:
```py
some_code ::= @code $code_end TEN ::= 10; code_end;
@some_code; # now 'TEN' is defined
```

What that did was put whatever was in the `some_code` AST into the main code AST at that spot.

The raw string literals constant bindings gives you macro behaviour:
```py
TEN ::= 10;
define_ten2 ::= @code $(TEN) code_end TEN2 ::= $TEN$; code_end;
@define_ten2; # TEN2 is now defined with the value 10 on it
```

### meta.iden and meta.expr
The `meta.iden` tag from `meta` can accept an identifier directly:
```py
an_actual_identifier ::= meta.iden some_random_identifier;
```

Then it can be directly injected into code:
```py
an_actual_identifier ::= meta.iden TWENTY;
@an_actual_identifier ::= 20; # 'TWENTY' is defined here with the value of 20
```

The `meta.expr`, similarly to `meta.iden`, can accept expressions directly on it's definition:
```py
an_actual_expression ::= meta.expr 3 + 4;
SEVEN ::= @an_actual_expression; # 'SEVEN' is equal to the expression '3 + 4'
```

### Metaprogramming with metas
You can return metas from ensured-compile-time functions:
```py
return_code ::= @() meta => @code $end
  square ::= (T :: type is number, a : T) T => a*a;
end;
```

Using the `@` operator on a function that returns a `meta` will not only call the function, but also inject the `meta` AST into the main AST:
```py
gen_square_fn ::= @() meta => @code $end
  square ::= (T :: type is number, a : T) T => a*a;
end;
@gen_square_fn(); # now the square function is defined
```

The `meta.iden` and `meta.expr` also can be passed as constant arguments, this is useful for putting their values directly on the code function:
```py
gen_fn ::= @(fn_name :: meta.iden, fn_body : meta.expr) meta => @code $(*)end
  $fn_name$ ::= (a : T) T => $fn_expr$;
end;
@gen_fn(square, a * a);
```

### Types of metas
This is the meta definition:
```py
meta ::= (
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
NOTE: here
`anyrt` is another builtin [alt](#Alts). It stands for 'any runtime', and as it implies, you can pass any value to it at runtime and it'll hold type information plus an immutable pointer to the data:
```py
a := 10;
b := "hello";
x : anyrt = a; # valid
y : anyrt = b; # valid
```

You can pass values directly, the data for storing the value will be allocated on the current stack frame:
```py
x : anyrt = 10; # valid
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
`anyrt` is actually specific to every Stark project. It already holds many tags by default, but every time a custom type is used as the value of an `anyrt` variable, the type will be added to the `anyrt` alt, or a sub-alt of `anyrt`, as a tag.

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

[Struct](#Structs), [alt](#Alts), [union](#Unions), [tuple](#Tuples), [array](#Arrays), [slice](#Slices), [pointer](#Pointers) and [view](#Views) tags are actually their own alts.

Every tag from the `struct` tag of `anyrt` follow this layout:
```py
(
  data = *``,
  name = str,
  members = [..](
    name = str,
    info = anyrt,
  ),
);
```

Every tag from the `alt` tag of `anyrt` follow this layout:
```py
(
  data = *``,
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
  data = *``,
  info = *anyrt,
  length = usize,
);
```

Every tag from the `slice` tag of `anyrt` follow this layout:
```py
(
  data = *``,
  info = *anyrt,
  length = usize,
);
```

Every tag from the `pointer` tag of `anyrt` follow this layout:
```py
(
  data = **``,
);
```

Every tag from the `view` tag of `anyrt` follow this layout:
```py
(
  data = **``,
);
```

The `data` field of every tag will obviously be a pointer to the correct type instead of [the nothing type](#Nothing-pointer). For the `array` and `slice` tags it'll be a slice to the corret type instead of a pointer. And for the `view` tag it'll be views instead of pointers.

Actually the above struct layouts are also tags of `anyrt`, they're called `struct_generic`, `alt_generic`, `tuple_generic`, `array_generic`, `slice_generic`, `pointer_generic` and `view_generic`. These tags are used for when you want to account to any type of struct, alt or tuple.

Distinct aliases are added to `anyrt` directly.

The tag name of any named type will be the type name itself.

There is a `unnamed` tag, this tag is an alt. This alt is basically an `anyrt` in itself, every sub-alt of `anyrt` has an equivalent inside `unnamed`, every unnamed custom type goes inside `unnamed`, the name of each tag will be `t` +  an arbitrary id.

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
- Alts are comparable with their tags

## Control flow (if, else, match and loops)
### If and else
As everything else in the language, `if`s are expressions. To make an if expression use the `if` keyword, followed by a boolean condition, then the `do` keyword and finally an expression:
```py
if <condition> do <expr>;
```

After the `if` expression you can optionally have an `else` expression. The else only receives an expression, not a condition:
```py
if x == 10 do something() else fallback();
```

`if` expressions with no `else` can only return expression of [the nothing type](#The-nothing-type):
```py
if something do (); # valid
_ := if something do 1; # error: an 'if' that returns a value must be followed by 'else'
```

The expression returned by the `if` and the `else` have to be of the same type:
```py
x := if something do 1 else 2.0; # error: value returned by 'else' is not of the same value returned by 'if'
```

And of course, if an `if` expression returns something it needs to be handled:
```py
if x == 10 do 10 else 9; # error: unhandled expression
```

It's possible to chain an `if` expression:
```py
if something do
  this()
else if something_else do
  that()
else
  other();
```

This isn't some special case, its just an `if`/`else` with the expression inside the `else` being another `if`/`else`:
```py
if something do
  this()
else
  if something_else do
    that()
  else
    other();
```

#### Assigning on if
As it's known, [variable assignments](#Variable-assignment) wrapped around parenthesis return the variable value after it's assignment. This becomes really useful on if expressions:
```py
x : mut i32;
if |x = get_x()| == 10 do do_stuff();
```

#### Definition on if
It's very commom to do an operation put the result in a variable and only use said variable if it's in certain coditions. For those cases you can actually make a definition inside an `if`, the variable will only be visible to the `if`/`else` chain. The assignment rule is still applicable, so pipe wrapping is needed:
```py
if |x := get_x()| == 10 && x < 10 do this(x)
else if x == 9                    do that(x);

if |a := is_this_true()| do this(a)
else                     do that(a);
```

Shadowing in `if`/`else`, [like in the rest of the language](#Variable-shadowing), isn't allowed:
```py
if      |x := get_this()| == 10 do this(x)
else if |x := get_that()| == 9  do that(x); # error: 'x' is already defined
```

Simply use a mutable variable if this is needed:
```py
if      |x := mut get_this()| == 10 do this(x)
else if |x  = get_that()    | == 9  do that(x); # valid
```

### Match expression
TODO: from switch to match. a match expression is a way to use function guards on variables
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

#### Switch on alts
A switch can accept an alt because them can be compared with tags. The difference with a `switch` on alts is that all tags must be handled:
```py
def Color : (Red|Green|Blue);
def col = Color;
switch col (
  Red => do_red(),
  Green => do_green(),
); # error: unhandled 'Blue' tag on an alt switch
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
switch def |x = get_i32()| (
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
while |i := 0| < 10 => i++;
```

The definition and assignment will only occur once, on next iterations only the current value of `i` will be used.

You can use the `nxt` expression to go to the next iteration immediately: 
```py
while |i := 0| < 10 => {
  i++;
  if i % 2 == 0 do nxt;
  do_something();
}
```

What the `nxt` expression does is it's just jumps to the end of the current expression-block. So it can only be used on loops and on expression blocks:
```py
while something => nxt; # error: 'nxt' outside of expression-block
```

Because `nxt` just jumps to the end of the expression block, all deferred expressions will be ran. So the correct way to do a C for loop is:
```py
while |i := 0| < 10 => {
  defer i++;
  if i % 2 == 0 do nxt;
  do_something();
}
```

`nxt` accepts an expression that will be ran at the next loop iteration:
```py
while def |i = 0| < 10 => {
  defer i++;
  if i % 2 == 0 do nxt i++; # i will incremeant by 1 in the next iteration
  do_something();
}
```

Because `nxt` is an expression it can be nested:
```py
while def |i = 0| < 10 => {
  defer i++;
  if i % 2 == 0 do nxt nxt; # skips next iteration
  do_something();
}
```

### For loop
`for` loops are loops that iterate over an [array](#Arrays), a [slice](#Slices), a [range](#Ranges) or an [iterator](#Iterators).

Each iteration of a `for` loop gives you a value.

For ranges this value is an integer:
```py
for i in 0..9 => io.slow_println("%", [i]);
```

By default is an i32, i64 or u64 depending on the values on the range. But you can specify exactly the type of integer that you want:
```py
for i : i8 in 0..9 => io._slow_println("%", [i]);
```

As any variable, the iteration variable is immutable by default, but you can mark it as mutable:
```py
for i : mut in 0..9 => {
  i *= 2;
  io.slow_println("%", [i]);
}
```

Mixing `mut` and the specified type is possible:
```py
for i : mut i8 in 0..9 => {
  i *= 2;
  io.slow_println("%", [i]);
}
```

You can mark the range as `rev` for reverse iteration:
```py
for i in rev 0..9 => io.slow_println("%", [i]);
```

For arrays and slices the value is a [tuple](#Tuples) of the current iteration index + an element from the array/slice. This element behavies like a value, but it can actually be a reference. It's similar to how [function arguments](#Immutable-arguments-are-references) works:
```py
nums := [1, 2, 3];
for (i, n) in nums => io.slow_println("nums[%] = %", [i, n]);
```

It's also possible to mark the [tuple](#Tuples) as mutable, if that's the case the element will be a pointer:
```py
nums := mut [1, 2, 3];
for (i, n) : mut in nums => {
  *n *= 2;
  io.slow_println("nums[%] = %", [i, *n]);
};
```

You obviously only can do that on mutable arrays/slices:
```py
nums := [1, 2, 3];
for (i, n) : mut in nums => ...; # error: 'nums' is immutable
```

When an immutable pointer to an element is needed use `= *`:
```py
nums := mut [1, 2, 3];
for (i, n) : * in nums => io.slow_println("&nums[%] = %", [i, n]);
```

You can mark the array/slice as `rev` for reverse iteration:
```py
nums := [1, 2, 3];
for (i, n) in rev nums => io.slow_println("nums[%] = %", [i, n]);
```

`soa` arrays and slices can't be iterated through. Do this instead:
```py
Vec2 ::= (x, y : f32);
pos := soa Vec2[(1.0, 2.0), (3.0, 4.0), (5.0, 6.0)];
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
for (i, n) in nums => io.slow_println("nums[%] = %", [i, *n]);
for (i, n) in rev nums => io.slow_println("nums[%] = %", [i, *n]);
def nums_mut = mut Linked_List.make[1, 2, 3];
for (i, n) = mut in nums_mut => {
  *n = 2;
  io.slow_println("nums[%] = %", [i, *n]);
};
```

### Compile-time control flows
Any control flow expression can be ran at compile-time, just prefix it with `@` and ensure to only use compile-time known values in it:
```py
@if SOME_CONST do ...
else if SOME_OTHER do ...
else ...;

@switch SOME_NEW_CONST (
  case0 => ...;
  case1 => ...;
  or    => ...;
);

@while SOME_CONST => ...;

@for i in 0..9 => ...;
```

And all of them can be ran in [module-scope](#Scoping).

All compile-time loops are unrolled. And all false `if`/`else`/`switch` conditions aren't included on the final code. They're still checked for errors though.

## Error handling
Functions can return an error alt instead of the specified return type using `||`:
```py
def DivErr : (By_Zero|);
def div : (a = i32, b = i32) i32 || DivErr => if b == 0 do .By_Zero else a/b;
```

This will implicitly create a temporary alt for usage on error handling.

If both the normal return value and the error return value are alts you need to specify the alt name on the tag. This avoids ambiguity:
```py
def Color : (Red|Green|Blue|Invalid);
def Error : (Invalid);
def colorize : (r, g, b = bool) Color || Error =>
  if      r && (g || b) do Error.Invalid
  else if g && (r || b) do Error.Invalid
  else if b && (r || g) do Error.Invalid
  else                  if r do Color.Red else if g do Color.Green else if b do Color.Blue else Color.Invalid;
```

Otherwise it's safe to just use the `.<tag>` shortcut:
```py
def Error : (Invalid|None);
def colorize : (r, g, b = bool) u32 || Error =>
  if      r && (g || b) do .Invalid
  else if g && (r || b) do .Invalid
  else if b && (r || g) do .Invalid
  else                     if r do 0xff0000 else if g do 0x00ff00 else if b do 0x0000ff else .None;
```

If that's the case you can't just call the function normally, you have to handle the error.

### The 'or' operator
The `or` operator is the way to handle errors returned by functions. If no errors occur the expected value is returned from the expression, if errors do occur a the value is returned from the right-hand side of the `or` operator:
```py
def res = div(10, 0) or 0;
```

The value returned from the right-hand side has to have the same type of the left-hand side or [the nothing type](#The-nothing-type). If the right-hand side returns the nothing type while the other side doesn't the program crashes:
```py
def res = div(10, 0) or this_returns_void(); # will crash
def res = div(10, 0) or 0.0; # compile-time error
```

If the `or` rhs is empty the program will crash if it reaches it as well:
```py
def foo : (error = bool) || Some_Error => if error do Some_Error.Occurred;
foo(false) or; # fine
foo(true) or this_returns_nothing(); # will not crash, both sides return the nothing type
foo(true) or; # will crash, rhs of 'or' is empty
```

You can think of this as an implicit exit [syscall](https://en.wikipedia.org/wiki/System_call) being added automatically. Because that's exactly what's happening.

If you use `ret` or `brk` you actually avoid the exit call, no crash occur, but the expressions will still work like usual:
```py
{
  foo(true) or brk;
  io.slow_println("never printed");
}
io.slow_println("no crash!");
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
res = div(10, 0) or(e) if e == .ByZero do io.slow_println("division by zero"); # will crash
```

`or` is an operator, and as such chaining is possible:
```py
this() or that() or(e) io.slow_println("error: %", [e]);
```

### Options
An option is just a type that can have the value `null` instead of a valid value. Internally the type just gains a flag value telling if it's null or not. To create an option add `?` as a prefix to the type:
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
if x != null do y = x; # valid
else => y = 0;
```

You can use the `or` operator on options. This will make the check automatic:
```py
def x = ?i32 maybe_number();
def y = x or 0;
```

The same `or` rules established earlier applies here. `or;` will crash, mismatched type will cause an error and the nothing type return will crash:
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
| 3          | ^        | Get alt value (suffix)                     | Left to Right |
| 3          | ^        | Get alt tag number (prefix)                | Right to Left |
| 3          | *        | Pointer dereferencing                      | Right to Left |
| 3          | &        | Address of                                 | Right to Left |
| 3          | &mut     | Mutable address of                         | Right to Left |
| 3          | ->       | Safe cast between types                    | Left to Right |
| 3          | !        | Safe automatic cast between types          | Left to Right |
| 4          | *        | Multiplication                             | Left to Right |
| 4          | /        | Division                                   | Left to Right |
| 4          | %        | Modulo                                     | Left to Right |
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
| 15         | :        | Constant assignment                        | Right to Left |
| 15         | =        | Variable assignment                        | Right to Left |
| 15         | +=       | Variable assignment by sum                 | Right to Left |
| 15         | -=       | Variable assignment by difference          | Right to Left |
| 15         | *=       | Variable assignment by product             | Right to Left |
| 15         | /=       | Variable assignment by quotient            | Right to Left |
| 15         | %=       | Variable assignment by modulo              | Right to Left |
| 15         | <<=      | Variable assignment by bitwise left shift  | Right to Left |
| 15         | >>=      | Variable assignment by bitwise right shift | Right to Left |
| 15         | &=       | Variable assignment by bitwise and         | Right to Left |
| 15         | \|=      | Variable assignment by bitwise or          | Right to Left |
| 15         | ^=       | Variable assignment by bitwise xor         | Right to Left |
| 16         | ,        | Comma                                      | Right to Left |
| 16         | |        | Union/Alt pipe                             | Right to Left |

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
some_overload ::= [a = _, b = u32, c = a](a, b) c||+;
```

The type pattern represents a type, it can also be a pointer, slice, view or array (the array length needs to be a constant integer or `_`):
```py
some_overload ::= [
  a = *_,
  b = **_,
  c = &*_,
  d = *mut _,
  e = [3]_,
  f = [_]_,
  g = [..]_,
  h = [..]mut _,
  i = [*]_,
  j = [*]mut _
](a, b, c, d, e, f, g, h, i) j||+;
```
This isn't all the possible combinations, because the possible combinations are infinite, like on variables where you can have a pointer to a slice of arrays of length 3 of mutable views to some type.

This isn't exclusive to the `_` identifier. These type attributes can be used on previously defined type patterns or real external types:
```py
some_overload ::= [
  a = *u32,
  b = **a,
  c = &*b,
  d = *mut b,
  e = [3]i32,
  f = [_]d,
  g = [..]f,
  h = [..]mut f32,
  i = [*]number,
  j = [*]mut h
](a, b, c, d, e, f, g, h, i) j;
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
- `_`: any type can be placed there, excluding [the nothing type](#The-nothing-type)
- `+`: any type can be placed there, including the nothing type
- Empty: only the nothing type can be placed there

The return pattern have two values, the return type and the error type. They're separated by `||`, on the left is the return type and on the right is the error type; like in real functions. The error type can only accept type patterns that have `_` or an alt as it's value:
```py
def some_overload0 : []() +||+;
def some_overload1 : []()  || ;
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
def sum : [a = number](a*) a|| (sum2_i32, sum3_i32, sum2_f32, sum3_f32);
```

And because functions are first-class citizens you can pass function literals to `ovl(<funcs>)` directly:
```py
def sum : [a = number](a*) a|| (
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
def sum : [a = _](a*) a||;
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
def sum : [a = _](a*) a||;
sum <- (T : imp type is number, a, b = T) T => a + b;
sum <- (T : type is number, a, b, c = T) T => a + b + c; # error: overloads only accepts functions with constant arguments marked as 'imp'
```

Keep in mind that the more specific overload beats the more generic:
```py
def operation : [a = _](a*) a||;
operation <- (a, b = i32) i32 => a - b;
operation <- (T : imp type is number, a, b = T) T => a + b;
def a = 1.0 operation 2.0; # a == 3.0
def b = 1 operation 2; # a == -1
```

### Operator overload
There are a set of builtin overloads that automatically bounds to some operators. These are it's definitions:
```py
__add__       ::= [a = _](a, a) a||+; # plus '+'
__sub__       ::= [a = _](a, a) a||+; # minus '-'
__mul__       ::= [a = _](a, a) a||+; # multiplication '*'
__div__       ::= [a = _](a, a) a||+; # division '/'
__mod__       ::= [a = _](a, a) a||+; # modulo '%'
__addeq__     ::= [a = _, b = &*_](b, a) ||+; # plus equals '+=', returns nothing
__subeq__     ::= [a = _, b = &*_](b, a) ||+; # minus equals '-=', returns nothing
__muleq__     ::= [a = _, b = &*_](b, a) ||+; # multiplication equals '*=', returns nothing
__diveq__     ::= [a = _, b = &*_](b, a) ||+; # division equals '/=', returns nothing
__modeq__     ::= [a = _, b = &*_](b, a) ||+; # modulo equals '%=', returns nothing
__addeqe__    ::= [a = _, b = &*_](b, a) a||+; # plus equals '+=', returns expression value
__subeqe__    ::= [a = _, b = &*_](b, a) a||+; # minus equals '-=', returns expression value
__muleqe__    ::= [a = _, b = &*_](b, a) a||+; # multiplication equals '*=', returns expression value
__diveqe__    ::= [a = _, b = &*_](b, a) a||+; # division equals '/=', returns expression value
__modeqe__    ::= [a = _, b = &*_](b, a) a||+; # modulo equals '%=', returns expression value
__incp__      ::= [a = &*mut _](a) ||+; # prefix increment '++', returns nothing
__decp__      ::= [a = &*mut _](a) ||+; # prefix decrement '--', returns nothing
__incs__      ::= [a = &*mut _](a) ||+; # suffix increment '++', returns nothing
__decs__      ::= [a = &*mut _](a) ||+; # suffix decrement '--', returns nothing
__incpe__     ::= [a = &*mut _](a) a||+; # prefix increment '++', returns expression value
__decpe__     ::= [a = &*mut _](a) a||+; # prefix decrement '--', returns expression value
__incse__     ::= [a = &*mut _](a) a||+; # suffix increment '++', returns expression value
__decse__     ::= [a = &*mut _](a) a||+; # suffix decrement '--', returns expression value
__eqto__      ::= [a = _, b = bool](a, a) b||+; # equal to '=='
__greater__   ::= [a = _, b = bool](a, a) b||+; # greater than '>'
__less__      ::= [a = _, b = bool](a, a) b||+; # less than '<'
__greatereq__ ::= [a = _, b = bool](a, a) b||+; # greater than or equal to '>='
__lesseq__    ::= [a = _, b = bool](a, a) b||+; # less than or equal to '<='
__indg__      ::= [a = _, b = _, c = &*_](c, b) a||+; # index get []
__inds__      ::= [a = &*mut _, b = _](a, b) a||+; # index set []
```

The operators that can be overloaded are limited to mathematical operations or data structure indexing. Do not abuse this feature.

## Classes
Classes aren't types like in other programming languages. Classes are just a compile-time group for types. They doesn't have any specific purpose and can be used to do whatever you want with them. But the main use is type filtering in generics.

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
Integers <- usize+isize; # 'Numbers' still doesn't have usize and isize
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
The only purpose of the `is` operator is to identify if a type is part of a class. What it does depends on where it's used. But mainly it'll return `true` if the type is part of the class and `false` if it isn't:
```py
Num32 ::= i32+u32+f32;
a := u32 is Num32; # a == true
b := u64 is Num32; # b == false
```

And you obviously can use `is` on anonymous classes:
```py
Num32 ::= i32+u32+f32;
Num64 ::= i64+u64+f64;
a := u32 is Num32+Num64; # a == true
b := u64 is Num32+Num64; # b == true
c := u16 is Num32+Num64; # b == false
```

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
- `customtype`: All custom types (structs, alts, etc)
- `struct`: All structs
- `alt`: All alts
- `any`: All types

Some of them are updated when custom types are added. Like `struct`, `alt`, `any`, etc.

## Spaces
Spaces are a subset of the current [module](#Modules) into one common constant. Everything that's valid on the outside module is valid inside of a space. To define a space simply use triangular brackets, everything inside it is part of the space:
```py
def some_space : <
  def sum : (x, y = i32) i32 => x+y;
  def sub : (x, y = i32) i32 => x-y;
>;
```

To access a symbol from a space use the `<space>.<symbol>` syntax:
```py
some_space : <
  def sum : (x, y = i32) i32 => x+y;
  def sub : (x, y = i32) i32 => x-y;
>;
def _ = some_space.sum(1, 2);
```

### Space privacy
Everything inside of a space is public by default, so the `pub` attribute doesn't really do anything. But you can mark them as `prv` to be only accessable inside the space itself:
```py
def some_space : <
  prv def mul : (x, y = f32) f32 => x+y;
  pub def dot : (x0,y0, x1,y1 = f32) f32 => x0 mul x1 + y0 mul y1; # 'pub' isn't really needed
>;
def _ = some_space.dot(1.0, 2.0, 3.0, 4.0);
def _ = some_space.mul(1.0, 2.0); # error: 'mul' is private
```

### Space type
All spaces have the type `spc`, so you can define them explicitly:
```py
def some_space : spc <
  def foo : () =>;
  def bar : () =>;
>;
```

### The 'use' overload
There's a bultin overload called `use`, it has two functions by default in it:
```py
def use : [a = spc, b = [..]meta.iden, c = meta](a, b+) c|| (
  @(space = spc) meta =>...,
  @(space = spc, symbols = [..]meta.iden) meta =>...,
);
```

The function that just takes a space will make binds for all the spaces symbols:
```py
def some_space : <
  def add : (x, y = i32) i32 => x+y;
  def sub : (x, y = i32) i32 => x-y;
  def mul : (x, y = i32) i32 => x*y;
  def div : (x, y = i32) i32 => x/y;
>;
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
def some_space : <
  def add : (x, y = i32) i32 => x+y;
  def sub : (x, y = i32) i32 => x-y;
  def mul : (x, y = i32) i32 => x*y;
  def div : (x, y = i32) i32 => x/y;
>;
@use(some_space, [add, div]);
```

`@use(some_space, [add, div])` does actually this:
```py
def add : some_space.add;
def div : some_space.div;
```

If `@use` finds a conflict between the binding is trying to make and an already defined symbol it asserts:
```py
def some_space : <
  def add : (x, y = i32) i32 => x+y;
  def sub : (x, y = i32) i32 => x-y;
  def mul : (x, y = i32) i32 => x*y;
  def div : (x, y = i32) i32 => x/y;
>;
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
def mod : <
  def add : ext mod.add;
  def sub : ext mod.sub;
  def mul : ext mod.mul;
  def div : ext mod.div;
>;
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
def math : <
  def add : (x, y = i32) i32 => x+y;
  def sub : (x, y = i32) i32 => x-y;
  def mul : (x, y = i32) i32 => x*y;
  def div : (x, y = i32) i32 => x/y;
>;
```

The reason why `.sks` is used for the extension of those separate space files is to avoid confusion. There's one key difference between a space and a module and that is their default privace: All modules have their symbols as private by default and all spaces have their symbols as public by default.

The search path for those `.sks` files is the directory in which the module that they are being included to is located.

### Configuring external modules
To configure settings from the standard bundle you must create a space file with the name `std.sks`. This will be included on the standard bundle modules. In there you can configure it as you like:
```py
# std.sks
TAPE_DEFAULT_CAPACITY : 1 << 20;
TARGET : "Linux:x86_64";
```

This is just a convention followed by the standard bundle. But it's good practice to follow it with your own bundles.

## Foreign function interface
You can get a symbol from a foreign dynamic or static library using the `unk` keyword, followed by a string with the name of said library and another string representing the name of said symbol:
```py
unk "libm" "sqrt"
```

The extension `.so`, `.dll`, etc; must not be included with the library name.

This will return a value with the type `unkfn`, that value can be assigned to a constant function of any type:
```py
sqrt :: (x : f64) f64 = unk "libm" "sqrt";
```

The linker will automaticaly link to every library specified by every `unk`.

The linking path will be specified by the linker. But you can also create a directory named `unk`, that path will be automatically added by the linker. 

## Scoping
There are two types of scopes in stark source code module-scope and block-scope.

Module-scope is at the file level:
```py
CONSTANT_AT_MODULE_SCOPE ::= 10;
variable_at_module_scope := 10;
main ::= () =>;
```

And block-scope is at the function level:
```py
main ::= () => {
  CONSTANT_AT_BLOCK_SCOPE ::= 10;
  variable_at_block_scope := 10;
};
```

Scopes can be nested:
```py
{ } # nested module-scope using expression-block

main :: () => {
  { } # nested block-scope using expression-block
};
```

## Machine code capabilities in Stark
It's possible to embed binary data directly into stark code using the `embed` expression. This expression takes flags for the embedded data permission and a constant slice of `u8`s. It'll return a [nothing pointer](#Nothing-pointer) or a mutable nothing pointer depending on the flags: 
`data ::= embed rw [0x86, 0xff]`;

### Embed flags
These are the possible flags that `embed` accepts:
- `r`: Readable-only data
- `w`: Writable-only data
- `x`: Executable-only data. On systems that don't permit only executable data (e.g. x86) the behavior is exactly the same as `rx`
- `rw`: Readable + writable data
- `rx`: Readable + executable data
- `wx`: Writable + executable data. On systems that don't permit only executable data (e.g. x86) the behavior is exactly the same as `rwx`
- `rxx`: Readable + executable data. If used in a function the machine code will be inlined. If it isn't behaves the same as `rx`.
- `rwx`: Readable + writable + executable data

If you doesn't have the appropriate flag for what you're trying to do with that data (read/write/execute) a segmentation fault will occur.

### Executing embedded data
To execute data created using `embed` first you need to cast it to a [function pointer](#Function-pointers), then simply run the function:
```py
add ::= embed x [0x48, 0x8D, 0x04, 0x37, 0xC3] -> *(a, b : u64) u64;
x := add(3, 4); # x == 7
```

### The bultin 'asm' function
There's a function on the `std.base` module called `asm`, this function takes intel assembly source code and returns a pointer to the assembly data. It internally uses `embed` with the flag `rxx`. The valid assembly source will depend on the current platform (x86, x64, etc):
```py
add ::= @asm $end
  lea rax, [rdi+rsi]
  ret
end -> *(a, b : u64) u64;
x := add(3, 4); # x == 7
```

### Manipulating registers
You can activaly read from and write to registers for your target platform using the `reg` keyword:
```py
reg rax = 10;
def x = u64 reg rax; # x == 10
```

The inferred type of the variable will be based on the register: `rax` = `u64`, `eax` = `u32`, etc:
```py
def x = reg al; # x is of type u8
```

This can pair up well with `embed` and `asm`.

## Standard bundle
### Base module
Work in progress...

### Mem module
NOTE: arena allocator and `Tape` (non-reallocatable dynamic array with an arena, static memory or stack as it's backend)
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

