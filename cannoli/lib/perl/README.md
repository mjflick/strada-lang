# Cannoli Perl Dispatch Library

Dispatch HTTP requests to Perl subroutines from Cannoli.

## Architecture

This is a hybrid Strada/C implementation:
- **cannoli_perl.strada** - Main logic (config parsing, initialization) written in Strada
- **cannoli_dispatch.c** - Thin C wrapper for dispatch return type conversion
- **perl_bridge.c** - Minimal C bridge for Perl embedding API

## Building

```bash
cd cannoli/lib/perl
make
```

Requires: `libperl-dev` (Debian/Ubuntu) or equivalent Perl development headers.

## Usage

```bash
# Basic usage with a script
cannoli --library ./lib/perl/cannoli_perl.so:script=/path/to/app.pl;handler=MyApp::handle --dev

# With lib paths
cannoli --library ./lib/perl/cannoli_perl.so:script=app.pl;handler=MyApp::handle;lib=./lib;lib=./vendor --dev

# With modules
cannoli --library ./lib/perl/cannoli_perl.so:use=MyApp;handler=MyApp::dispatch --dev
```

## Configuration Options

Options are passed after the colon, separated by semicolons:

| Option | Description |
|--------|-------------|
| `handler=Sub::Name` | Perl subroutine to call for each request (required) |
| `script=/path/to.pl` | Perl script to load on startup |
| `lib=/path` | Add path to @INC (can be repeated) |
| `use=Module::Name` | Use a Perl module on startup |

## Handler Signature

Your Perl handler receives four arguments:

```perl
sub handle {
    my ($method, $path, $path_info, $body) = @_;

    # Return response
    return '{"status":"ok"}';  # JSON auto-detected
}
```

## Return Values

| Return | Result |
|--------|--------|
| Plain string | Auto-detects JSON/HTML, returns 200 OK |
| `STATUS:code:content` | Custom status code with content |
| `REDIRECT:url` | 302 redirect to URL |
| Empty string | 404 Not Found |

## Example Handler

```perl
# myapp.pl
package MyApp;

use strict;
use warnings;
use JSON;

sub handle {
    my ($method, $path, $path_info, $body) = @_;

    if ($path eq '/') {
        return encode_json({ message => 'Hello from Perl!' });
    }
    elsif ($path eq '/hello') {
        return 'Hello, World!';
    }
    elsif ($path =~ m{^/api/(.+)}) {
        my $resource = $1;
        return encode_json({ resource => $resource, method => $method });
    }
    elsif ($method eq 'POST' && $path eq '/submit') {
        my $data = decode_json($body);
        return encode_json({ received => $data });
    }

    return '';  # 404
}

1;
```

Run with:
```bash
cannoli --library ./cannoli_perl.so:script=myapp.pl;handler=MyApp::handle --dev -p 8080
```

## XS Modules

To use XS modules (like JSON::XS, DBI, etc.), you may need to preload libperl:

```bash
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libperl.so.5.38 cannoli --library ...
```

## Files

- `cannoli_perl.strada` - Main initialization logic in Strada
- `cannoli_dispatch.c` - C wrapper for dispatch (handles return type)
- `perl_bridge.c` - C bridge for Perl embedding API
- `Makefile` - Build configuration
