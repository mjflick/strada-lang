# Makefile for cannoli Perl dispatch library (Strada + C hybrid)
#
# This builds:
#   1. libperl_bridge.so - Minimal C bridge for Perl API
#   2. cannoli_perl.so   - Main dispatch logic (Strada + C wrapper)

# Paths
STRADA_ROOT = ../../..
STRADA_RUNTIME = $(STRADA_ROOT)/runtime
STRADAC = $(STRADA_ROOT)/stradac

# Get Perl compile flags
PERL_CCOPTS := $(shell perl -MExtUtils::Embed -e ccopts 2>/dev/null)
PERL_LDOPTS := $(shell perl -MExtUtils::Embed -e ldopts 2>/dev/null)

CC = gcc
CFLAGS = -Wall -Wextra -fPIC -O2 -I$(STRADA_RUNTIME) $(PERL_CCOPTS)
LDFLAGS_BRIDGE = -shared $(PERL_LDOPTS)
LDFLAGS_STRADA = -shared -L$(STRADA_RUNTIME) -rdynamic

# Targets
BRIDGE_TARGET = libperl_bridge.so
STRADA_TARGET = cannoli_perl.so

.PHONY: all clean test check-perl check-strada

all: check-perl check-strada $(BRIDGE_TARGET) $(STRADA_TARGET)

check-perl:
	@perl -MExtUtils::Embed -e 'print "Perl: $$^V\n"' 2>/dev/null || \
		(echo "Error: Perl development headers not found. Install libperl-dev"; exit 1)

check-strada:
	@test -f $(STRADAC) || (echo "Error: Strada compiler not found at $(STRADAC). Run 'make' in root first."; exit 1)

# Build the C bridge for Perl API
$(BRIDGE_TARGET): perl_bridge.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS_BRIDGE)
	@echo "Built: $(BRIDGE_TARGET)"

# Compile Strada to C
cannoli_perl_strada.c: cannoli_perl.strada
	$(STRADAC) $< $@

# Build the combined .so (Strada code + C dispatch wrapper)
$(STRADA_TARGET): cannoli_perl_strada.c cannoli_dispatch.c $(BRIDGE_TARGET)
	$(CC) $(CFLAGS) -shared -o $@ cannoli_perl_strada.c cannoli_dispatch.c \
		$(STRADA_RUNTIME)/strada_runtime.c -ldl -lm -rdynamic
	@echo "Built: $(STRADA_TARGET)"
	@echo ""
	@echo "Usage:"
	@echo "  cannoli --library ./lib/perl/$(STRADA_TARGET):handler=MyApp::handle --dev"
	@echo ""
	@echo "Options (semicolon-separated after colon):"
	@echo "  handler=Sub::Name   Perl sub to call (required)"
	@echo "  script=/path/to.pl  Load a Perl script"
	@echo "  lib=/path           Add to @INC"
	@echo "  use=Module::Name    Load a module"

clean:
	rm -f $(BRIDGE_TARGET) $(STRADA_TARGET) cannoli_perl_strada.c *.o

# Example test
test: $(STRADA_TARGET)
	@echo "Creating test handler..."
	@echo 'package TestApp;' > /tmp/test_perl_handler.pl
	@echo 'use strict;' >> /tmp/test_perl_handler.pl
	@echo 'sub handle {' >> /tmp/test_perl_handler.pl
	@echo '    my ($$method, $$path, $$path_info, $$body) = @_;' >> /tmp/test_perl_handler.pl
	@echo '    if ($$path eq "/") {' >> /tmp/test_perl_handler.pl
	@echo '        return "{\"message\":\"Hello from Perl!\",\"status\":\"ok\"}";' >> /tmp/test_perl_handler.pl
	@echo '    }' >> /tmp/test_perl_handler.pl
	@echo '    elsif ($$path eq "/hello") {' >> /tmp/test_perl_handler.pl
	@echo '        return "Hello, World! (from Perl)";' >> /tmp/test_perl_handler.pl
	@echo '    }' >> /tmp/test_perl_handler.pl
	@echo '    elsif ($$path eq "/info") {' >> /tmp/test_perl_handler.pl
	@echo '        return "{\"method\":\"$$method\",\"path\":\"$$path\",\"path_info\":\"$$path_info\",\"perl_version\":\"$$^V\"}";' >> /tmp/test_perl_handler.pl
	@echo '    }' >> /tmp/test_perl_handler.pl
	@echo '    elsif ($$path =~ m{^/user/(\d+)}) {' >> /tmp/test_perl_handler.pl
	@echo '        return "{\"user_id\":$$1,\"name\":\"User $$1\"}";' >> /tmp/test_perl_handler.pl
	@echo '    }' >> /tmp/test_perl_handler.pl
	@echo '    return "";' >> /tmp/test_perl_handler.pl
	@echo '}' >> /tmp/test_perl_handler.pl
	@echo '1;' >> /tmp/test_perl_handler.pl
	@echo ""
	@echo "Test handler created at /tmp/test_perl_handler.pl"
	@echo "Run: cannoli --library ./lib/perl/$(STRADA_TARGET):script=/tmp/test_perl_handler.pl;handler=TestApp::handle --dev"
