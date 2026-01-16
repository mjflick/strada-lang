# Regular Expressions in Strada

Strada provides Perl-style regular expression support through the `=~` and `!~` operators, regex substitution with `s///`, and several built-in functions for advanced regex operations.

## Basic Pattern Matching

Use the `=~` operator to test if a string matches a pattern:

```strada
my str $text = "Hello, World!";

if ($text =~ /World/) {
    say("Found World!");
}

# Negated match with !~
if ($text !~ /Goodbye/) {
    say("No Goodbye found");
}
```

## Pattern Syntax

Strada uses POSIX Extended Regular Expressions (ERE) with some Perl extensions.

### Anchors

| Pattern | Meaning |
|---------|---------|
| `^` | Start of string |
| `$` | End of string |
| `\b` | Word boundary |

```strada
# ^ matches start of string
if ("hello world" =~ /^hello/) { say("Starts with hello"); }

# $ matches end of string
if ("hello world" =~ /world$/) { say("Ends with world"); }

# Both anchors for exact match
if ("hello" =~ /^hello$/) { say("Exact match"); }

# IMPORTANT: $ in regex means end-of-line, NOT variable interpolation
# To use both a variable and $ anchor:
my str $suffix = "world";
if ("hello world" =~ /$suffix$/) { say("Ends with $suffix"); }
```

### Character Classes

| Pattern | Meaning |
|---------|---------|
| `.` | Any character except newline |
| `\d` | Digit (0-9) |
| `\D` | Non-digit |
| `\w` | Word character (a-z, A-Z, 0-9, _) |
| `\W` | Non-word character |
| `\s` | Whitespace (space, tab, newline) |
| `\S` | Non-whitespace |
| `[abc]` | Character class (a, b, or c) |
| `[^abc]` | Negated class (not a, b, or c) |
| `[a-z]` | Character range |

```strada
if ($text =~ /\d+/) { say("Contains digits"); }
if ($text =~ /[aeiou]/) { say("Contains a vowel"); }
if ($text =~ /[A-Z][a-z]+/) { say("Contains capitalized word"); }
```

### Quantifiers

| Pattern | Meaning |
|---------|---------|
| `*` | Zero or more |
| `+` | One or more |
| `?` | Zero or one |
| `{n}` | Exactly n times |
| `{n,}` | n or more times |
| `{n,m}` | Between n and m times |

```strada
if ($text =~ /a+/) { say("One or more a's"); }
if ($text =~ /\d{3}-\d{4}/) { say("Phone number format"); }
```

### Alternation and Grouping

| Pattern | Meaning |
|---------|---------|
| `\|` | Alternation (or) |
| `(...)` | Grouping and capture |
| `(?:...)` | Non-capturing group |

```strada
if ($text =~ /cat|dog/) { say("Found cat or dog"); }
if ($text =~ /(hello|hi) world/) { say("Greeting found"); }
```

## Flags/Modifiers

Flags are specified after the closing delimiter:

| Flag | Meaning |
|------|---------|
| `i` | Case-insensitive matching |
| `m` | Multi-line mode (^ and $ match line boundaries) |
| `s` | Single-line mode (. matches newline) |
| `g` | Global (for substitution - replace all) |

```strada
# Case-insensitive
if ($text =~ /hello/i) { say("Found hello (any case)"); }

# Multi-line
my str $lines = "line1\nline2";
if ($lines =~ /^line2/m) { say("Found line2 at start of line"); }
```

## Capturing Groups

Use parentheses to capture parts of the match. Access captures with the `captures()` function:

```strada
my str $date = "2024-01-15";

if ($date =~ /(\d{4})-(\d{2})-(\d{2})/) {
    my array @parts = captures();
    say("Full match: " . @parts[0]);   # 2024-01-15
    say("Year: " . @parts[1]);         # 2024
    say("Month: " . @parts[2]);        # 01
    say("Day: " . @parts[3]);          # 15
}
```

### Named Captures (Not Currently Supported)

Strada currently uses positional captures only. Named captures are not yet implemented.

## Substitution

Use `s///` for search and replace:

```strada
my str $text = "Hello World";

# Replace first occurrence
$text =~ s/World/Strada/;
say($text);  # Hello Strada

# Replace all occurrences with /g flag
my str $repeated = "cat cat cat";
$repeated =~ s/cat/dog/g;
say($repeated);  # dog dog dog
```

### Case Modification in Replacement

