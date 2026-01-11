# Strada Language Manual

This is the complete technical reference for the Strada programming language. For tutorials, see [Tutorial](TUTORIAL.md). For quick syntax lookup, see [Quick Reference](QUICK_REFERENCE.md).

## Table of Contents

1. [Lexical Structure](#1-lexical-structure)
2. [Types](#2-types)
3. [Variables](#3-variables)
4. [Operators](#4-operators)
5. [Expressions](#5-expressions)
6. [Statements](#6-statements)
7. [Functions](#7-functions)
8. [Arrays](#8-arrays)
9. [Hashes](#9-hashes)
10. [References](#10-references)
11. [Strings](#11-strings)
12. [Regular Expressions](#12-regular-expressions)
13. [Control Flow](#13-control-flow)
14. [Exception Handling](#14-exception-handling)
15. [Packages and Modules](#15-packages-and-modules)
16. [Object-Oriented Programming](#16-object-oriented-programming)
17. [Closures](#17-closures)
18. [Multithreading](#18-multithreading)
19. [Foreign Function Interface](#19-foreign-function-interface)
20. [Built-in Functions](#20-built-in-functions)
21. [Magic Variables](#21-magic-variables)
22. [Reserved Words](#22-reserved-words)

---

## 1. Lexical Structure

### 1.1 Character Set

Strada source files are encoded in UTF-8. Identifiers and keywords use ASCII characters only.

### 1.2 Comments

```strada
# Single-line comment (extends to end of line)

/* Multi-line comment
   can span multiple lines */
```

### 1.3 Identifiers

Identifiers start with a letter or underscore, followed by letters, digits, or underscores.

```
identifier := [a-zA-Z_][a-zA-Z0-9_]*
```

Identifiers are case-sensitive: `$foo`, `$Foo`, and `$FOO` are distinct.

### 1.4 Sigils

Variable names are prefixed with sigils indicating their type:

| Sigil | Type | Example |
|-------|------|---------|
| `$` | Scalar (single value) | `$count`, `$name` |
| `@` | Array | `@items`, `@numbers` |
| `%` | Hash | `%config`, `%data` |
| `&` | Subroutine reference | `\&func` |

### 1.5 Literals

**Integer literals:**
```strada
42        # Decimal
0xFF      # Hexadecimal
0o77      # Octal
0b1010    # Binary
```

**Floating-point literals:**
```strada
3.14      # Standard
1.5e10    # Scientific notation
2.5E-3    # Scientific with negative exponent
```

**String literals:**
```strada
"double quoted"     # Allows escape sequences
'single quoted'     # Literal (minimal escaping)
```

**Escape sequences in double-quoted strings:**

| Sequence | Meaning |
|----------|---------|
| `\n` | Newline |
| `\t` | Tab |
| `\r` | Carriage return |
| `\\` | Backslash |
| `\"` | Double quote |
| `\0` | Null character |

**Special literals:**
```strada
undef     # Undefined value
```

### 1.6 Operators and Punctuation

```
+ - * / %           # Arithmetic
== != < > <= >=     # Numeric comparison
eq ne lt gt le ge   # String comparison
<=>                 # Spaceship (comparison)
&& || !             # Logical
& | ^ ~ << >>       # Bitwise
. x                 # String (concat, repeat)
= += -= *= /= .=    # Assignment
-> =>               # Arrow, fat comma
\ @{ %{ ${         # Reference operators
```

---

## 2. Types

### 2.1 Type System Overview

Strada is statically typed. Variables must have declared types, but implicit conversions occur in certain contexts.

### 2.2 Scalar Types

| Type | Description | Size | Range |
|------|-------------|------|-------|
| `int` | Signed integer | 64 bits | -2^63 to 2^63-1 |
| `num` | Floating point | 64 bits | IEEE 754 double |
| `str` | String | Variable | UTF-8 encoded |
| `scalar` | Generic scalar | Variable | Any of the above |

### 2.3 Composite Types

| Type | Description |
|------|-------------|
| `array` | Ordered list of scalars |
| `hash` | Key-value map (string keys) |

### 2.4 Special Types

| Type | Description |
|------|-------------|
| `void` | No value (for function returns) |
| `undef` | Undefined/uninitialized value |

### 2.5 Reference Types

References point to other values:

```strada
\$scalar    # Reference to scalar
\@array     # Reference to array
\%hash      # Reference to hash
\&func      # Reference to function
```

### 2.6 Type Conversions

**Implicit conversions:**

| From | To | Rule |
|------|----|------|
| `int` | `num` | Exact conversion |
| `int` | `str` | Decimal representation |
| `num` | `int` | Truncation toward zero |
| `num` | `str` | Standard floating-point format |
| `str` | `int` | Parse as integer, 0 if invalid |
| `str` | `num` | Parse as float, 0.0 if invalid |

**Explicit casting:**

```strada
my int $i = cast_int($value);
my num $n = cast_num($value);
my str $s = cast_str($value);
```

### 2.7 Boolean Context

Values are evaluated as boolean in conditions:

| Type | False when | True when |
|------|------------|-----------|
| `int` | 0 | Non-zero |
| `num` | 0.0 | Non-zero |
| `str` | "" or "0" | Non-empty and not "0" |
| `array` | Empty | Non-empty |
| `hash` | Empty | Non-empty |
| `undef` | Always | Never |
| Reference | Never | Always |

---

## 3. Variables

### 3.1 Declaration

Variables are declared with `my`:

```strada
my TYPE SIGIL NAME;
my TYPE SIGIL NAME = EXPRESSION;
```

Examples:
```strada
my int $count;              # Uninitialized (undef)
my int $count = 0;          # Initialized
my str $name = "Alice";
my array @items = ();
my hash %config = ();
my scalar $value = 42;
```

### 3.2 Scope

Variables are lexically scoped to the enclosing block:

```strada
func example() void {
    my int $x = 1;          # Visible in entire function

    if ($x > 0) {
        my int $y = 2;      # Visible only in if block
        say($x + $y);       # OK: both visible
    }

    say($x);                # OK
    # say($y);              # Error: $y not in scope
}
```

### 3.3 Global Variables

Package-level variables are accessible across functions:

```strada
package MyApp;

my int $global_count = 0;   # Package global

func increment() void {
    $global_count = $global_count + 1;
}

func get_count() int {
    return $global_count;
}
```

### 3.4 Special Variables

| Variable | Description |
|----------|-------------|
| `@ARGV` | Command-line arguments |
| `$ARGC` | Argument count |
| `$_` | Default variable (in map/grep/sort) |
| `$a`, `$b` | Sort comparison variables |
| `__PACKAGE__` | Current package name |
| `__FILE__` | Current source file |
| `__LINE__` | Current line number |

---

## 4. Operators

### 4.1 Operator Precedence (highest to lowest)

| Precedence | Operators | Associativity |
|------------|-----------|---------------|
| 1 | `->` | Left |
| 2 | `++` `--` | N/A |
| 3 | `!` `~` `\` `-` (unary) | Right |
| 4 | `**` | Right |
| 5 | `*` `/` `%` `x` | Left |
| 6 | `+` `-` `.` | Left |
| 7 | `<<` `>>` | Left |
| 8 | `<` `>` `<=` `>=` `lt` `gt` `le` `ge` | Left |
| 9 | `==` `!=` `<=>` `eq` `ne` | Left |
| 10 | `&` | Left |
| 11 | `|` `^` | Left |
| 12 | `&&` | Left |
| 13 | `||` | Left |
| 14 | `? :` | Right |
| 15 | `=` `+=` `-=` etc. | Right |
| 16 | `,` | Left |

### 4.2 Arithmetic Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `+` | Addition | `$a + $b` |
| `-` | Subtraction | `$a - $b` |
| `*` | Multiplication | `$a * $b` |
| `/` | Division | `$a / $b` |
| `%` | Modulo | `$a % $b` |
| `**` | Exponentiation | `$a ** $b` |
| `-` (unary) | Negation | `-$a` |

### 4.3 String Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `.` | Concatenation | `$a . $b` |
| `x` | Repetition | `$s x 3` |

### 4.4 Comparison Operators

**Numeric:**

| Operator | Description |
|----------|-------------|
| `==` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less or equal |
| `>=` | Greater or equal |
| `<=>` | Spaceship (-1, 0, or 1) |

**String:**

| Operator | Description |
|----------|-------------|
| `eq` | Equal |
| `ne` | Not equal |
| `lt` | Less than |
| `gt` | Greater than |
| `le` | Less or equal |
| `ge` | Greater or equal |
| `cmp` | Spaceship for strings |

### 4.5 Logical Operators

| Operator | Description |
|----------|-------------|
| `&&` | Logical AND (short-circuit) |
| `||` | Logical OR (short-circuit) |
| `!` | Logical NOT |

### 4.6 Bitwise Operators

| Operator | Description |
|----------|-------------|
| `&` | Bitwise AND |
| `|` | Bitwise OR |
| `^` | Bitwise XOR |
| `~` | Bitwise NOT |
| `<<` | Left shift |
| `>>` | Right shift |

### 4.7 Assignment Operators

| Operator | Equivalent |
|----------|------------|
| `=` | Assignment |
| `+=` | `$x = $x + $y` |
| `-=` | `$x = $x - $y` |
| `*=` | `$x = $x * $y` |
| `/=` | `$x = $x / $y` |
| `%=` | `$x = $x % $y` |
| `.=` | `$s = $s . $t` |
| `&=` | `$x = $x & $y` |
| `|=` | `$x = $x | $y` |

### 4.8 Other Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `? :` | Ternary conditional | `$x > 0 ? "pos" : "neg"` |
| `->` | Dereference/method call | `$ref->{"key"}` |
| `\` | Reference creation | `\$var` |
| `..` | Range | `1..10` |

---

## 5. Expressions

### 5.1 Primary Expressions

- Literals: `42`, `3.14`, `"hello"`, `undef`
- Variables: `$x`, `@arr`, `%hash`
- Function calls: `func()`, `func($arg)`
- Parenthesized: `($a + $b)`

### 5.2 Array/Hash Access

```strada
$array[INDEX]           # Array element
$hash{"KEY"}            # Hash element
$ref->[INDEX]           # Dereference array element
$ref->{"KEY"}           # Dereference hash element
```

### 5.3 Anonymous Constructors

```strada
[EXPR, EXPR, ...]       # Anonymous array reference
{ KEY => VAL, ... }     # Anonymous hash reference
```

### 5.4 Function Expressions (Closures)

```strada
func (PARAMS) { BODY }
func (PARAMS) RETURN_TYPE { BODY }
```

---

## 6. Statements

### 6.1 Expression Statement

Any expression followed by semicolon:

```strada
$x = $y + 1;
say("hello");
```

### 6.2 Block Statement

```strada
{
    STATEMENT;
    STATEMENT;
    ...
}
```

### 6.3 Declaration Statement

```strada
my TYPE SIGIL NAME;
my TYPE SIGIL NAME = EXPR;
```

### 6.4 Empty Statement

```strada
;
```

---

## 7. Functions

### 7.1 Function Definition

```strada
func NAME(PARAMETERS) RETURN_TYPE {
    BODY
}
```

Examples:
```strada
func add(int $a, int $b) int {
    return $a + $b;
}

func greet(str $name) void {
    say("Hello, " . $name);
}
```

### 7.2 Parameters

**Required parameters:**
```strada
func example(int $x, str $y, num $z) void { }
```

**Default parameters:**
```strada
func greet(str $name, str $greeting = "Hello") void {
    say($greeting . ", " . $name);
}
```

**Array parameters:**
```strada
func sum(array @nums) int {
    my int $total = 0;
    foreach my int $n (@nums) {
        $total = $total + $n;
    }
    return $total;
}
```

**Hash parameters:**
```strada
func process(hash %options) void {
    if (exists($options{"verbose"})) {
        say("Verbose mode");
    }
}
```

### 7.3 Return Values

```strada
func example() int {
    return 42;
}

func nothing() void {
    # No return value
}

func multi() array {
    return (1, 2, 3);  # Return array
}
```

### 7.4 Recursion

Functions can call themselves:

```strada
func factorial(int $n) int {
    if ($n <= 1) {
        return 1;
    }
    return $n * factorial($n - 1);
}
```

### 7.5 Forward Declarations

For mutual recursion:

```strada
func is_even(int $n) int;  # Forward declaration

func is_odd(int $n) int {
    if ($n == 0) { return 0; }
    return is_even($n - 1);
}

func is_even(int $n) int {
    if ($n == 0) { return 1; }
    return is_odd($n - 1);
}
```

---

## 8. Arrays

### 8.1 Creation

```strada
my array @empty = ();
my array @nums = (1, 2, 3, 4, 5);
my array @mixed = (1, "two", 3.0);
```

### 8.2 Access

```strada
@arr[INDEX]             # Get element (0-based)
@arr[-1]                # Last element
@arr[-2]                # Second to last
```

### 8.3 Modification

```strada
@arr[INDEX] = VALUE;    # Set element
push(@arr, VALUE);      # Add to end
pop(@arr);              # Remove from end
unshift(@arr, VALUE);   # Add to beginning
shift(@arr);            # Remove from beginning
```

### 8.4 Array Functions

| Function | Description |
|----------|-------------|
| `push(@arr, $val)` | Append element |
| `pop(@arr)` | Remove and return last |
| `shift(@arr)` | Remove and return first |
| `unshift(@arr, $val)` | Prepend element |
| `size(@arr)` | Get length |
| `reverse(@arr)` | Reverse array |
| `sort(@arr)` | Sort (default) |
| `sort { BLOCK } @arr` | Sort with comparator |

### 8.5 Iteration

```strada
foreach my TYPE $var (@arr) {
    # Process $var
}

for (my int $i = 0; $i < size(@arr); $i = $i + 1) {
    # Access @arr[$i]
}
```

### 8.6 Map, Grep, Sort

```strada
# Map - transform elements
my scalar $result = map { EXPR; } @arr;

# Grep - filter elements
my scalar $result = grep { CONDITION; } @arr;

# Sort - custom comparison
my scalar $result = sort { $a <=> $b; } @arr;
```

---

## 9. Hashes

### 9.1 Creation

```strada
my hash %empty = ();
my scalar $href = { key => "value", num => 42 };
```

### 9.2 Access

```strada
$hash{"KEY"}            # Get value
$hash{"KEY"} = VALUE;   # Set value
$href->{"KEY"}          # Via reference
```

### 9.3 Hash Functions

| Function | Description |
|----------|-------------|
| `keys(%hash)` | Get all keys as array |
| `values(%hash)` | Get all values as array |
| `exists($hash{"key"})` | Check if key exists |
| `delete($hash{"key"})` | Remove key |
| `size(%hash)` | Get number of keys |

### 9.4 Iteration

```strada
foreach my str $key (keys(%hash)) {
    my scalar $val = $hash{$key};
    say($key . " => " . $val);
}
```

---

## 10. References

### 10.1 Creating References

```strada
\$scalar    # Reference to scalar variable
\@array     # Reference to array variable
\%hash      # Reference to hash variable
\&func      # Reference to function

[1, 2, 3]   # Anonymous array reference
{a => 1}    # Anonymous hash reference
```

### 10.2 Dereferencing

**Scalar references:**
```strada
$$ref           # Dereference (read/write)
deref($ref)     # Alternative read
deref_set($ref, $val)  # Alternative write
```

**Array references:**
```strada
@{$ref}         # Full array
$ref->[$i]      # Element access
```

**Hash references:**
```strada
%{$ref}         # Full hash
$ref->{"key"}   # Element access
```

### 10.3 Reference Functions

| Function | Description |
|----------|-------------|
| `is_ref($val)` | Check if value is a reference |
| `reftype($ref)` | Get reference type ("SCALAR", "ARRAY", "HASH") |
| `deref($ref)` | Dereference scalar |
| `deref_set($ref, $val)` | Set through scalar reference |

---

## 11. Strings

### 11.1 String Functions

| Function | Description |
|----------|-------------|
| `length($s)` | Get string length |
| `substr($s, $start)` | Substring from start |
| `substr($s, $start, $len)` | Substring with length |
| `index($s, $sub)` | Find substring position |
| `rindex($s, $sub)` | Find from end |
| `uc($s)` | Uppercase |
| `lc($s)` | Lowercase |
| `ucfirst($s)` | Capitalize first |
| `lcfirst($s)` | Lowercase first |
| `trim($s)` | Remove leading/trailing whitespace |
| `ltrim($s)` | Remove leading whitespace |
| `rtrim($s)` | Remove trailing whitespace |
| `reverse($s)` | Reverse string |
| `chomp($s)` | Remove trailing newline |
| `chop($s)` | Remove last character |
| `chr($n)` | Character from code point |
| `ord($s)` | Code point from character |
| `join($sep, @arr)` | Join array elements |
| `split($sep, $s)` | Split string to array |

---

## 12. Regular Expressions

### 12.1 Match Operator

```strada
$string =~ /PATTERN/          # Match
$string !~ /PATTERN/          # Negated match
```

### 12.2 Substitution Operator

```strada
$string =~ s/PATTERN/REPLACEMENT/     # Replace first
$string =~ s/PATTERN/REPLACEMENT/g    # Replace all
```

### 12.3 Pattern Syntax

| Pattern | Matches |
|---------|---------|
| `.` | Any character (except newline) |
| `^` | Start of string |
| `$` | End of string |
| `*` | Zero or more |
| `+` | One or more |
| `?` | Zero or one |
| `{n}` | Exactly n |
| `{n,m}` | Between n and m |
| `[abc]` | Character class |
| `[^abc]` | Negated class |
| `\d` | Digit |
| `\w` | Word character |
| `\s` | Whitespace |
| `\D` | Non-digit |
| `\W` | Non-word |
| `\S` | Non-whitespace |
| `(...)` | Capture group |
| `|` | Alternation |

### 12.4 Modifiers

| Modifier | Effect |
|----------|--------|
| `i` | Case insensitive |
| `g` | Global (for substitution) |
| `m` | Multiline |

### 12.5 Capture Functions

```strada
my array @captures = capture($string, $pattern);
```

---

## 13. Control Flow

### 13.1 Conditional Statements

**if/elsif/else:**
```strada
if (CONDITION) {
    BODY
} elsif (CONDITION) {
    BODY
} else {
    BODY
}
```

**switch/case:**
```strada
switch (EXPR) {
    case VALUE:
        BODY
        break;
    case VALUE:
    case VALUE:
        BODY
        break;
    default:
        BODY
}
```

### 13.2 Loops

**while:**
```strada
while (CONDITION) {
    BODY
}
```

**for:**
```strada
for (INIT; CONDITION; UPDATE) {
    BODY
}
```

**foreach:**
```strada
foreach my TYPE $var (LIST) {
    BODY
}
```

### 13.3 Loop Control

| Statement | Effect |
|-----------|--------|
| `last` | Exit innermost loop |
| `next` | Skip to next iteration |
| `last LABEL` | Exit labeled loop |
| `next LABEL` | Skip in labeled loop |

**Labels:**
```strada
OUTER: while (1) {
    INNER: while (1) {
        last OUTER;  # Exit both loops
    }
}
```

### 13.4 Goto

```strada
LABEL:
    # code
    goto LABEL;
```

---

## 14. Exception Handling

### 14.1 Try/Catch

```strada
try {
    # Code that may throw
} catch ($error) {
    # Handle error
}
```

### 14.2 Throw

```strada
throw "Error message";
throw $error_value;
```

### 14.3 Die

Terminates program with error:

```strada
die("Fatal error");
```

---

## 15. Packages and Modules

### 15.1 Package Declaration

```strada
package MyPackage;

# Package contents
```

### 15.2 Using Modules

```strada
use Module::Name;                    # Import all
use Module::Name qw(func1 func2);    # Import specific
```

### 15.3 Module Files

Module `Foo::Bar` is in file `lib/Foo/Bar.sm`.

### 15.4 Qualified Names

```strada
My::Module::function($arg);
$My::Module::variable;
```

---

## 16. Object-Oriented Programming

### 16.1 Blessed References

```strada
my scalar $obj = bless(\%hash, "ClassName");
```

### 16.2 Inheritance

```strada
package Child;
inherit Parent;

# Or for multiple inheritance
inherit Parent1, Parent2;

# Or at runtime
inherit("Child", "Parent");
```

### 16.3 OOP Functions

| Function | Description |
|----------|-------------|
| `bless($ref, $pkg)` | Associate ref with package |
| `blessed($obj)` | Get package name |
| `isa($obj, $pkg)` | Type check (with inheritance) |
| `can($obj, $method)` | Check method exists |
| `SUPER::method()` | Call parent method |

### 16.4 DESTROY

Destructor called when object is freed:

```strada
func ClassName_DESTROY(scalar $self) void {
    # Cleanup
    SUPER::DESTROY($self);
}
```

---

## 17. Closures

### 17.1 Syntax

```strada
my scalar $closure = func (PARAMS) { BODY };
my scalar $closure = func (PARAMS) TYPE { BODY };
```

### 17.2 Calling

```strada
$closure->(ARGS);
```

### 17.3 Capturing

Variables from enclosing scope are captured by reference:

```strada
my int $x = 10;
my scalar $f = func () { return $x; };
$x = 20;
say($f->());  # 20
```

---

## 18. Multithreading

### 18.1 Thread Functions

| Function | Description |
|----------|-------------|
| `thread::create($closure)` | Create and start thread |
| `thread::join($thread)` | Wait for completion |
| `thread::detach($thread)` | Detach thread |
| `thread::self()` | Get current thread ID |

### 18.2 Mutex Functions

| Function | Description |
|----------|-------------|
| `thread::mutex_new()` | Create mutex |
| `thread::mutex_lock($m)` | Lock mutex |
| `thread::mutex_trylock($m)` | Try to lock (non-blocking) |
| `thread::mutex_unlock($m)` | Unlock mutex |
| `thread::mutex_destroy($m)` | Destroy mutex |

### 18.3 Condition Variables

| Function | Description |
|----------|-------------|
| `thread::cond_new()` | Create condition |
| `thread::cond_wait($c, $m)` | Wait on condition |
| `thread::cond_signal($c)` | Signal one waiter |
| `thread::cond_broadcast($c)` | Signal all waiters |
| `thread::cond_destroy($c)` | Destroy condition |

---

## 19. Foreign Function Interface

### 19.1 Loading Libraries

```strada
my int $lib = sys::dl_open("libfoo.so");
my int $func = sys::dl_sym($lib, "function_name");
```

### 19.2 Calling Functions

```strada
my int $result = sys::dl_call_int($func, [$arg1, $arg2]);
my num $result = sys::dl_call_num($func, [$arg1]);
my str $result = sys::dl_call_str($func, $arg);
sys::dl_call_void($func, [$args]);
```

### 19.3 StradaValue Passthrough

For C functions expecting StradaValue*:

```strada
my int $result = sys::dl_call_int_sv($func, [$sv1, $sv2]);
```

---

## 20. Built-in Functions

### 20.1 Core Functions

**Output:**
- `say($val)` - Print with newline
- `print($val)` - Print without newline
- `printf($fmt, ...)` - Formatted print
- `warn($msg)` - Print to stderr
- `die($msg)` - Fatal error

**Type checking:**
- `defined($val)` - Check if defined
- `typeof($val)` - Get type name
- `is_ref($val)` - Check if reference
- `reftype($ref)` - Get reference type

**Debugging:**
- `dumper($val)` - Data::Dumper-style output

### 20.2 sys:: Namespace

**Files:**
- `sys::open($path, $mode)` - Open file
- `sys::close($fh)` - Close file
- `sys::readline($fh)` - Read line
- `sys::slurp($path)` - Read entire file
- `sys::spew($path, $data)` - Write file

**Process:**
- `sys::fork()` - Fork process
- `sys::wait()` - Wait for child
- `sys::exec($cmd)` - Replace process
- `sys::system($cmd)` - Run command
- `sys::getpid()` - Get process ID
- `sys::exit($code)` - Exit process

**Time:**
- `sys::time()` - Unix timestamp
- `sys::sleep($secs)` - Sleep seconds
- `sys::usleep($usecs)` - Sleep microseconds

**Environment:**
- `sys::getenv($name)` - Get environment variable
- `sys::setenv($name, $val)` - Set environment variable

### 20.3 math:: Namespace

- `math::sin($x)`, `math::cos($x)`, `math::tan($x)`
- `math::sqrt($x)`, `math::pow($x, $y)`
- `math::abs($x)`, `math::floor($x)`, `math::ceil($x)`
- `math::log($x)`, `math::exp($x)`
- `math::rand()` - Random 0.0-1.0

### 20.4 thread:: Namespace

See [Multithreading](#18-multithreading).

---

## 21. Magic Variables

| Variable | Description |
|----------|-------------|
| `@ARGV` | Command-line arguments |
| `$ARGC` | Argument count |
| `$_` | Default variable in map/grep |
| `$a`, `$b` | Sort comparison variables |
| `__PACKAGE__` | Current package name |
| `__FILE__` | Current file name |
| `__LINE__` | Current line number |

---

## 22. Reserved Words

The following words are reserved and cannot be used as identifiers:

```
array       break       case        catch       continue
default     delete      do          else        elsif
exists      extern      for         foreach     func
goto        hash        if          inherit     int
keys        last        local       map         my
next        num         our         package     pop
print       push        return      say         scalar
shift       sort        str         sub         switch
throw       try         undef       unshift     unless
until       use         values      void        while
```

---

## Appendix A: Grammar Summary

```
program         := (package_decl | use_decl | inherit_decl | func_def | stmt)*

package_decl    := 'package' IDENT ';'
use_decl        := 'use' qualified_name ('qw(' IDENT* ')')? ';'
inherit_decl    := 'inherit' IDENT (',' IDENT)* ';'

func_def        := 'func' IDENT '(' params? ')' type block

params          := param (',' param)*
param           := type SIGIL IDENT ('=' expr)?

type            := 'int' | 'num' | 'str' | 'scalar' | 'array' | 'hash' | 'void'

block           := '{' stmt* '}'

stmt            := var_decl | if_stmt | while_stmt | for_stmt | foreach_stmt
                 | switch_stmt | try_stmt | return_stmt | last_stmt | next_stmt
                 | goto_stmt | label_stmt | expr_stmt | block

var_decl        := 'my' type SIGIL IDENT ('=' expr)? ';'

expr            := assignment | ternary | logical | comparison | arithmetic
                 | unary | postfix | primary
```

---

## Appendix B: Operator Quick Reference

| Category | Operators |
|----------|-----------|
| Arithmetic | `+ - * / % **` |
| String | `. x` |
| Comparison (num) | `== != < > <= >= <=>` |
| Comparison (str) | `eq ne lt gt le ge cmp` |
| Logical | `&& || !` |
| Bitwise | `& | ^ ~ << >>` |
| Assignment | `= += -= *= /= %= .= &= |=` |
| Reference | `\ -> @{} %{} ${}` |
| Regex | `=~ !~ s///` |
| Other | `? : .. ,` |
