# Stark
A simple language for systems programming.

Current version: 0.1.0

## Functions
### Function definition and calling conventions
Function are defined using the following syntax:
```
def func_name : (arg0 = type0, arg1 = type1, ...) return_type => func_body_expr;
```

Functions always returns the expression at the function body.

A basic function that returns a sum of two numbers would be defined as:
```
def sum : (a = i32, b = i32) i32 => a + b;
```

To call a regular function:
```
def foo : (_a = i32, _b = i32) =>;
foo(10, 12);
```

But if it takes only one argument the parenthesis are optional:
```
def foo : (_x = i32) =>;
foo 10;
```

Functions that takes exactly two arguments can be called as an 'infix call' `x foo y`, where `x` and `y` are arguments and `foo` is the function:
```
def foo : (_a = i32, _b = i32) =>;
10 sum 12;
```

### Unused arguments
Unused parameters causes a compile-time error:
```
def sum : (a = i32, b = i32, c = i32) i32 => a + b; # error: 'c' is unused
```

To supress the error put an underscore as a prefix on the argument name:
```
def sum : (a = i32, b = i32, _c = i32) i32 => a + b; # valid
```

### Void return type
For functions with no return value use `void` as the return type:
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
Most functions arguments are variables, so they can be defined in the same way (see more on [the variables section](#Variables)):
```
def fn0 : (_x = i32) =>;
def fn1 : (_x, _y = i32, _c = f32) =>;
def fn2 : (_x = i32 1) =>; # default values have to be compile-time constants
def fn3 : (_x, _y = i32 1) =>;
def fn4 : (_x = 0) =>;
def fn5 : (_x, _y = 0, _c, _d = 0.0) =>;
```

Functions can also have constant arguments:
```
def foo : (num : u64 0) u64 => num;
```

That means that you can only pass constant values for those arguments:
```
def foo : (num : u64 0) u64 => num;
def x = foo(10); # valid
def x = foo(x); # invalid, x isn't a constant
```

Constant arguments are the only place where you don't need to specify the constant value on definition (see more on [the constants section](#Constants)):
```
def foo : (num : u64) u64 => num;
```
So the code above doesn't actually make a type aliases, instead it makes an i64 constant.

### Compile-time functions
If a function only takes constant arguments and it don't access external non-constant data, it becomes a compile-time function. I.e. it'll be run at compile-time:
```
def comp_sum : (a, b : i32) i32 => a + b;
def x = comp_sum(12, 4); # 16 will be computed at compile-time
```

It's actually possible to run any function, that don't access external non-constant data, at compile time if you pass compile-time known values as it's arguments and use the `@` operator:
```
def sum : (a, b = i32) i32 => a + b;
def x = @sum(12, 4); # 16 will be computed at compile-time
```

You generally can't run functions at file scope, compile-time functions and @ function calls are the exception:
```
def foo : () => do_something();

foo(); # invalid
@foo(); # valid

def main : () => {
}
```

### Generic functions
With constant arguments you can make generic functions using the `type` type:
```
def sum : (T : type, a, b = T) T => a + b;
```

It's also possible to use the `is` (see more on [the type families section](#Type-Families-and-'is'-operator)):
```
def sum : (T : type is number, a, b = T) T => a + b;
```

### Big sized immutable arguments
Immutable arguments that have a value bigger than 8 bytes are passed as references (implicit pointer). That way copies are not made.

So you don't need to explicitly pass immutable pointers around for big structs, like you would do it in C. It is actually more performant if you don't use immutable pointers to just pass values because inlining becomes a possibility:
```
def bar(x = *Big_Struct) Something => x.something;
def foo(x = Big_Struct) Something => x.something;
big = Big_Struct;
bar(&big)
foo(big) # performance for this is the same as bar or better if inlined
```

### Functions are just values
Functions are just values assigned to [constants](Constants). So you can call an anonymous function directly:
```
def num = ((x, y = i32) i32 => x + y;)(10, 20); # num is assigned to 30
```

Because of this, nested functions are simple:
```
def foo : () => {
  def bar : (x, y = i32) i32 => x + y;
}
```

This does not compromise recursion for functions defined as constants.

### Function pointers
Functions can even be assigned directly to variables. Because of this they'll become function pointers:
```
def var = (x = i32) i32 => x*x;
```

You can't use recursion on functions assigned directly to variables though:
```
def var = (x = i32) i32 => var(x)*x; # error: var is undefined
```

The type of a function pointer looks like this:
```
fn(type0, type1, ...) return_type
```

Function pointers can't have constant arguments.

## Constants

## Variables

## Structs
Constant members without a value have to be at the top
```
def IntList = struct(
  T : type is integer,
  buffer = *T,
  capacity, length = u32
);
def numbers = IntList(u32);
```

## Control flow (if, else and loops)
### Compile-time control flows

## Operators precedence
| Precedence | Operator    | Description                                | Associativity |
|------------|-------------|--------------------------------------------|---------------|
| 0          | @func(x, y) | Compile-time function call                 | Left to Right |
| 1          | func(x, y)  | Regular function call                      | Left to Right |
| 1          | func x      | One parameter function call                | Left to Right |
| 1          | x func y    | Infix function call                        | Left to Right |
| 1          | ++          | Sufix incremeant                           | Left to Right |
| 1          | --          | Sufix Decremeant                           | Left to Right |
| 1          | []          | Indexing                                   | Left to Right |
| 1          | .           | Member access                              | Left to Right |
| 1          | Struct()    | Struct literal                             | Left to Right |
| 2          | ++          | Prefix incremeant                          | Right to Left |
| 2          | --          | Sufix incremeant                           | Right to Left |
| 2          | +           | Unary plus                                 | Right to Left |
| 2          | -           | Unary minus                                | Right to Left |
| 2          | !           | Logical not                                | Right to Left |
| 2          | $           | Bitwise not                                | Right to Left |
| 2          | *           | Pointer derreferencing                     | Right to Left |
| 2          | &           | Address of                                 | Right to Left |
| 2          | &mut        | Mutable address of                         | Right to Left |
| 2          | sizeof      | Size of type                               | Right to Left |
| 2          | alignof     | Alignment of type                          | Right to Left |
| 2          | ->          | Safe cast between types                    | Left to Right |
| 2          | trans       | Transmute pointer                          | Left to Right |
| 3          | ^           | Power                                      | Left to Right |
| 4          | *           | Multiplication                             | Left to Right |
| 4          | /           | Division                                   | Left to Right |
| 4          | %           | Modular                                    | Left to Right |
| 5          | +           | Addition                                   | Left to Right |
| 5          | -           | Subtraction                                | Left to Right |
| 6          | <<          | Bitwise shift left                         | Left to Right |
| 6          | >>          | Bitwise shift right                        | Left to Right |
| 7          | >           | Greater than                               | Left to Right |
| 7          | <           | Less than                                  | Left to Right |
| 7          | >=          | Greater or equals than                     | Left to Right |
| 7          | <=          | Less or equals than                        | Left to Right |
| 8          | ==          | Equals                                     | Left to Right |
| 8          | !=          | Not equal                                  | Left to Right |
| 9          | &           | Bitwise and                                | Left to Right |
| 10         | \|          | Bitwise or                                 | Left to Right |
| 11         | ~           | Bitwise xor                                | Left to Right |
| 12         | &&          | Logical and                                | Left to Right |
| 13         | \|\|        | Logical or                                 | Left to Right |
| 14         | ?:          | Ternary conditional                        | Right to Left |
| 15         | :           | Constant assignment                        | Right to Left |
| 15         | =           | Variable assignment                        | Right to Left |
| 15         | +=          | Variable assignment by sum                 | Right to Left |
| 15         | -=          | Variable assignment by difference          | Right to Left |
| 15         | *=          | Variable assignment by product             | Right to Left |
| 15         | /=          | Variable assignment by quotient            | Right to Left |
| 15         | %=          | Variable assignment by modular             | Right to Left |
| 15         | <<=         | Variable assignment by bitwise left shift  | Right to Left |
| 15         | >>=         | Variable assignment by bitwise right shift | Right to Left |
| 15         | &=          | Variable assignment by bitwise and         | Right to Left |
| 15         | \|=         | Variable assignment by bitwise or          | Right to Left |
| 15         | ~=          | Variable assignment by bitwise xor         | Right to Left |
| 16         | ,           | Comma                                      | Right to Left |

