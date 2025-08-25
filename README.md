# Stark
Current version: 0.0.2

Stark is a systems programming language with the focus on maximum customizability. With Stark you don't need to give up on features of other languages, you can implement those features in Stark itself.

## The kernel
The kernel is the actual core language shipped with the compiler. It is __immutable__, meaning that it'll _never_ be updated.

### Expressions
The kernel is purely expression-based. Meaning that every single thing in it is considered an expression.

If an expression is not part of a bigger expression it must be ended with a semicolon.

Expressions are typed and composable, they _may_ receive inputs and _always_ produce outputs.

### Comments
Comments starts with `//` and go until the end of the line:
```
// This is a comment!
```

Use `/* */` for multiline comments:
```
/*
This
is
a
comment!
*/
```

### Symbols
In the kernel there are only two types of user defined _things_:
1. Variables
2. Constants

These two things are called __symbols__.

Variables are values that exists at _runtime_ and have memory. To define a variable use the following syntax:
```
name : type;
```

Constants are _immutable_ values that exists only at _compile-time_ and don't have memory. To define a constant use the following syntax:
```
NAME :: type;
```

Both of the symbol definition expressions are of the type [`noret`](#No-return).

Symbol names must follow the [identifier](#Identifier) rules, because they are identifiers.

Use the `=` operator to assign values to both constants and variables:
```
var : i32;
var = 10;

CONST :: i32;
CONST = 20;
```

It is not possible to redefine symbols:
```
var : i32;
const :: i32;
var : i32;    // error: variable redefinition
const :: i32; // error: constant redefinition
```

It's also possible to use `=` right after a symbol definition:
```
var : i32 = 10;
const :: i32 = 20;
```

A variable can be assigned multiple times:
```
var : i32;
var = 10;
var = 20;
```

A constant can only be assigned _once_:
```
const :: i32;
const = 10;
const = 20; // error: constant reassignment
```

Because constants are compile-time only, it's only possible to assign compile-time known values to it:
```
var : i32 = 10;
const :: i32 = var; // error: assigning non-compile-time value to constant
```

A variable must be assigned to a value at least once before being possible to read from it:
```
a : i32;
b : i32 = a; // error: 'a' is uninitalized
```

This is bypassed if you passed it's [address](#Pointers) to a [function](#Functions):
```
a : i32;
init_a(&a);
b : i32 = a; // fine
```

All constants must always be assigned:
```
const :: i32; // error: 'const' is never given a value
// end of file
```

The assignment of constants must happen on the same scope as the constant definition:
```
const :: i32;
{
  const = 10; // error: assigning to constant 'const' in a different scope from it's definition
};
```

The `_` symbol as a special one. It is possible to define it multiple times and you're able to assign it to anything:
```
_ : i32;
_ : f32;
_ : [10]u8;
_ = 1;
_ = 2.0;
_ = "three";
```

This symbol is used for discarting expression results when not needed and does _not have memory_. It is not possible to read from it in any way:
```
_ = 10;
a : i32 = _;     // error: trying to read from symbol '_'
b : *i32 = &_;   // error: trying to read from symbol '_'
c : i32 = _ + 1; // error: trying to read from symbol '_'
d : i32 = _[0];  // error: trying to read from symbol '_'
```

All defined symbols, that do not start with an underscore, _must_ be read from at least once through out the program:
```
foo : i32 = 10; // error: 'foo' is never used
_foo : i32 = 10; // fine
// end of file
```

### Constant expressions
Constant expressions are expressions that use only constants and literals and operations between them. [Function](#Functions) calling is not considered a constant expression.

### Blocks
A block is a way to evaluted multiple separate expressions in sequence. To start a block use `{` and close the block with `}`:
```
{
  foo : i32 = 10;
  bar : i32 = foo*foo;
};
```

_All_ expressions inside a nested block have to return either [`void`](#Void) or [`noret`](#No-return):
```
{
  var : i32; // fine, expression returns 'noret'
  var; // error: expression inside block returns 'i32'
};
```

Blocks are also just expressions, because of this nested blocks:
```
{
  foo : i32 = 10;
  {
    nested_foo : i32 = 20;
  };
};
```

Blocks have their own scope. Because of this block-defined symbols are only visible within the block itself and nested blocks:
```
{
  foo : i32 = 10;
  {
    bar : i32 = foo*foo; // fine
  };
  baz : i32 = bar*bar; // error: undefined symbol 'bar'
};
```

Blocks are expressions that by default return [`void`](#Void), but it's possible to specify a returning of a block using the `ret` operator:
```
_ = {
  ret 10;
}; // returns 10
```

The `ret` operator must be used inside a block: 
```
ret 0; // error: 'ret' outside of block
```

And it must receive exactly one expression of any type, except [`noret`](#No-return):
```
{
  ret; // error: missing expression after 'ret'
};
```

To use `ret` just as an early return of a block use an empty block as it's expression:
```
foo : i32 = 10;
{
  foo++;
  ret {};
  foo++; // not executed
};
bar : i32 = foo; // bar == 11
```

This is not special syntax, it is just a way to return [`void`](#Void).

`ret` itself is an expression that returns `noret`, so chaining is not possible:
```
_ = {
  ret ret 0; // error: passing 'noret' expression 'ret 0' to 'ret'
};
```

`ret` will actually return from all nested blocks:
```
foo : i32 = 10;
{
  foo++;
  {
    ret {};
    foo++; // not executed
  }
  foo++; // not executed
};
bar : i32 = foo; // bar == 11
```

All of the `ret`s inside a block (and it's nested blocks) have to return the same type:
```
{
  ret {};
  ret 10; // error: previous 'ret' in block returned 'void' and this one returns 'i32'
};
```

#### No return blocks
A no return block is a block with the type [`noret`](#No-return). This type of block must follow the following rules:
1. It _cannot_ have any `ret` operations in it or its nested blocks
2. It _must_ not have a parent block
3. It _must_ have at least one expression
4. Its last expression must be either a call to a [function](#Functions) of returning type `noret` or an [infinite loop](#While)

To define a no return block prefix the block with `!`:
```
!{
  while true do {};
};
```

### Functions
In th kernel the syntax for function defintions is:
```
return_type (param0 : type0, param1 : type1, ...) body;
```

The `body` must be an expression that evaluates to a value of the same type as `return_type`.

Functions are actually just compile-time only values and do not have names. As stated [previously](#Symbols) all of the user defined "things" are either _variables_ or _constants_, functions with names are [usually](#Function-pointers) the latter. The type of a constant that holds a function is `fn`:
```
add :: fn = i32 (x : i32, y : i32) x + y;
```

To call a function use the `()` operator with arguments (if there are any) passed inside it, multiple arguments are separated by commas:
```
ten :: fn = i32 () 10;
sqr :: fn = i32 (x : i32) x*x;
mul :: fn = i32 (x : i32, y : i32) x*y;
_ = ten();        // 10
_ = sqr(10);      // 100
_ = mul(10, 100); // 1000
```

Because functions are just values and [everything is an expression](#Expressions), calling a function definition directly is valid:
```
a : i32 = (i32 (x : i32, y : i32) x+y)(1, 2); # a == 3
```

And nested functions are only natural:
```
foo :: fn = void () {
  bar :: fn = void () {};
};
```

Nested functions have access to the outer functions constants:
```
foo :: fn = void () {
  TEN :: i32 = 10;
  mul_by_ten :: fn = i32 (x : i32) x*TEN; // valid
};
```

But not the outer functions variables:
```
foo :: fn = void () {
  ten : i32 = 10;
  mul_by_ten :: fn = i32 (x : i32) x*ten; // error: undefined symbol 'ten'
};
```

As established before all outer defined constants are still available for the function, this includes the function constant itself. Because of this recursion is possible:
```
fac :: fn = i32 (x : i32) if x < 0 do 1 else x * fac(x-1);
```

Blocks inside of nested functions are _not_ considered nested blocks of the parent function. Using `ret` on them is not going to cause the inner function to also exit:
```
foo :: fn = void () {
  mul_by_ten :: fn = i32 (x : i32) {
    ret x*10;
  };
  a : i32 = mul_by_ten(2); // still runs
};
```

The return type of a function can be [`noret`](#No-return). If that is the case the function will _never_ return to it's caller location and its body must be a [infinite loop](#While), a [no return block](#No-return-blocks), a call to another function with a returning type of `noret` or an [external symbol](#FFI):
```
foo :: fn = noret () while true do {}; // fine
bar :: fn = noret () {}; // error: 'noret' function does return
```

Functions follows the target architecture and operating systems C calling convention.

Naming one or multiple parameters as `_` is possible, you're still not going to be able to read from it. It's basically parameters skipping:
```
foo :: fn = void (_ : i32, _ : i32) {};
```

### Primitives
#### Integers
The kernel provides exactly 131072 integer types, half of these is signed integers and the other half are unsigned.

The identifier referencing to those integer types starts with `i` (signed) or `u` (unsigned), followed by a number in between 1 and 65535:
```
one_bit : u1 = 1;
seven_bits : u7 = 127;
lots_of_bits : u65535 = (1<<65535)-1;
```

The remaining two integer types are `isize` and `usize`. Those have the same size and alignment as [pointers](#Pointers).

The actual size of an integer type will be the minimal bytes necessary for it on the targeted architecture. For example on a modern x86_64 CPU:
```
a : i1; // actually 1 byte in size
b : i15; // actually 2 bytes in size
c : i127; // actually 16 bytes in size
```

There are four types of integer literals:
1. _Decimal:_ `0123456789`
2. _Hexadecimal:_ `0x0123456789abcdef`
3. _Binary:_ `0b01`
4. _Octal:_ `0o01234567`

All of them can be negated with a `-` prefix, there is also a `+` prefix for simetry.

Separeting the digits by underscores is also valid:
```
a : i32 = 123_456;
b : i32 = 0xdead_beef;
c : i32 = 0b1_0_1_0
d : i32 = 0o1234_567;
```

The underscore has to be in between two digits:
```
a : i32 = _0123; // '_0123' is an identifier
b : i32 = 0123_; // error: invalid decimal literal
b : i32 = 01__23; // error: invalid decimal literal
```

Passing a literal that is greater than the integer bit precision will cause a compile-time error:
```
a : i7 = 64; // error: '64' can't be held by 'i7'
```

Operations between integers of different types are not allowed:
```
a : i8 = 1;
b : i16 = 2;
c : i16 = a + b; // error: addition between integers of different types
```

This includes symbol assignment:
```
a : i8 = 1;
b : i16 = a; // error: assigning expression of type 'i8' to variable of type 'i16'
```

Overflow is guaranteed to always wrap for unsigned integers:
```
a : u4 = 15;
b : u4 = a + 1; // b == 0
```

Signed integers are implemented using two's complement. Overflow on them is undefined behavior.

Integers do _not_ decay to floating points:
```
a : f32 = 123; // error: passing integer to floating point
```

All integer expressions supports the following operations:
| Operation | Description                                                               |
|-----------|---------------------------------------------------------------------------|
| +1        | Returns the number                                                        |
| -1        | Negates the number                                                        |
| 1+1       | Adds the numbers                                                          |
| 1-1       | Subtracts the numbers                                                     |
| 1*1       | Multiplies the numbers                                                    |
| 1/1       | Divides the numbers                                                       |
| 1%1       | Get the remainder of the division                                         |
| 1<<1      | Shifts to the left the bits of the lhs number by the rhs number of times  |
| 1>>1      | Shifts to the right the bits of the lhs number by the rhs number of times |
| 1^1       | Do a bitwise exclusive or                                                 |
| 1\|1      | Do a bitwise logical or                                                   |
| 1&1       | Do a bitwise logical and                                                  |
| ~1        | Swaps all of the bits of the number                                       |
| 1 == 1    | Checks if numbers are equal, then return the boolean result               |
| 1 != 1    | Checks if numbers are not equal, then return the boolean result           |
| 1 >= 1    | Checks if lhs is greater or equal to rhs, then return the boolean result  |
| 1 <= 1    | Checks if lhs is less or equal to rhs, then return the boolean result     |
| 1 > 1     | Checks if lhs is greater than rhs, then return the boolean result         |
| 1 < 1     | Checks if lhs is less than lhs, then return the boolean result            |

And integer variables supports the following assignment operations besides the standard one:
| Operation | Description                                                               |
|-----------|---------------------------------------------------------------------------|
| a += 1    | Add number to the variable                                                |
| a -= 1    | Subtract number from the variable                                         |
| a *= 1    | Multipliy number to the variable                                          |
| a /= 1    | Divide number to the variable                                             |
| a %= 1    | Get the remainder of the division and puts in the variable                |
| a <<= 1   | Shifts left by number and put result in the variable                      |
| a >>= 1   | Shifts right by number and put result in the variable                     |
| a ^= 1    | Do a bitwise exclusive or and put result in the variable                  |
| a \|= 1   | Do a bitwise logical or and put result in the variable                    |
| a &= 1    | Do a bitwise logical and and put result in the variable                   |
| a++       | Returns variable current value, then increment it by one                  |
| ++a       | Increment the variable by one and return that value                       |
| a--       | Returns variable current value, then decrement it by one                  |
| --a       | Decrement the variable by one and return that value                       |

Keep in mind that all the assignment operations by default are `noret`, but if wrapped around pipes it is possible to get its value:
```
a : u32 = 10;
b : u32 = |a++|; // valid
c : u32 = a++; // error: assigning 'noret' to 'u32'
```

This is true for any assignment operations, including [floating point](#Floating-points) ones.

Because assignments return `noret` this is not valid:
```
var : u32 = 0;
set_var :: fn = void (value : u32) var = value; // error: passing 'noret' expression to 'void'
```

Wrap it in a block instead:
```
set_var :: fn = void (value : u32) { var = value; }; // valid
```

##### Characters
Characters are nothing more than just integers. Even character literals are just a way to write specific set of integers.

To write a character literal open with a single quote, put the character and close with a single quote:
```
a   : u8 = 'a'; // a == 0x0061
one : u8 = '1'; // b == 0x0031
```

These characters are unicode code points characters.

Because character literals are just another way to do integer literals they work in the same way: 
```
a : u8  = 'a'; // a == 0x0061
b : u32 = '€'; // c == 0x20AC
b : u8  = '€'; // error: '€' can't be held by 'u8'
```

Escaping sequences start with `\` followed by the sequence:
- `\0`: Null character
- `\a`: Bell
- `\b`: Backspace
- `\t`: Horizontal tab
- `\n`: New line
- `\v`: Vertical tab
- `\f`: Form feed
- `\r`: Carriage return
- `\<num>`: Octal value, `<num>` can be any positive octal literal up to 32-bit precision
- `\x<num>`: Hex value, `<num>` can be any positive hexadecimal literal up to 32-bit precision
- `\b<num>`: Bin value, `<num>` can be any positive binary literal up to 32-bit precision
- `\\`: Backslash
- `\"` Double quote

Besides the backslash and double quote having escaping sequences, using the characters directly is viable and preferable. They are here for [string literal](#Strings) consistency:
```
a : u8 = '\'; // equivalent to '\\'
b : u8 = '"'; // equivalent to '\"'
```

Differently from other programming languages to get the character of a single quote simply wrap the quote in single quotes:
```
a : u8 = '''; // single quote character
```

A escaped single quote is a syntax error.

#### Floating points
There are four types of floating point numbers in the Stark kernel:
- `f16`: Half precision floating point
- `f32`: Single precision floating point
- `f64`: Double precision floating point
- `f80`: Extended precision floating point
- `f128`: Quadruple precision floating point

Floating point literals follow the `<digits>.<digits>` syntax:
```
a : f32 = 123.456;
```

Omitting the digits on one side at a time is valid (it'll default to zero):
```
a : f32 = .1; // equivalent '0.1'
a : f32 = 1.; // equivalent '1.0'
a : f32 = .; // error: invalid syntax
```

To negated a floating point literal use the `-` prefix, there is also a `+` prefix for simetry.

Separeting the digits by underscores is also valid on floating points and follow the same rules as integers:
```
a : i32 = 123_456.7_8_9; // valid
b : i32 = 0_.0; // error: invalid floating point literal
c : i32 = 0._0; // error: invalid floating point literal
d : i32 = 0__0.0; // error: invalid floating point literal
e : i32 = 0.0_; // error: invalid floating point literal
f : i32 = _0.0; // error: invalid syntax
```

Operations between floating points of different types are not allowed:
```
a : f32 = 1.0;
b : f64 = 2.0;
c : i16 = a + b; // error: addition between floating points of different types
```

This includes symbol assignment:
```
a : f32 = 1;
b : f64 = a; // error: assigning expression of type 'f32' to variable of type 'f64'
```

Floating points do _not_ decay into integers:
```
a : i32 = 123.0; // error: passing floating point to integer
```

All floating point expressions supports the following operations:
| Operation  | Description                                                               |
|------------|---------------------------------------------------------------------------|
| +1.0       | Returns the number                                                        |
| -1.0       | Negates the number                                                        |
| 1.0+1.0    | Adds the numbers                                                          |
| 1.0-1.0    | Subtracts the numbers                                                     |
| 1.0*1.0    | Multiplies the numbers                                                    |
| 1.0/1.0    | Divides the numbers                                                       |
| 1.0 == 1.0 | Checks if numbers are equal, then return the boolean result               |
| 1.0 != 1.0 | Checks if numbers are not equal, then return the boolean result           |
| 1.0 >= 1.0 | Checks if lhs is greater or equal to rhs, then return the boolean result  |
| 1.0 <= 1.0 | Checks if lhs is less or equal to rhs, then return the boolean result     |
| 1.0 > 1.0  | Checks if lhs is greater than rhs, then return the boolean result         |
| 1.0 < 1.0  | Checks if lhs is less than lhs, then return the boolean result            |

And integer variables supports the following assignment operations besides the standard one:
| Operation | Description                                                               |
|-----------|---------------------------------------------------------------------------|
| a += 1.0  | Add number to the variable                                                |
| a -= 1.0  | Subtract number from the variable                                         |
| a *= 1.0  | Multipliy number to the variable                                          |
| a /= 1.0  | Divide number to the variable                                             |
| a %= 1.0  | Get the remainder of the division and puts in the variable                |
| a++       | Returns variable current value, then increment it by one                  |
| ++a       | Increment the variable by one and return that value                       |
| a--       | Returns variable current value, then decrement it by one                  |
| --a       | Decrement the variable by one and return that value                       |

#### Booleans
The type of booleans is `bool`, they are memory equivalent to a `u1`.

Booleans have only two possible states `true` (`1` in memory) and `false` (`0` in memory).

All boolean expressions supports the following operations:
| Operation      | Description                               |
|----------------|-------------------------------------------|
| true && true   | Logical and between the two expressions   |
| true \|\| true | Logical or between the two expressions    |
| !true          | Logical negation of an expression         |
| true == true   | Checks if both expressions are equivalent |
| true != true   | Checks if both expressions are different  |

#### C types
There are a set of types that are equivalent to the C types of your target platform:
- `c_char`: `char`
- `c_short`: `short`
- `c_int`: `int`
- `c_long`: `long`
- `c_llong`: `long long`
- `c_uchar`: `unsigned char`
- `c_ushort`: `unsigned short`
- `c_uint`: `unsigned int`
- `c_ulong`: `unsigned long`
- `c_ullong`: `unsigned long long`
- `c_float`: `float`
- `c_double`: `double`
- `c_ldouble`: `long double`

They will actually become just type aliases to the correct Stark kernel types, for example: in x86_64 linux this would be the actual aliases:
```
c_char    :: type = i8;
c_short   :: type = i16;
c_int     :: type = i32;
c_long    :: type = i64;
c_llong   :: type = i64;
c_uchar   :: type = u8;
c_ushort  :: type = u16;
c_uint    :: type = u32;
c_ulong   :: type = u64;
c_ullong  :: type = u64;
c_float   :: type = f32;
c_double  :: type = f64;
c_ldouble :: type = f80;
```

#### Void
`void` is a type with exactly one state, and that is `void` itself.

This type has a size and alignment of 0.

Differently from most language, in the kernel a variable with type void is acceptable:
```
a : void;
```

The variable will have an address, but it'll not allocate any size on the stack.

There is no actual `void` literal, but because [blocks](#Blocks) return void by default, anempty block can be used as a sort of "void literal":
```
a : void = {};
```

Reads and writes to a `void` variable or constant are NOPs.

#### No return
The no return type, represented by the `noret` identifier, is basically a type that do not return any value, much like [`void`](#Void).

The differences from `void` are in its uses for [functions](#Functions) and [blocks](#No-return-blocks). And mainly that it is not possible to declare variables and constants with the type `noret`.

#### Type
`type` is the type of types. Only constants can have this type. It is basically used to do type aliasing:
```
integer : type = i32;
```

#### Function types
`fn` is the type of [functions](#Functions).

It is only possible to have constants with this type, not variables.

Operators, _only_ the operators without expressions, can be assigned to function types. The expressions will become arguments to the functions in order:
```
add :: fn = +;
my_if_else :: fn = if do else;
a : i32 = add(1, 2);
b : f32 = add(1.0, 2.0);
c : i32 = my_if_else(a == 3, 4, 5);
```

#### Module type
`mod` is the type of [modules](#Modules)

`mod` is just a namespace with all the public [symbols](#Symbols) of a module.

It is only possible to have constants with this type, not variables.

#### Identifier
Identifier in itself is also a type. This is a constant-only type.

The type of an identifier is `iden`, you can put any valid identifier in it

In the kernel identifiers has to follow 2 rules to be considered valid:
1. It must start with an ASCII letter or underscore.
2. After the first character, it optionally can contain a sequence of ASCII letters, underscores and ASCII number digits.
3. The identifier must not be part of an operator.

Example of valid and invalid identifiers:
```
iden0 :: iden = x; // valid
iden1 :: iden = _10; // valid
iden2 :: iden = i32; // valid
iden3 :: iden = iden; // valid
iden4 :: iden = 0x; // error: invalid syntax '0' followed by 'x'
iden5 :: iden = if; // error: 'if' is part of an operator, thus it is not an identifier
```

### Arrays
The type of arrays is `[N]T`, where `T` is any type that can be assigned to a variable _including arrays themselves_, and `N` is the number of elements of the array.

To index an array use the `[]` operator:
```
buff : [3]i32;
buff[0] = 1;
buff[2] = 2;
buff[3] = 3;
a = buff[0]; // a == 1
b = buff[2]; // b == 2
c = buff[3]; // c == 3
```

Arrays do not decay to pointers, they are a buffer of sequential memory in the stack itself. When a function takes an array as argument or returns an array, an element-wise copy occurs:
```
add :: fn = [2]i32 (a : [2]i32, b : [2]i32) {
  result : [2]i32;
  result[0] = a[0] + b[0];
  result[1] = a[1] + b[1];
  ret result;
};
a : [2]i32;
b : [2]i32;
a[0] = 1; a[1] = 2;
b[0] = 3; b[1] = 4;
c : [2]i32 = add(a, b); // valid
```

The kernel doesn't provide any types of array literals.

### Pointers
The type of pointers is `*T`, where `T` is any type that can be assigned to a variable _including pointers themselves_.

To take the address of variables use the `&` operator:
```
a : i32;
p : *i32 = &a;
```

To dereference a pointer use the following syntax:
```
a : i32;
p : *i32 = &a;
p* = 10;
b : i32 = p*; // b == 10
```

Pointer arithmetic is possible, it'll always advance by the number times the size of the type the pointer points to:
```
buff : [3]i32;
p = &buff;
(p + 0)* = 1;
(p + 1)* = 2;
(p + 3)* = 3;
a : i32 = (p + 0)*; // a == 1
b : i32 = (p + 1)*; // a == 2
c : i32 = (p + 2)*; // a == 3
```

Indexing pointers with `[]` is not allowed in the kernel.

#### Strings
Strings are actually just pointers to a UTF-8 NUL-terminated buffer.

String literals start with a double quote and end with a double quote:
```
msg : *u8 = "hello, world!";
```

The buffer generated by string literals lives on the read-only static memory of the program.

The same escape sequence of [characters](#Characters) are available for string literals:
```
msg : *u8 = "\thello,\n\tworld\n";
```

To have a double quote in the string use the escaped double quote:
```
msg : *u8 = "he said \"something\"";
```

### Function pointers
Function pointers are a special type of pointer that enables the usage of the function call operator `()` on it.

A type of a function pointer is `*return_type (type0, type1, ...)`.

It is possible to use the `&` operator to take the address of functions:
```
add :: fn = i32 (x : i32, y : i32) x + y;
padd : *i32 (i32, i32) = &add;
a : i32 = padd(1, 2); // a == 3
```

Functions do not decay to function pointers in the kernel, taking it's address is needed.

### Structs
Struct are user defined types with custom memory layout.

The syntax for a struct definition is:
```
(member0 : type0, member1 : type1, ...)
```

Like [functions](#Functions), structs are also just compile time values that do not have names. They can be used directly when creating variables or constants:
```
point : (x : i32, y : i32);
```

To access field members of structs use the `.` operator:
```
point : (x : i32, y : i32);
point.x = 1;
point.y = 2;
a : i32 = point.x + point.y; // a == 3
```

Every struct is unique, even if they have the same memory layout or even the same member names:
```
point0 : (x : i32, y : i32);
point1 : (x : i32, y : i32);
point0.x = 1;
point0.y = 2;
point1 = point0; // error: assigning 'point0' to 'point1', but their types differ
```

Because structs are types they have the type [`type`](#Type), type aliasing is the way to "name" them:
```
Point :: type = (x : i32, y : i32);
point0 : Point;
point1 : Point;
point0.x = 1;
point0.y = 2;
point1 = point0; // fine
```

The alignment of a struct will be equal to the member with the biggest alignment:
```
Foo :: type = (a : u32, b : u16); // alignment will be 4
```

The size of a struct needs to be a multiple of its alignment. Because of this, if it's needed, an implicit padding will be added at the end of the struct:
```
Foo :: type = (a : u32, b : u16); // the alignment is 4, but the size would've being 6. because of this it receives 2 bytes of padding.
```

Misalignment between inner fields is not allowed:
```
Entity :: type = (alive : bool, id : u32); // error: member 'alive' has 3 bytes of misalignment
```

To resolve this relayout your struct:
```
Entity :: type = (id : u32, alive : bool); // fine
```

Or add explicit padding:
```
Entity :: type = (alive : bool, _pad : u3, id : u32); // fine
```

This is done to prevent silent memory wastage, for example in C this:
```c
struct foo {
    bool flag0;
    uint32_t id;
    bool flag1;
};
```

Would be fine, but it would generate 6 bytes of wastage. In the Stark kernel this is simply not allowed, the only implicit padding is at the end where it really is necessary.

The `_` symbol can be used as a struct member and can't be used for multiple fields, this is used specifically for padding:
```
Foo :: type = (flag0 : bool, _ : u3, id0 : u32, flag1 : bool, _ : u3, id1 : u32); // fine
```

It is not possible to access members defined as `_`:
```
foo : Foo;
foo._ = 10; // error: can't read from member '_'
```

Dereferencing a struct just to get the address of a member will not cause an actual dereferencing:
```
Point :: type = (x : i32, y : i32);
p : *Point = 0;
py : *i32 = &(p*.y); // no dereference actually occurs, therefore the program will not crash
```

The kernel doesn't provide any type of struct initializer or constructor.

### Casting
Somtimes converting an expression from one type to another is required. Because of this __casting__ exists. The casting operator is `->` and work as the following:
```
expression -> type;
```

#### Integer to integer
As explained on the [integers section](#Integers), integer operations can only occur between integers of the same type:
```
a : u32 = 10;
b : i32 = a; // error: assigning expression of type 'u32' to variable of type 'i32'
```

To overcome this issue casting between integer expressions to _any_ integer type is possible:
```
a : u32 = 10;
b : i32 = a -> i32; // fine
```

If an expression is not within the range of the integer type that it is being casted to wrapping will occur:
```
a : u32 = 0xffffffff;
b : i32 = a -> i32; // b == -1
```

#### Floating point to floating point
Similarly to integer expressions, [floating point](#Floating-points) operations can only occur between floating points of the same type:
```
a : f32 = 10.0;
b : f64 = a; // error: assigning expression of type 'f32' to variable of type 'f64'
```

Casting between floating point types is possible:
```
a : f32 = 10.0;
b : f64 = a -> f64; // fine
```

#### Integer to floating point and vice-versa
As explained in both the [integer](#Integers) and [floating point](#Floating-points) sections operations between them is not possible:
```
a : i32 = 10;
b : f32 = a; // error: assigning expression of type 'i32' to variable of type 'f32'
```

But as expected casting between them is possible:
```
a : i32 = 10;
b : f32 = 10.0;
c : f32 = a -> f32; // fine
d : i32 = b -> i32; // fine
```

#### Pointer to pointer
Casting between any two types of [pointers](#Pointers) is valid:
```
a  : [4]u8;
p0 : *u32 = &a -> *u32; // valid, casting from '*[4]u8' to '*u32'
```

#### Integer to pointer and vice-versa
Casting between [pointers](#Pointers) and [integers](#Integers) is valid, but only the integers `usize` and `isize`:
```
a  : usize = 0xdeadbeef;
b  : u64   = 0xdeadbeef;
p0 : *void = a -> *void; // valid
p1 : *void = b -> *void; // error: casting from 'u64' to '*void'
```

The inverse is also valid:
```
v : void;
p : *void = &v;
a : usize = p -> usize; // valid
b : u64   = p -> u64; // error: casting from '*void' to 'u64'
```

#### Struct to struct
Casting between [struct](#Structs) types is permitted, but only if the struct layout is the exact same:
```
Vec3  :: type = (x : f32, y : f32, z : f32);
Vec2  :: type = (x : f32, y : f32);
Color :: type = (r : f32, g : f32, b : f32);
pos : Vec3;
pos.x = 1.0; pos.y = 2.0; pos.z = 3.0;
col : Color = pos -> Color; // valid
pos2d : Vec2 = pos -> Vec2; // error: casting from 'Vec3' to 'Vec2' is not possible
```

#### Identifier to string
Constants that have the [identifier](#Identifier) type can be casted to `*u8`, they will be treated just like a strings and will become part of the static memory:
```
some_iden :: iden = abc;
a_string : *u8 = some_iden -> *u8; // a_string == "abc"
```

### If
`if`s in the kernel are operators, they are written in the following way:
```
if condition do body;
```

Where `condition` is a [boolean](#Booleans) expression and `body` a [`void`](#Void) or [`noret`](#No-return) expression.

An `if` operation is an expression of type [`noret`](#No-return).

There is also the `if do else` operator:
```
if condition do body0 else body1;
```

A `if do else` operation will return something different depending on its bodies return types:
- If `body0` is either `noret` or `void` `body1` must be either `noret` or `void`. The returning type will be `noret`
- If `body0` is any other type then `body1` must have the same type. The returning expression will be the evaluated body

Examples of `if do else`:
```
foo :: fn = void (x : i32, y : i32) {
  if x == y do // this 'if do else' will be a 'noret' expression
    this_thing()
  else
    this_other_thing();

  if x > y do // this 'if do else' will be a 'noret' expression
    that_thing()
  else if x == 10 do // nesting an 'if do' operation inside the 'if do else' operation
    that_other_thing();

  if x < y do // this 'if do else' will be a 'noret' expression
    something()
  else if x == 10 do // nesting an 'if do else' operation inside the 'if do else' operation
    something_else()
  else
    something_else_entirely();

  z : i32 = if x < 0 && y > 0 do x*10 else y*10; // this 'if do else' will be an 'i32' expression
  if x > 0 && y < 0 do
    x*10
  else
    void_returning_func(); // error: 'if do else' branches differ in type
};
```

### While
The syntax for a while loop operation is
```
while condition do body;
```

While operations are `noret` expressions. The `body` must be [`void`](#Void) or [`noret`](#No-return).

### Modules
In the kernel a file is a module. The file name is the module name.

The only expressions permitted at module scope are [symbol definitions](#Symbols). You may assign this symbols _once_ (even variables) to [constant expressions](#Constant-expressions) only:
```
// module scope
var : u32 = 10; // fine
const :: u32 = 10; // fine
ten_fn :: fn = u32 () 10; // fine
ten : u32 = ten_fn(); // error: 'ten_fn()' is not a constant expression
foo(); // error: only symbol definitions are permitted at module scope
```

All public symbols defined within a module scope become available to the entire scope, regardless of definition order:
```
twenty :: i32 = ten*2; // valid
ten    :: i32 = 10;
```

Cyclic assignments are not valid:
```
twenty :: i32 = ten*2; // error: cyclic assignment
ten    :: i32 = twenty/2;
```

A module is divided in sections. Each section is either `prv` (private) or `pub` (public):
```
prv
g_inc_state :: i32 = 2;
g_state      : i32 = 0;
pub
next_state  :: fn = i32 () |g_state += g_inc_state|;
reset_state :: fn = void () { g_state = 0; };
```

It is possible to have _multiple_ sections:
```
prv
g_inc_state :: i32 = 2;
pub
next_state  :: fn = i32 () |state += g_inc_state|;
prv
g_state      : i32 = 0;
pub
reset_state :: fn = void () { state = 0; };
```

A module must have _at least_ one `pub` section or one `prv` section.

To include a module use the `inc` operator, this operator takes a string literal and outputs a `mod`:
```
other_module :: mod = inc "other_module";
```

The string passed by `inc` is the name of the source Stark file. This file will be searched in your include path.

`mod` is a constant-only type, it cannot be used on variable:
```
other_module : mod = inc "other_module"; // error: variables can't have type 'mod'
```

All symbols under the `pub` sections of the included module will become available:
```
other_module :: mod = inc "other_module";
other_module.some_func();
```

Including a module more than once is valid, this will just cause an alias to be created:
```
other_module       :: mod = inc "other_module";
other_module_again :: mod = inc "other_module"; // fine
```

When modules are included they will be part of the build compilation. The compilation is unity build, symbols in Stark are not equivalent to symbols exported on the binary.

Even if a module is included twice, even a module being included inside two different modules, they will only be compiled once.

It is possible to mark a module to be build as an object file with the `incobj` operation:
```
other_module :: mod = incobj "other_module"; // this module will be build into an object file
```

If a module is included as an object file at least once it'll not be part of the unity build even if other modules it is included as just `inc`.

Differently from the unity build, the symbols of object file __will match__ the Stark symbols names.

### FFI
To get external symbols from object files and static or dynamic libraries use the `extsym` operation.

This operation will return an intermediate symbol that can be passed to the following places:
- The body of a funcion
- The value of a variable

In both of those cases the Stark symbol will become a typed alias:
```
exit :: fn = noret (code : c_int) extsym "exit";
errno :: c_int = extsym "errno";
```

The external symbols will be looked up inside libraries that the program is being linked to or in object file that the program is being compiled with (this does not include `incobj` object files).

If two external symbols match the given name an error will occur.

The last argument of a function can be the value `c_vargs`, this is explicitly for interacting with C functions with variadic arguments, Stark does not have any mechanisms to deal with them inside stark itself:
```
printf :: fn = i32 (fmt : *c_char, c_vargs) extsym "printf";
main :: fn = void () {
  printf("hello, %s %d times" -> *c_char, "world", 10);
};
```

### Operators
Everything in the Stark kernel is an [expression](#Expressions) or an operation around expressions.



### Compile pipeline

```
my_compipe :: fn = *stark_ast (src : *u8, src_size : usize) {
  
};
compipe = my_compipe;
```

## The Standard Features Extension

