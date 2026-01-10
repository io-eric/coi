# Language Guide

Coi is a statically-typed language. This guide covers the core language features.

## Types

### Primitive Types

```tsx
int count = 42;          // Integer (32-bit signed)
float speed = 3.14;      // Floating point (32-bit)
string name = "Coi";     // String
bool active = true;      // Boolean
```

### Arrays

```tsx
// Array literals
int[] scores = [10, 20, 30]; 
string[] tags = ["web", "fast"];
mut float[] prices = [];     // Mutable empty array

// Fixed-size arrays with repeat initializer [value; count]
int[] zeros = [0; 100];      // Array of 100 zeros
int[] tens = [10; 5];        // [10, 10, 10, 10, 10]
float[] coords = [0.0; 3];   // [0.0, 0.0, 0.0]
```

### Array Methods

```tsx
int len = scores.size();     // Get length
scores.push(40);             // Add element (requires mut)
scores.pop();                // Remove last element (requires mut)
scores.clear();              // Remove all elements (requires mut)
bool empty = scores.isEmpty();

// Index access
int first = scores[0];
scores[0] = 100;             // Requires mut
```

## Enums

Enums define a set of named constants. They can be declared inside components, as shared (accessible from other components), or globally.

### Basic Enums

```tsx
// Inside a component
component App {
    enum Mode {
        Idle,
        Running,
        Paused
    }
    
    mut Mode currentMode = Mode::Idle;
    
    def start() : void {
        currentMode = Mode::Running;
    }
}

// Global enum (outside any component)
enum Status {
    Pending,
    Success,
    Error
}

component Handler {
    mut Status status = Status::Pending;
}
```

### Shared Enums

Shared enums can be accessed from other components using the `ComponentName.EnumName::Value` syntax:

```tsx
component Theme {
    shared enum Mode {
        Light,
        Dark
    }
    
    mut Mode current = Mode::Light;
}

component Settings {
    // Access shared enum from another component
    mut Theme.Mode selectedTheme = Theme.Mode::Dark;
}
```

### Enum Features

- Enums implicitly convert to/from `int` for easy serialization
- Comparison with `==` and `!=` works as expected
- Trailing commas are allowed in enum value lists
- `.size()` returns the number of enum values

```tsx
// Implicit int conversion
mut Mode mode = Mode::Idle;
int modeValue = mode;        // enum -> int
mode = 2;                    // int -> enum

// Get enum size (useful for iteration)
int count = Mode.size();     // returns 3 (Idle, Running, Paused)

// Comparison
if (mode == Mode::Running) {
    // ...
}
```

## Control Flow

### Conditionals

```tsx
if (x > 10) {
    message = "big";
} else {
    message = "small";
}
```

### Ternary Operator

```tsx
// Basic ternary
int max = x > y ? x : y;

// Nested ternary
string sign = n > 0 ? "positive" : n < 0 ? "negative" : "zero";

// In expressions
string msg = count > 0 ? "has items" : "empty";
```

### Loops

Coi supports range-based loops and iterator-based foreach loops.

```tsx
// Range-based (start:end)
for i in 0:10 {
    sum += i;
}

// ForEach (Arrays)
for score in scores {
    total += score;
}
```

## Operators

### Arithmetic

```tsx
int sum = a + b;
int diff = a - b;
int prod = a * b;
int quot = a / b;
int mod = a % b;
```

### Comparison

```tsx
if (x == y) { }  // equal
if (x != y) { }  // not equal
if (x < y)  { }  // less than
if (x > y)  { }  // greater than
if (x <= y) { }  // less or equal
if (x >= y) { }  // greater or equal
```

### Logical

```tsx
if (a && b) { }  // and
if (a || b) { }  // or
if (!a) { }      // not
```

### Assignment

```tsx
count += 10;
count -= 5;
count *= 2;
count /= 2;
count++;
count--;
```

## Functions

Functions are defined with the `def` keyword:

```tsx
def add(int a, int b) : int {
    return a + b;
}

def greet(string name) : void {
    System.log("Hello, " + name);
}

// Public function (accessible from outside component)
pub def reset() : void {
    count = 0;
}
```

## Next Steps

- [Components](components.md) — Component syntax, lifecycle, props
- [View Syntax](view-syntax.md) — JSX-like templates, conditionals, loops
- [Styling](styling.md) — Scoped and global CSS