```strada
my str $text = "hello world";

# \U - uppercase, \L - lowercase, \E - end modification
$text =~ s/hello/\Uhello\E/;  # Not currently supported
# Use uc() function instead:
$text = uc(substr($text, 0, 5)) . substr($text, 5);
```

## Variable Interpolation in Patterns

Variables are interpolated in regex patterns:

```strada
my str $search = "world";
if ($text =~ /$search/) {
    say("Found: $search");
}

# Combine variable with $ anchor (end of line)
my str $suffix = "end";
if ($text =~ /$suffix$/) {
    say("Text ends with $suffix");
}
```

### Escaping Special Characters

To match literal special characters, escape them with backslash:

```strada
# Match literal dot
if ($filename =~ /\.txt$/) { say("Text file"); }

# Match literal dollar sign - use two backslashes in Strada string
if ($price =~ /\$\d+/) { say("Price found"); }

# Common metacharacters that need escaping: . * + ? [ ] { } ( ) | ^ $ \
```

## Built-in Functions

### match()

Simple pattern match (returns 1 or 0):

```strada
my int $found = match($text, "pattern");
```

### capture()

Get all capture groups as an array:

```strada
my array @groups = capture($text, "(\d+)-(\d+)");
```

### split()

Split string by pattern:

```strada
my array @words = split("\\s+", $text);  # Split on whitespace
my array @parts = split(",", $csv);       # Split on comma
```

**Note:** `split()` uses regex patterns. To split on literal special characters, escape them:

```strada
my array @parts = split("\\.", $ip_address);  # Split on literal dot
my array @items = split("\\|", $data);        # Split on literal pipe
```

### join()

Join array with separator (not regex, but commonly used with split):

```strada
my str $text = join(", ", @items);
```

## Common Patterns

### Email Validation

```strada
func is_valid_email(str $email) int {
    return match($email, "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
}
```

### Phone Number

```strada
func is_phone(str $phone) int {
    return match($phone, "^\\d{3}[-.]?\\d{3}[-.]?\\d{4}$");
}
```

### URL Extraction

```strada
func extract_urls(str $text) array {
    my array @urls = ();
    # This is simplified - real URL matching is more complex
    while ($text =~ /(https?:\/\/[^\s]+)/) {
        my array @cap = captures();
        push(@urls, @cap[1]);
        $text =~ s/https?:\/\/[^\s]+//;
    }
    return @urls;
}
```

### Whitespace Cleanup

```strada
# Trim leading/trailing whitespace
$text =~ s/^\s+//;
$text =~ s/\s+$//;

# Collapse multiple spaces to single
$text =~ s/\s+/ /g;
```

## Important Notes

### $ in Regex vs Variable Interpolation

The `$` character has two meanings:
1. **End-of-line anchor** when at end of pattern or not followed by a word character
2. **Variable interpolation** when followed by a variable name

```strada
# $ as anchor (end of line)
if ($text =~ /end$/) { ... }

# $ as variable interpolation
my str $var = "test";
if ($text =~ /$var/) { ... }

# Both together - variable followed by anchor
if ($text =~ /$var$/) { ... }  # Match $var at end of string
```

### POSIX vs Perl Regex

Strada uses POSIX Extended Regular Expressions. Some Perl features are not available:
- Lookahead/lookbehind (`(?=...)`, `(?!...)`, etc.)
- Named captures (`(?<name>...)`)
- Possessive quantifiers (`*+`, `++`)
- Unicode properties (`\p{...}`)

### Performance Considerations

- Regex patterns are compiled each time they're used
- For repeated matching with the same pattern, consider storing the pattern in a variable
- Complex patterns with many alternations can be slow
- Use anchors (`^`, `$`) when possible to limit search space

## Troubleshooting

### Pattern Not Matching

1. Check anchor usage - `^` and `$` match start/end of entire string by default
2. Verify escaping - special characters need backslash
3. Use `/i` flag for case-insensitive matching
4. Test pattern components separately

### Captures Not Working

1. Ensure pattern actually matched before calling `captures()`
2. Remember `@captures[0]` is the full match, groups start at index 1
3. Captures are cleared on each match - save them if needed

```strada
if ($text =~ /pattern/) {
    my array @saved = captures();  # Save immediately
    # ... use @saved later
}
```

### Special Characters in Replacement

In substitution replacements, most characters are literal. Use function calls for case changes:

```strada
# Instead of \U for uppercase:
my str $word = "hello";
$word =~ s/hello/HELLO/;  # Literal replacement
# Or:
$word = uc($word);        # Using uc() function
```
