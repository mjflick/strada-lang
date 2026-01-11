use strict;
use warnings;
use ExtUtils::MakeMaker;

# Path to Strada runtime
my $strada_root = '../..';
my $runtime_inc = "$strada_root/runtime";
my $runtime_src = "$strada_root/runtime/strada_runtime.c";

# Check for runtime header
unless (-f "$runtime_inc/strada_runtime.h") {
    die "Cannot find strada_runtime.h in $runtime_inc\n" .
        "Please ensure you're building from the perl/Strada directory\n";
}

WriteMakefile(
    NAME         => 'Strada',
    VERSION_FROM => 'Strada.pm',
    ABSTRACT     => 'Call compiled Strada shared libraries from Perl',
    AUTHOR       => 'Strada Project',
    LICENSE      => 'perl_5',

    # Include paths
    INC          => "-I$runtime_inc",

    # Include Strada runtime in OBJECT list
    OBJECT       => '$(O_FILES) strada_runtime$(OBJ_EXT)',
    LIBS         => ['-ldl -lm'],

    # Compiler flags
    CCFLAGS      => '-fPIC',

    # Extra files to clean
    clean        => { FILES => '*.o strada_runtime.o' },

    # Minimum Perl version
    MIN_PERL_VERSION => '5.010',

    META_MERGE   => {
        'meta-spec' => { version => 2 },
        resources   => {
            repository => {
                type => 'git',
                url  => 'https://github.com/strada-lang/strada.git',
                web  => 'https://github.com/strada-lang/strada',
            },
        },
    },
);

# Custom target to build Strada runtime object
sub MY::postamble {
    return <<"MAKE";

# Build Strada runtime object
strada_runtime\$(OBJ_EXT): $runtime_src
\t\$(CC) \$(CCFLAGS) \$(OPTIMIZE) -fPIC -I$runtime_inc -c $runtime_src -o \$@

MAKE
}
