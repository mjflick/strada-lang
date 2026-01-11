package Strada;

use strict;
use warnings;

our $VERSION = '0.01';

require XSLoader;
XSLoader::load('Strada', $VERSION);

=head1 NAME

Strada - Call compiled Strada shared libraries from Perl

=head1 SYNOPSIS

    use Strada;

    # Load a Strada shared library
    my $lib = Strada::Library->new('path/to/library.so');

    # Call a function with no arguments
    my $result = $lib->call('my_function');

    # Call a function with arguments
    my $result = $lib->call('add_numbers', 1, 2);

    # Unload the library
    $lib->unload();

=head1 DESCRIPTION

This module provides an interface for Perl programs to load and call
functions from compiled Strada shared libraries (.so files).

Strada values are automatically converted to/from Perl types:

    Strada int    <-> Perl integer
    Strada num    <-> Perl number
    Strada str    <-> Perl string
    Strada array  <-> Perl array reference
    Strada hash   <-> Perl hash reference
    Strada undef  <-> Perl undef

=head1 METHODS

=head2 Low-level API

=head3 Strada::load($path)

Load a shared library. Returns a handle (integer) or 0 on failure.

=head3 Strada::unload($handle)

Unload a previously loaded library.

=head3 Strada::get_func($handle, $name)

Get a function pointer from a loaded library.

=head3 Strada::call($func, @args)

Call a Strada function with arguments. Up to 4 arguments supported.

=head2 High-level API

=head3 Strada::Library->new($path)

Create a new Library object and load the shared library.

=head3 $lib->call($func_name, @args)

Call a function by name with arguments.

=head3 $lib->unload()

Unload the library.

=cut

# High-level OO interface
package Strada::Library;

sub new {
    my ($class, $path) = @_;

    my $handle = Strada::load($path);
    die "Failed to load Strada library: $path" unless $handle;

    return bless {
        handle => $handle,
        path   => $path,
        funcs  => {},  # Cache function pointers
    }, $class;
}

sub call {
    my ($self, $func_name, @args) = @_;

    # Get or cache function pointer
    my $func = $self->{funcs}{$func_name};
    unless ($func) {
        $func = Strada::get_func($self->{handle}, $func_name);
        die "Function not found: $func_name" unless $func;
        $self->{funcs}{$func_name} = $func;
    }

    return Strada::call($func, @args);
}

sub unload {
    my ($self) = @_;

    if ($self->{handle}) {
        Strada::unload($self->{handle});
        $self->{handle} = 0;
        $self->{funcs} = {};
    }
}

sub DESTROY {
    my ($self) = @_;
    $self->unload() if $self->{handle};
}

1;

__END__

=head1 EXAMPLE

Create a Strada library (math_lib.strada):

    package math_lib;

    func add(int $a, int $b) int {
        return $a + $b;
    }

    func multiply(int $a, int $b) int {
        return $a * $b;
    }

    func greet(str $name) str {
        return "Hello, " . $name . "!";
    }

Compile it as a shared library:

    ./stradac -shared math_lib.strada libmath.so

Use from Perl:

    use Strada;

    my $lib = Strada::Library->new('./libmath.so');

    print $lib->call('add', 2, 3), "\n";        # 5
    print $lib->call('multiply', 4, 5), "\n";   # 20
    print $lib->call('greet', 'Perl'), "\n";    # Hello, Perl!

    $lib->unload();

=head1 BUILDING

    cd perl/Strada
    perl Makefile.PL
    make
    make test
    make install

=head1 REQUIREMENTS

- Strada runtime headers (strada_runtime.h)
- Perl development headers

=head1 AUTHOR

Strada Project

=head1 LICENSE

Same as Strada.

=cut
