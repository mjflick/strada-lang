# File Handles in Strada

Strada provides Perl-like file handle support for reading and writing files and sockets. This guide covers all aspects of file handle I/O.

## Overview

Strada has two types of I/O handles:

| Type | Internal | Use Case |
|------|----------|----------|
| `STRADA_FILEHANDLE` | `FILE*` | Regular files |
| `STRADA_SOCKET` | Buffered socket | Network connections |

Both types support the same high-level operations: diamond operator `<$fh>` for reading and `say()`/`print()` for writing.

## Opening Files

```strada
# Open for reading
my scalar $fh = sys::open("input.txt", "r");
if (!defined($fh)) {
    die("Cannot open file");
}

# Open for writing (creates/truncates)
my scalar $out = sys::open("output.txt", "w");

# Open for appending
my scalar $log = sys::open("app.log", "a");

# Open for read/write
my scalar $rw = sys::open("data.txt", "r+");
```

### File Modes

| Mode | Description |
|------|-------------|
| `"r"` | Read only (file must exist) |
| `"w"` | Write only (creates/truncates) |
| `"a"` | Append (creates if needed) |
| `"r+"` | Read and write (file must exist) |
| `"w+"` | Read and write (creates/truncates) |
| `"a+"` | Read and append |

## Reading from Files

### Single Line (Scalar Context)

The diamond operator `<$fh>` reads one line at a time:

```strada
my scalar $fh = sys::open("input.txt", "r");
my str $line = <$fh>;

while (defined($line)) {
    say("Got: " . $line);
    $line = <$fh>;
}

sys::close($fh);
```

**Note:** The newline is automatically stripped from each line.

### All Lines (Array Context)

When assigned to an array, the diamond operator reads ALL lines:

```strada
my scalar $fh = sys::open("input.txt", "r");
my array @lines = <$fh>;  # Reads entire file!
sys::close($fh);

say("Read " . scalar(@lines) . " lines");

foreach my str $line (@lines) {
    say($line);
}
```

This also works with assignment to existing arrays:

```strada
my array @data = ();
my scalar $fh = sys::open("file.txt", "r");
@data = <$fh>;  # Replaces contents with all lines
sys::close($fh);
```

### Entire File as String

Use `sys::slurp()` to read the entire file as a single string:

```strada
my str $content = sys::slurp("file.txt");
```

Or from an open filehandle:

```strada
my scalar $fh = sys::open("file.txt", "r");
my str $content = sys::slurp_fh($fh);
sys::close($fh);
```

## Writing to Files

### Using say() and print()

The `say()` and `print()` functions accept a filehandle as the first argument:

```strada
my scalar $fh = sys::open("output.txt", "w");

say($fh, "This line has a newline");     # Adds \n
print($fh, "No newline here");           # No \n
print($fh, " - continued\n");            # Manual \n

sys::close($fh);
```

### Using sys::spew()

Write an entire string to a file:

```strada
sys::spew("output.txt", "File contents here");
```

Or to an open filehandle:

```strada
my scalar $fh = sys::open("output.txt", "w");
sys::spew_fh($fh, "Content");
sys::close($fh);
```

## Closing Files

Always close files when done:

```strada
sys::close($fh);
```

Files are also closed when the filehandle goes out of scope (reference counting), but explicit closing is recommended.

## Socket I/O

Sockets work identically to files with the diamond operator and say/print:

```strada
# Connect to server
my scalar $sock = sys::socket_client("example.com", 80);

# Send HTTP request
say($sock, "GET / HTTP/1.0");
say($sock, "Host: example.com");
say($sock, "");  # Empty line ends headers

# Read response line by line
my str $line = <$sock>;
while (defined($line)) {
    say($line);
    $line = <$sock>;
}

sys::socket_close($sock);
```

### Reading All Lines from Socket

```strada
my scalar $sock = sys::socket_client("example.com", 80);
say($sock, "GET / HTTP/1.0");
say($sock, "Host: example.com");
say($sock, "");

# Read entire response
my array @response = <$sock>;
sys::socket_close($sock);

foreach my str $line (@response) {
    say($line);
}
```

