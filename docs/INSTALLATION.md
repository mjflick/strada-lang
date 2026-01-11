# Strada Installation Guide

This guide covers building Strada from source on Unix-like systems (Linux, macOS).

## Prerequisites

### Required

- **GCC** (GNU Compiler Collection) version 7.0 or later
- **GNU Make** version 3.81 or later
- **Standard C library** with POSIX support

### Optional (for specific features)

- **pthreads** - For multithreading (usually included with glibc)
- **libdl** - For dynamic loading/FFI (usually included)
- **libm** - Math library (usually included)
- **OpenSSL** - For SSL/TLS support in networking
- **Perl 5** development headers - For Perl integration (`libperl-dev`)

### Checking Prerequisites

```bash
# Check GCC
gcc --version

# Check Make
make --version

# Check for required libraries
ldconfig -p | grep -E "(libdl|libm|libpthread)"
```

## Quick Install

```bash
# Clone the repository
git clone https://github.com/yourusername/strada.git
cd strada

# Build the compiler
make

# Verify it works
./strada -r examples/test_simple.strada
```

## Detailed Build Process

### Step 1: Build the Self-Hosting Compiler

The Makefile handles the full bootstrap process:

```bash
make
```

This does the following:
1. Compiles the bootstrap compiler (C code in `bootstrap/`)
2. Uses bootstrap to compile the self-hosting compiler (`compiler/*.strada`)
3. Produces `./stradac` - the Strada compiler

### Step 2: Verify the Build

```bash
# Run the test suite
make test

# Verify self-hosting (compiler compiles itself)
make test-selfhost

# Build all examples
make examples
```

### Step 3: Install (Optional)

For system-wide installation:

```bash
# Create installation directories
sudo mkdir -p /usr/local/bin
sudo mkdir -p /usr/local/lib/strada
sudo mkdir -p /usr/local/include/strada

# Install compiler and wrapper
sudo cp stradac /usr/local/bin/
sudo cp strada /usr/local/bin/
sudo chmod +x /usr/local/bin/strada

# Install runtime
sudo cp runtime/strada_runtime.c /usr/local/lib/strada/
sudo cp runtime/strada_runtime.h /usr/local/include/strada/

# Add to PATH (add to your .bashrc or .zshrc)
export PATH="/usr/local/bin:$PATH"
export STRADA_HOME="/usr/local/lib/strada"
```

## Build Targets

| Target | Description |
|--------|-------------|
| `make` | Build the self-hosting compiler |
| `make run PROG=name` | Compile and run `examples/name.strada` |
| `make test` | Run runtime tests |
| `make test-selfhost` | Verify compiler can compile itself |
| `make examples` | Build all example programs |
| `make clean` | Remove all build artifacts |
| `make help` | Show all available targets |

## Compilation Options

### Using the Wrapper Script

The `./strada` wrapper script provides a convenient interface:

```bash
# Compile to executable (default)
./strada program.strada          # Creates ./program

# Compile and run immediately
./strada -r program.strada

# Keep the generated C code
./strada -c program.strada       # Creates program.c and ./program

# Include debug symbols
./strada -g program.strada       # Adds -g to gcc

# Specify output name
./strada -o myapp program.strada
```

### Using stradac Directly

For more control:

```bash
# Compile Strada to C
./stradac program.strada program.c

# Compile C to executable
gcc -o program program.c runtime/strada_runtime.c -Iruntime -ldl -lm

# With threading support
gcc -o program program.c runtime/strada_runtime.c -Iruntime -ldl -lm -lpthread

# With debug symbols
gcc -g -o program program.c runtime/strada_runtime.c -Iruntime -ldl -lm

# For FFI with rdynamic
gcc -rdynamic -o program program.c runtime/strada_runtime.c -Iruntime -ldl -lm
```

## Directory Structure

After building:

```
strada/
├── stradac              # Self-hosting compiler executable
├── strada               # Wrapper script for easy compilation
├── runtime/
│   ├── strada_runtime.c # Runtime library (link with programs)
│   └── strada_runtime.h # Runtime header
├── compiler/
│   ├── AST.strada       # Compiler source (AST definitions)
│   ├── Lexer.strada     # Compiler source (tokenizer)
│   ├── Parser.strada    # Compiler source (parser)
│   ├── CodeGen.strada   # Compiler source (code generator)
│   └── Main.strada      # Compiler source (entry point)
├── bootstrap/
│   └── stradac          # Bootstrap compiler (frozen)
├── build/               # Build artifacts (generated)
├── examples/            # Example programs
├── lib/                 # Standard library modules
└── docs/                # Documentation
```

## Platform-Specific Notes

### Linux

Standard build should work. Ensure development packages are installed:

```bash
# Debian/Ubuntu
sudo apt-get install build-essential

# Fedora/RHEL
sudo dnf install gcc make

# Arch
sudo pacman -S base-devel
```

For Perl integration:
```bash
# Debian/Ubuntu
sudo apt-get install libperl-dev

# Fedora/RHEL
sudo dnf install perl-devel
```

### macOS

Install Xcode Command Line Tools:

```bash
xcode-select --install
```

Note: Use `libSystem` instead of explicit `-ldl` (it's included automatically).

### WSL (Windows Subsystem for Linux)

Follow Linux instructions. WSL2 recommended for best performance.

## Troubleshooting

### "gcc: command not found"

Install GCC:
```bash
# Ubuntu/Debian
sudo apt-get install gcc

# macOS
xcode-select --install
```

### "cannot find -ldl"

On some systems, libdl is part of libc:
```bash
# Try without -ldl
gcc -o program program.c runtime/strada_runtime.c -Iruntime -lm
```

### "strada_runtime.h: No such file"

Ensure you're including the runtime directory:
```bash
gcc -o program program.c runtime/strada_runtime.c -Iruntime -ldl -lm
```

### Bootstrap compiler fails

Try rebuilding from scratch:
```bash
make clean
cd bootstrap && make clean && make
cd .. && make
```

### Self-hosting verification fails

This may indicate a bug. Try:
```bash
# Use bootstrap compiler directly
./bootstrap/stradac program.strada output.c
gcc -o program output.c runtime/strada_runtime.c -Iruntime -ldl -lm
```

## Building Optional Components

### Perl 5 Integration

```bash
# Install Perl development files
sudo apt-get install libperl-dev  # Debian/Ubuntu

# Build the Perl FFI library
cd lib/perl5
make

# Test
cd ../..
./strada -r examples/test_perl5.strada
```

### SSL/TLS Support

```bash
# Install OpenSSL development files
sudo apt-get install libssl-dev  # Debian/Ubuntu

# Build the SSL library
cd lib/ssl
make

# Test
cd ../..
./strada -r examples/test_ssl.strada
```

## Verifying Installation

Run the test suite to verify everything works:

```bash
# Quick smoke test
./strada -r examples/test_simple.strada

# Full test suite
./t/run_tests.sh

# Expected output: All tests should pass
```

Create a test program:

```bash
cat > hello.strada << 'EOF'
func main() int {
    say("Strada is installed correctly!");
    return 0;
}
EOF

./strada -r hello.strada
rm hello.strada hello
```

## Updating

To update to the latest version:

```bash
git pull
make clean
make
make test
```

## Uninstalling

If you installed system-wide:

```bash
sudo rm /usr/local/bin/stradac
sudo rm /usr/local/bin/strada
sudo rm -rf /usr/local/lib/strada
sudo rm -rf /usr/local/include/strada
```

For local installation, simply delete the repository:

```bash
rm -rf strada/
```
