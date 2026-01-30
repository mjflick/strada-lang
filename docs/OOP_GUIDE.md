# Strada Object-Oriented Programming Guide

Strada supports Perl-style object-oriented programming with blessed references, inheritance (including multiple inheritance), and polymorphism.

## Table of Contents

1. [OOP Basics](#1-oop-basics)
2. [Creating Classes](#2-creating-classes)
3. [Constructors](#3-constructors)
4. [Methods](#4-methods)
5. [Inheritance](#5-inheritance)
6. [Multiple Inheritance](#6-multiple-inheritance)
7. [SUPER:: Calls](#7-super-calls)
8. [DESTROY Destructors](#8-destroy-destructors)
9. [Type Checking](#9-type-checking)
10. [UNIVERSAL Methods](#10-universal-methods)
11. [Design Patterns](#11-design-patterns)
12. [Best Practices](#12-best-practices)

---

## 1. OOP Basics

### The Perl OOP Model

Strada uses Perl's OOP model:
- Objects are blessed hash references
- Methods are regular functions with `$self` as first argument
- Inheritance uses `@ISA` equivalent (`inherit`)
- No enforced encapsulation (convention-based privacy)

### Key Concepts

| Concept | Strada Implementation |
|---------|----------------------|
| Class | Package with constructor and methods |
| Object | Blessed hash reference |
| Method | Function taking `$self` |
| Attribute | Hash key on `$self` |
| Inheritance | `inherit` statement/function |
| Type check | `isa()` function |

---

## 2. Creating Classes

### Basic Class Structure

```strada
package ClassName;

# Constructor
func ClassName_new(PARAMS) scalar {
    my hash %self = ();
    # Initialize attributes
    $self{"attr"} = $value;
    return bless(\%self, "ClassName");
}

# Methods
func ClassName_method(scalar $self, PARAMS) RETURN_TYPE {
    # Access attributes
    my scalar $attr = $self->{"attr"};
    # ...
}
```

### Complete Example

```strada
package Person;

func Person_new(str $name, int $age) scalar {
    my hash %self = ();
    $self{"name"} = $name;
    $self{"age"} = $age;
    $self{"created"} = sys::time();
    return bless(\%self, "Person");
}

func Person_get_name(scalar $self) str {
    return $self->{"name"};
}

func Person_get_age(scalar $self) int {
    return $self->{"age"};
}

func Person_set_age(scalar $self, int $age) void {
    if ($age >= 0) {
        $self->{"age"} = $age;
    }
}

func Person_greet(scalar $self) void {
    say("Hello, I'm " . $self->{"name"} .
        " and I'm " . $self->{"age"} . " years old.");
}

func Person_have_birthday(scalar $self) void {
    $self->{"age"} = $self->{"age"} + 1;
    say("Happy birthday! Now " . $self->{"age"});
}

package main;

func main() int {
    my scalar $alice = Person_new("Alice", 30);
    Person_greet($alice);
    Person_have_birthday($alice);

    return 0;
}
```

---

## 3. Constructors

### Basic Constructor

```strada
func ClassName_new() scalar {
    my hash %self = ();
    return bless(\%self, "ClassName");
}
```

### Constructor with Parameters

```strada
func Point_new(int $x, int $y) scalar {
    my hash %self = ();
    $self{"x"} = $x;
    $self{"y"} = $y;
    return bless(\%self, "Point");
}
```

### Constructor with Defaults

```strada
func Config_new(str $name, int $debug = 0, int $verbose = 0) scalar {
    my hash %self = ();
    $self{"name"} = $name;
    $self{"debug"} = $debug;
    $self{"verbose"} = $verbose;
    return bless(\%self, "Config");
}

# Usage
my scalar $cfg1 = Config_new("app");           # defaults
my scalar $cfg2 = Config_new("app", 1);        # debug on
my scalar $cfg3 = Config_new("app", 1, 1);     # both on
```

### Factory Constructor

```strada
func Shape_create(str $type, array @args) scalar {
    if ($type eq "circle") {
        return Circle_new($args[0]);
    }
    if ($type eq "rectangle") {
        return Rectangle_new($args[0], $args[1]);
    }
    return undef;
}
```

### Cloning

```strada
func Person_clone(scalar $self) scalar {
    my hash %new = ();
    $new{"name"} = $self->{"name"};
    $new{"age"} = $self->{"age"};
    return bless(\%new, blessed($self));
}
```

---

## 4. Methods

### Instance Methods

```strada
# Method with $self
func Counter_increment(scalar $self) void {
    $self->{"count"} = $self->{"count"} + 1;
}

# Method returning value
func Counter_get(scalar $self) int {
    return $self->{"count"};
}
```

### Accessors (Getters/Setters)

```strada
# Getter
func Person_name(scalar $self) str {
    return $self->{"name"};
}

# Setter
func Person_set_name(scalar $self, str $name) void {
    $self->{"name"} = $name;
}

# Combined getter/setter
func Person_age(scalar $self, int $new_age = -1) int {
    if ($new_age >= 0) {
        $self->{"age"} = $new_age;
    }
    return $self->{"age"};
}
```

### Method Chaining

```strada
func Builder_set_name(scalar $self, str $name) scalar {
    $self->{"name"} = $name;
    return $self;  # Return $self for chaining
}

func Builder_set_value(scalar $self, int $value) scalar {
    $self->{"value"} = $value;
    return $self;
}

func Builder_build(scalar $self) scalar {
    return {
        name => $self->{"name"},
        value => $self->{"value"}
    };
}

# Usage
my scalar $result = Builder_new()
    ->set_name("test")
    ->set_value(42)
    ->build();
```

Note: Method chaining requires using the arrow syntax on the return value.

### Private Methods (Convention)

```strada
# Private methods start with underscore
func Person__validate_age(scalar $self, int $age) int {
    if ($age < 0) {
        return 0;
    }
    if ($age > 150) {
        return 0;
    }
    return 1;
}

func Person_set_age(scalar $self, int $age) int {
    if (Person__validate_age($self, $age)) {
        $self->{"age"} = $age;
        return 1;
    }
    return 0;
}
```

### Calling Package Functions Without Repeating the Package Name

Use `::func()` to call functions in the current package without repeating the package name:

```strada
package Person;

func validate_age(int $age) int {
    if ($age < 0 || $age > 150) {
        return 0;
    }
    return 1;
}

func set_age(scalar $self, int $age) int {
    # Use :: to call validate_age in current package
    if (::validate_age($age)) {      # Resolves to Person_validate_age
        $self->{"age"} = $age;
        return 1;
    }
    return 0;
}

func complex_operation(scalar $self) void {
    ::helper1();                     # Person_helper1
    ::helper2($self);                # Person_helper2
    .::helper3();                    # Alternate syntax
    __PACKAGE__::helper4();          # Explicit form
}
```

Three equivalent syntaxes:
- `::func()` — Preferred shorthand
- `.::func()` — Alternate shorthand
- `__PACKAGE__::func()` — Explicit form

All resolve to `PackageName_func()` at **compile time**.

### Variadic Methods

Methods can accept variable numbers of arguments using the spread operator syntax:

```strada
package Calculator;

# Variadic method - sum all numbers
func Calculator_sum(scalar $self, int ...@nums) int {
    my int $total = 0;
    foreach my int $n (@nums) {
        $total = $total + $n;
    }
    return $total;
}

# Fixed params + variadic
func Calculator_sum_with_base(scalar $self, int $base, int ...@nums) int {
    my int $total = $base;
    foreach my int $n (@nums) {
        $total = $total + $n;
    }
    return $total;
}

# Usage
my scalar $calc = Calculator::new();
$calc->sum(1, 2, 3);                    # 6
$calc->sum(10, 20, 30, 40, 50);         # 150

# Using spread operator
my array @values = (100, 200, 300);
$calc->sum(...@values);                 # 600
$calc->sum(1, ...@values, 99);          # 700

# With fixed param
$calc->sum_with_base(1000, 1, 2, 3);    # 1006
$calc->sum_with_base(500, ...@values);  # 1100
```

---

## 5. Inheritance

### Single Inheritance

**File-level syntax (single-package files):**

```strada
package Dog;
inherit Animal;

func Dog_new(str $name) scalar {
    # ...
}
```

**Function syntax (multi-package files):**

```strada
package Dog;

func Dog_init() void {
    inherit("Dog", "Animal");
}

func Dog_new(str $name) scalar {
    my hash %self = ();
    $self{"name"} = $name;
    $self{"species"} = "dog";
    return bless(\%self, "Dog");
}
```

### Complete Inheritance Example

```strada
package Animal;

func Animal_new(str $name, str $species) scalar {
    my hash %self = ();
    $self{"name"} = $name;
    $self{"species"} = $species;
    return bless(\%self, "Animal");
}

func Animal_speak(scalar $self) void {
    say($self->{"name"} . " makes a sound");
}

func Animal_get_name(scalar $self) str {
    return $self->{"name"};
}

package Dog;

func Dog_init() void {
    inherit("Dog", "Animal");
}

func Dog_new(str $name) scalar {
    my hash %self = ();
    $self{"name"} = $name;
    $self{"species"} = "dog";
    $self{"tricks"} = 0;
    return bless(\%self, "Dog");
}

# Override Animal_speak
func Dog_speak(scalar $self) void {
    say($self->{"name"} . " says: Woof!");
}

# New method
func Dog_learn_trick(scalar $self) void {
    $self->{"tricks"} = $self->{"tricks"} + 1;
    say($self->{"name"} . " knows " . $self->{"tricks"} . " tricks!");
}

package main;

func main() int {
    Dog_init();  # Set up inheritance

    my scalar $dog = Dog_new("Rex");

    # Inherited method
    say("Name: " . Animal_get_name($dog));

    # Overridden method
    Dog_speak($dog);

    # New method
    Dog_learn_trick($dog);

    # Type checks
    say("isa Dog: " . isa($dog, "Dog"));
    say("isa Animal: " . isa($dog, "Animal"));

    return 0;
}
```

---

## 6. Multiple Inheritance

### Syntax

**File-level (comma-separated):**

```strada
package Duck;
inherit Animal, Flyable, Swimmable;
```

**Function syntax:**

```strada
func Duck_init() void {
    inherit("Duck", "Animal");
    inherit("Duck", "Flyable");
    inherit("Duck", "Swimmable");
}
```

### Complete Example

```strada
package Printable;

func Printable_print(scalar $self) void {
    say("[Printable] " . $self->{"content"});
}

package Saveable;

func Saveable_save(scalar $self, str $filename) void {
    sys::spew($filename, $self->{"content"});
    say("Saved to " . $filename);
}

func Saveable_load(scalar $self, str $filename) void {
    $self->{"content"} = sys::slurp($filename);
    say("Loaded from " . $filename);
}

package Serializable;

func Serializable_serialize(scalar $self) str {
    return "CONTENT:" . $self->{"content"};
}

func Serializable_deserialize(scalar $self, str $data) void {
    if ($data =~ /^CONTENT:(.*)/) {
        my array @parts = capture($data, "^CONTENT:(.*)");
        $self->{"content"} = $parts[0];
    }
}

package Document;

func Document_init() void {
    inherit("Document", "Printable");
    inherit("Document", "Saveable");
    inherit("Document", "Serializable");
}

func Document_new(str $content) scalar {
    my hash %self = ();
    $self{"content"} = $content;
    $self{"created"} = sys::time();
    return bless(\%self, "Document");
}

func Document_set_content(scalar $self, str $content) void {
    $self->{"content"} = $content;
}

package main;

func main() int {
    Document_init();

    my scalar $doc = Document_new("Hello, World!");

    # Check all inheritances
    say("isa Printable: " . isa($doc, "Printable"));
    say("isa Saveable: " . isa($doc, "Saveable"));
    say("isa Serializable: " . isa($doc, "Serializable"));

    # Use methods from different parents
    Printable_print($doc);
    say("Serialized: " . Serializable_serialize($doc));

    return 0;
}
```

### Method Resolution Order (MRO)

When multiple parents have the same method, the first parent in the inheritance list takes precedence:

```strada
package A;
func A_method() str { return "A"; }

package B;
func B_method() str { return "B"; }

package C;
func C_init() void {
    inherit("C", "A");  # A comes first
    inherit("C", "B");
}

# When looking up 'method' on C:
# 1. Check C_method - not found
# 2. Check A_method - found, use A's implementation
# 3. B_method would only be checked if A didn't have it
```

---

## 7. SUPER:: Calls

### Calling Parent Methods

```strada
package Animal;

func Animal_speak(scalar $self) void {
    say($self->{"name"} . " makes a sound");
}

package Dog;

func Dog_init() void {
    inherit("Dog", "Animal");
}

func Dog_speak(scalar $self) void {
    # Call parent's speak first
    SUPER::speak($self);
    # Then add our own behavior
    say($self->{"name"} . " says: Woof!");
}
```

### SUPER in Constructors

```strada
package Employee;

func Employee_init() void {
    inherit("Employee", "Person");
}

func Employee_new(str $name, int $age, str $title) scalar {
    # Create base object
    my hash %self = ();
    $self{"name"} = $name;
    $self{"age"} = $age;
    $self{"title"} = $title;
    return bless(\%self, "Employee");
}

# Or call parent constructor-like initialization
func Employee_init_from_person(scalar $person, str $title) scalar {
    my hash %self = ();
    $self{"name"} = $person->{"name"};
    $self{"age"} = $person->{"age"};
    $self{"title"} = $title;
    return bless(\%self, "Employee");
}
```

### SUPER with Multiple Inheritance

With multiple inheritance, SUPER:: calls the first parent that has the method:

```strada
package Duck;

func Duck_init() void {
    inherit("Duck", "Bird");    # First parent
    inherit("Duck", "Swimmer"); # Second parent
}

func Duck_move(scalar $self) void {
    # Calls Bird_move if it exists, otherwise Swimmer_move
    SUPER::move($self);
    say("Duck is moving");
}
```

---

## 8. DESTROY Destructors

### Basic Destructor

```strada
package FileHandle;

func FileHandle_new(str $filename) scalar {
    my hash %self = ();
    $self{"filename"} = $filename;
    $self{"handle"} = sys::open($filename, "r");
    return bless(\%self, "FileHandle");
}

func FileHandle_DESTROY(scalar $self) void {
    if (defined($self->{"handle"})) {
        sys::close($self->{"handle"});
        say("Closed file: " . $self->{"filename"});
    }
}
```

### Destructor Chain

```strada
package Animal;

func Animal_DESTROY(scalar $self) void {
    say("Animal " . $self->{"name"} . " being destroyed");
}

package Dog;

func Dog_init() void {
    inherit("Dog", "Animal");
}

func Dog_DESTROY(scalar $self) void {
    say("Dog cleanup for " . $self->{"name"});
    # Call parent destructor
    SUPER::DESTROY($self);
}

package main;

func main() int {
    Dog_init();

    {
        my scalar $dog = Dog_new("Rex");
        # $dog goes out of scope here
    }
    # Output:
    #   Dog cleanup for Rex
    #   Animal Rex being destroyed

    return 0;
}
```

### Resource Management

```strada
package Connection;

func Connection_new(str $host, int $port) scalar {
    my hash %self = ();
    $self{"host"} = $host;
    $self{"port"} = $port;
    $self{"socket"} = sys::socket_client($host, $port);
    $self{"connected"} = defined($self->{"socket"});
    return bless(\%self, "Connection");
}

func Connection_send(scalar $self, str $data) int {
    if (!$self->{"connected"}) {
        return 0;
    }
    return sys::socket_send($self->{"socket"}, $data);
}

func Connection_DESTROY(scalar $self) void {
    if ($self->{"connected"}) {
        sys::socket_close($self->{"socket"});
        say("Connection to " . $self->{"host"} . " closed");
    }
}
```

---

## 9. Type Checking

### The isa() Function

```strada
my scalar $obj = Dog_new("Rex");

# Direct type check
if (isa($obj, "Dog")) {
    say("It's a Dog");
}

# Inheritance check
if (isa($obj, "Animal")) {
    say("It's an Animal (or subclass)");
}

# Negative check
if (!isa($obj, "Cat")) {
    say("It's not a Cat");
}
```

### The blessed() Function

```strada
my scalar $obj = Dog_new("Rex");

# Get package name
my str $pkg = blessed($obj);
say("Package: " . $pkg);  # "Dog"

# Check if blessed at all
if (length(blessed($obj)) > 0) {
    say("Object is blessed");
}
```

### The can() Function

```strada
my scalar $obj = Dog_new("Rex");

# Check if method exists
if (can($obj, "speak")) {
    say("Can speak");
}

if (!can($obj, "fly")) {
    say("Cannot fly");
}
```

### Type-Safe Method Dispatch

```strada
func feed(scalar $animal) void {
    if (!isa($animal, "Animal")) {
        die("Expected an Animal");
    }

    if (isa($animal, "Dog")) {
        say("Giving dog food");
    } elsif (isa($animal, "Cat")) {
        say("Giving cat food");
    } else {
        say("Giving generic food");
    }
}
```

---

## 10. UNIVERSAL Methods

Strada supports UNIVERSAL methods that work on any object:

### Using isa as a Method

```strada
my scalar $dog = Dog_new("Rex");

# Function style
if (isa($dog, "Animal")) { ... }

# Method style
if ($dog->isa("Animal")) { ... }
```

### Using can as a Method

```strada
my scalar $dog = Dog_new("Rex");

# Function style
if (can($dog, "speak")) { ... }

# Method style
if ($dog->can("speak")) { ... }
```

---

## 11. Design Patterns

### Singleton

```strada
package Logger;

my scalar $instance = undef;

func Logger_new() scalar {
    if (defined($instance)) {
        return $instance;
    }

    my hash %self = ();
    $self{"entries"} = [];
    $instance = bless(\%self, "Logger");
    return $instance;
}

func Logger_log(scalar $self, str $message) void {
    push(@{$self->{"entries"}}, $message);
    say("[LOG] " . $message);
}

func Logger_get_entries(scalar $self) array {
    return @{$self->{"entries"}};
}
```

### Factory

```strada
package ShapeFactory;

func ShapeFactory_create(str $type, array @args) scalar {
    switch ($type) {
        case "circle" {
            return Circle_new($args[0]);
        }
        case "rectangle" {
            return Rectangle_new($args[0], $args[1]);
        }
        case "triangle" {
            return Triangle_new($args[0], $args[1], $args[2]);
        }
        default {
            die("Unknown shape: " . $type);
        }
    }
}
```

### Observer

```strada
package Observable;

func Observable_new() scalar {
    my hash %self = ();
    $self{"observers"} = [];
    return bless(\%self, "Observable");
}

func Observable_add_observer(scalar $self, scalar $observer) void {
    push(@{$self->{"observers"}}, $observer);
}

func Observable_notify(scalar $self, str $event) void {
    foreach my scalar $obs (@{$self->{"observers"}}) {
        Observer_update($obs, $self, $event);
    }
}

package Observer;

func Observer_new(str $name) scalar {
    my hash %self = ();
    $self{"name"} = $name;
    return bless(\%self, "Observer");
}

func Observer_update(scalar $self, scalar $source, str $event) void {
    say($self->{"name"} . " received: " . $event);
}
```

### Strategy

```strada
package SortStrategy;

func SortStrategy_new(scalar $comparator) scalar {
    my hash %self = ();
    $self{"compare"} = $comparator;
    return bless(\%self, "SortStrategy");
}

func SortStrategy_sort(scalar $self, array @items) array {
    my scalar $cmp = $self->{"compare"};
    return @{sort { $cmp->($a, $b); } @items};
}

# Usage
my scalar $ascending = SortStrategy_new(func ($a, $b) {
    return $a <=> $b;
});

my scalar $descending = SortStrategy_new(func ($a, $b) {
    return $b <=> $a;
});

my array @nums = (3, 1, 4, 1, 5, 9);
my array @asc = SortStrategy_sort($ascending, @nums);
my array @desc = SortStrategy_sort($descending, @nums);
```

---

## 12. Best Practices

### 1. Use Consistent Naming

```strada
# Package names: CamelCase
package MyClass;

# Constructor: ClassName_new
func MyClass_new() scalar { ... }

# Methods: ClassName_method_name
func MyClass_get_value(scalar $self) int { ... }

# Private: ClassName__private_method (double underscore)
func MyClass__helper(scalar $self) void { ... }
```

### 2. Initialize All Attributes

```strada
func Person_new(str $name) scalar {
    my hash %self = ();

    # Always initialize all attributes
    $self{"name"} = $name;
    $self{"age"} = 0;
    $self{"email"} = "";
    $self{"created"} = sys::time();

    return bless(\%self, "Person");
}
```

### 3. Validate in Setters

```strada
func Person_set_age(scalar $self, int $age) int {
    if ($age < 0 || $age > 150) {
        return 0;  # Failure
    }
    $self->{"age"} = $age;
    return 1;  # Success
}
```

### 4. Use Inheritance Initialization

```strada
# For multi-package files, always use init functions
func Dog_init() void {
    inherit("Dog", "Animal");
}

func Cat_init() void {
    inherit("Cat", "Animal");
}

# Call init before creating objects
func main() int {
    Dog_init();
    Cat_init();

    my scalar $dog = Dog_new("Rex");
    # ...
}
```

### 5. Chain DESTROY Properly

```strada
func Child_DESTROY(scalar $self) void {
    # Clean up child-specific resources first
    if (defined($self->{"resource"})) {
        cleanup_resource($self->{"resource"});
    }

    # Then call parent destructor
    SUPER::DESTROY($self);
}
```

### 6. Document Public Interface

```strada
# Person class - represents a person with name and age
#
# Constructor:
#   Person_new(str $name, int $age) -> scalar
#
# Methods:
#   Person_get_name(scalar $self) -> str
#   Person_set_age(scalar $self, int $age) -> int (returns 1 on success)
#   Person_greet(scalar $self) -> void
```

### 7. Use Type Checking for Safety

```strada
func process_animals(array @animals) void {
    foreach my scalar $animal (@animals) {
        if (!isa($animal, "Animal")) {
            die("Expected Animal, got " . blessed($animal));
        }

        # Now safe to call Animal methods
        Animal_speak($animal);
    }
}
```

---

## Summary

| Feature | Syntax |
|---------|--------|
| Define class | `package ClassName;` |
| Constructor | `func ClassName_new() scalar { ... bless(\%self, "ClassName") }` |
| Method | `func ClassName_method(scalar $self) TYPE { ... }` |
| Inheritance | `inherit Parent;` or `inherit("Child", "Parent");` |
| Multiple inheritance | `inherit A, B, C;` |
| SUPER call | `SUPER::method($self, @args)` |
| Destructor | `func ClassName_DESTROY(scalar $self) void { ... }` |
| Type check | `isa($obj, "ClassName")` |
| Get package | `blessed($obj)` |
| Method check | `can($obj, "method")` |

---

## See Also

- [Tutorial](TUTORIAL.md) - Basic OOP tutorial
- [Language Manual](LANGUAGE_MANUAL.md) - Complete reference
- [Examples](EXAMPLES.md) - OOP examples