## Buffering

### File Buffering

Files use stdio's internal buffering (`FILE*`):
- Typically 4KB-8KB buffer managed by the C library
- Very efficient for sequential access
- Use `sys::flush($fh)` to force write

### Socket Buffering

Sockets use Strada's custom 8KB buffers:

```
StradaSocketBuffer:
  - read_buf[8192]   : Input buffer
  - read_pos         : Current read position
  - read_len         : Amount of data in buffer
  - write_buf[8192]  : Output buffer
  - write_len        : Amount buffered for writing
```

**Read behavior:**
- `recv()` fills the 8KB buffer
- Lines are extracted from the buffer
- Leftover data stays for next read

**Write behavior:**
- Data is buffered until newline or buffer full
- `say()` always flushes (line-buffered)
- `print()` flushes only if data ends with `\n`
- Use `sys::socket_flush($sock)` for explicit flush

### CRLF Handling

Socket reads automatically strip `\r` (carriage return), so CRLF line endings from network protocols are handled correctly:

```strada
# Server sends: "HTTP/1.1 200 OK\r\n"
my str $line = <$sock>;
# $line contains: "HTTP/1.1 200 OK" (no \r\n)
```

## Server Example

```strada
func handle_client(scalar $client) void {
    say($client, "Welcome to the server!");

    my str $line = <$client>;
    while (defined($line)) {
        if ($line eq "quit") {
            say($client, "Goodbye!");
            last;
        }
        say($client, "Echo: " . $line);
        $line = <$client>;
    }

    sys::socket_close($client);
}

func main() int {
    my scalar $server = sys::socket_server(8080);
    say("Server listening on port 8080");

    while (1) {
        my scalar $client = sys::socket_accept($server);
        if (defined($client)) {
            handle_client($client);
        }
    }

    return 0;
}
```

## File Position

```strada
my scalar $fh = sys::open("file.txt", "r+");

# Get current position
my int $pos = sys::tell($fh);

# Seek to position
sys::seek($fh, 0, 0);   # Beginning (SEEK_SET)
sys::seek($fh, 10, 1);  # Forward 10 bytes (SEEK_CUR)
sys::seek($fh, -5, 2);  # 5 bytes from end (SEEK_END)

# Rewind to beginning
sys::rewind($fh);

# Check for end of file
if (sys::eof($fh)) {
    say("At end of file");
}

sys::close($fh);
```

## Low-Level I/O

For advanced use cases, low-level file descriptor operations are available:

```strada
# Open with file descriptor
my int $fd = sys::open_fd("file.txt", 0);  # O_RDONLY

# Read raw bytes
my str $data = sys::read_fd($fd, 1024);

# Write raw bytes
sys::write_fd($fd, "data");

# Close
sys::close_fd($fd);
```

## Best Practices

1. **Always close files** - Don't rely on garbage collection
2. **Check for errors** - Test `defined($fh)` after opening
3. **Use array context for small files** - `my array @lines = <$fh>` is convenient
4. **Use scalar context for large files** - Process line by line to save memory
5. **Flush sockets when needed** - Use `sys::socket_flush()` for non-line protocols
6. **Use say() for line-oriented I/O** - It handles newlines and flushing

## Summary

| Operation | Syntax |
|-----------|--------|
| Open file | `sys::open($path, $mode)` |
| Close file | `sys::close($fh)` |
| Read line | `my str $line = <$fh>` |
| Read all lines | `my array @lines = <$fh>` |
| Read entire file | `sys::slurp($path)` |
| Write with newline | `say($fh, $text)` |
| Write without newline | `print($fh, $text)` |
| Write entire file | `sys::spew($path, $content)` |
| Flush buffer | `sys::flush($fh)` or `sys::socket_flush($sock)` |
| Socket connect | `sys::socket_client($host, $port)` |
| Socket server | `sys::socket_server($port)` |
| Socket accept | `sys::socket_accept($server)` |
| Socket close | `sys::socket_close($sock)` |
