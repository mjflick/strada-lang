# Cannoli.pm - Request/Response object for Perl handlers
#
# This module provides an object-oriented interface for handling
# HTTP requests in Cannoli Perl handlers.
#
# Usage:
#   sub handle {
#       my ($c) = @_;
#       my $method = $c->method;
#       my $path = $c->path;
#       $c->status(200);
#       $c->content_type('application/json');
#       $c->write('{"ok":true}');
#   }

package Cannoli;

use strict;
use warnings;

our $VERSION = '1.0';

# Constructor - called by perl_bridge with request data
sub new {
    my ($class, %args) = @_;

    my $self = bless {
        # Request data
        _method       => $args{method} // 'GET',
        _path         => $args{path} // '/',
        _path_info    => $args{path_info} // '',
        _query_string => $args{query_string} // '',
        _body         => $args{body} // '',
        _headers      => $args{headers} // {},
        _remote_addr  => $args{remote_addr} // '',
        _content_type => $args{content_type} // '',
        _params       => undef,  # Lazily parsed
        _variables    => {},     # Request variables/stash
        _document_root => $args{document_root} // '',

        # Response data
        _res_status   => 200,
        _res_content_type => '',
        _res_headers  => {},
        _res_body     => '',
        _res_redirect => undef,
        _allow_ranges => 0,
        _internal_redirect => undef,
    }, $class;

    return $self;
}

#
# Request Accessors
#

sub method       { $_[0]->{_method} }
sub path         { $_[0]->{_path} }
sub path_info    { $_[0]->{_path_info} }
sub query_string { $_[0]->{_query_string} }
sub body         { $_[0]->{_body} }
sub remote_addr  { $_[0]->{_remote_addr} }
sub request_content_type { $_[0]->{_content_type} }

# Get all request headers as hash ref
sub headers {
    my ($self) = @_;
    return $self->{_headers};
}

# Get a specific request header (case-insensitive)
sub header {
    my ($self, $name) = @_;
    return '' unless defined $name;
    my $lc_name = lc($name);
    return $self->{_headers}{$lc_name} // '';
}

# Check if request has a header
sub has_header {
    my ($self, $name) = @_;
    return 0 unless defined $name;
    return exists $self->{_headers}{lc($name)};
}

# Get parsed parameters (from query string and body)
sub params {
    my ($self) = @_;
    unless (defined $self->{_params}) {
        $self->{_params} = $self->_parse_params();
    }
    return $self->{_params};
}

# Get a specific parameter
sub param {
    my ($self, $name) = @_;
    my $params = $self->params;
    return $params->{$name} // '';
}

# Check if parameter exists
sub has_param {
    my ($self, $name) = @_;
    my $params = $self->params;
    return exists $params->{$name};
}

# Parse parameters from query string and body
sub _parse_params {
    my ($self) = @_;
    my %params;

    # Parse query string
    if (length($self->{_query_string})) {
        my $qs_params = _parse_query($self->{_query_string});
        %params = %$qs_params;
    }

    # Parse form body if applicable
    my $ct = $self->{_content_type};
    if ($self->{_method} eq 'POST' && length($self->{_body})) {
        if ($ct =~ /application\/x-www-form-urlencoded/i) {
            my $body_params = _parse_query($self->{_body});
            %params = (%params, %$body_params);
        }
    }

    return \%params;
}

# Parse query string into hash
sub _parse_query {
    my ($qs) = @_;
    my %params;
    return \%params unless defined $qs && length($qs);

    for my $pair (split /&/, $qs) {
        my ($key, $val) = split /=/, $pair, 2;
        next unless defined $key && length($key);
        $key = _url_decode($key);
        $val = _url_decode($val // '');
        $params{$key} = $val;
    }
    return \%params;
}

# URL decode
sub _url_decode {
    my ($s) = @_;
    return '' unless defined $s;
    $s =~ tr/+/ /;
    $s =~ s/%([0-9A-Fa-f]{2})/chr(hex($1))/eg;
    return $s;
}

# Check request method
sub is_get    { $_[0]->{_method} eq 'GET' }
sub is_post   { $_[0]->{_method} eq 'POST' }
sub is_put    { $_[0]->{_method} eq 'PUT' }
sub is_delete { $_[0]->{_method} eq 'DELETE' }
sub is_patch  { $_[0]->{_method} eq 'PATCH' }
sub is_head   { $_[0]->{_method} eq 'HEAD' }
sub is_options { $_[0]->{_method} eq 'OPTIONS' }

# Check if AJAX request
sub is_ajax {
    my ($self) = @_;
    return $self->header('x-requested-with') eq 'XMLHttpRequest';
}

# Get user agent
sub user_agent { $_[0]->header('user-agent') }

# Get referer
sub referer { $_[0]->header('referer') }

# Get host
sub host { $_[0]->header('host') }

#
# nginx-compatible aliases (ngx_http_perl_module compatibility)
#

# $r->args - returns request arguments (query string)
sub args { $_[0]->{_query_string} }

# $r->uri - returns request URI (path)
sub uri { $_[0]->{_path} }

# $r->request_method - returns HTTP method
sub request_method { $_[0]->{_method} }

# $r->request_body - returns client request body
sub request_body { $_[0]->{_body} }

# $r->request_body_file - returns filename with request body (not implemented)
sub request_body_file { return undef; }

# $r->header_in(field) - returns value of client request header
sub header_in {
    my ($self, $field) = @_;
    return $self->header($field);
}

# $r->header_out(field, value) - sets response header field
sub header_out {
    my ($self, $field, $value) = @_;
    return $self->set_header($field, $value);
}

# $r->header_only - returns true if only headers should be sent (HEAD request)
sub header_only { $_[0]->{_method} eq 'HEAD' }

# $r->filename - returns filename corresponding to request URI
sub filename {
    my ($self) = @_;
    return '' unless length($self->{_document_root});
    return $self->{_document_root} . $self->{_path};
}

# $r->allow_ranges - enables byte-range support for response
sub allow_ranges {
    my ($self) = @_;
    $self->{_allow_ranges} = 1;
    return $self;
}

# $r->discard_request_body - instructs to discard request body (no-op)
sub discard_request_body { return $_[0]; }

# $r->internal_redirect(uri) - performs internal redirect
sub internal_redirect {
    my ($self, $uri) = @_;
    $self->{_internal_redirect} = $uri;
    return $self;
}

# $r->send_http_header([type]) - sends response header (no-op, done automatically)
sub send_http_header {
    my ($self, $type) = @_;
    $self->content_type($type) if defined $type;
    return $self;
}

# $r->flush - immediately sends data to client (no-op in our model)
sub flush { return $_[0]; }

# $r->log_error(errno, message) - writes message to error log
sub log_error {
    my ($self, $errno, $message) = @_;
    if ($errno) {
        warn "[error] $message (errno: $errno)\n";
    } else {
        warn "[error] $message\n";
    }
    return $self;
}

# $r->unescape(text) - decodes URI-encoded text
sub unescape {
    my ($self, $text) = @_;
    return _url_decode($text);
}

# $r->variable(name[, value]) - gets or sets nginx variable (request stash)
sub variable {
    my ($self, $name, $value) = @_;
    if (@_ > 2) {
        $self->{_variables}{$name} = $value;
        return $self;
    }
    return $self->{_variables}{$name};
}

# Alias: stash (Mojolicious-style)
sub stash { shift->variable(@_) }

# $r->sleep(milliseconds, handler) - not implemented (async not supported)
sub sleep {
    my ($self, $ms, $handler) = @_;
    warn "Cannoli::sleep() not implemented - async handlers not supported\n";
    return $self;
}

# $r->has_request_body(handler) - checks for request body
sub has_request_body {
    my ($self, $handler) = @_;
    if (length($self->{_body}) > 0) {
        $handler->($self) if ref $handler eq 'CODE';
        return 1;
    }
    return 0;
}

# $r->sendfile(name[, offset[, length]]) - sends specified file to client
sub sendfile {
    my ($self, $filename, $offset, $length) = @_;
    $offset //= 0;

    return $self unless defined $filename && -f $filename;

    # Read file content
    open my $fh, '<', $filename or do {
        $self->log_error(0, "Cannot open file: $filename");
        return $self;
    };
    binmode $fh;

    # Seek to offset if specified
    if ($offset > 0) {
        seek($fh, $offset, 0);
    }

    # Read content
    my $content;
    if (defined $length && $length > 0) {
        read($fh, $content, $length);
    } else {
        local $/;
        $content = <$fh>;
    }
    close $fh;

    # Auto-detect content type based on extension
    my $ct = 'application/octet-stream';
    if ($filename =~ /\.html?$/i) { $ct = 'text/html'; }
    elsif ($filename =~ /\.css$/i) { $ct = 'text/css'; }
    elsif ($filename =~ /\.js$/i) { $ct = 'application/javascript'; }
    elsif ($filename =~ /\.json$/i) { $ct = 'application/json'; }
    elsif ($filename =~ /\.xml$/i) { $ct = 'application/xml'; }
    elsif ($filename =~ /\.txt$/i) { $ct = 'text/plain'; }
    elsif ($filename =~ /\.png$/i) { $ct = 'image/png'; }
    elsif ($filename =~ /\.jpe?g$/i) { $ct = 'image/jpeg'; }
    elsif ($filename =~ /\.gif$/i) { $ct = 'image/gif'; }
    elsif ($filename =~ /\.svg$/i) { $ct = 'image/svg+xml'; }
    elsif ($filename =~ /\.ico$/i) { $ct = 'image/x-icon'; }
    elsif ($filename =~ /\.pdf$/i) { $ct = 'application/pdf'; }
    elsif ($filename =~ /\.zip$/i) { $ct = 'application/zip'; }

    $self->content_type($ct) unless length($self->{_res_content_type});
    $self->write($content);

    # Set Content-Length for range requests
    if ($self->{_allow_ranges}) {
        $self->set_header('Accept-Ranges', 'bytes');
    }

    return $self;
}

# Parse JSON body
sub json {
    my ($self) = @_;
    return undef unless length($self->{_body});
    return _parse_json($self->{_body});
}

#
# Response Methods
#

# Set/get response status
sub status {
    my ($self, $code) = @_;
    if (defined $code) {
        $self->{_res_status} = int($code);
        return $self;  # Allow chaining
    }
    return $self->{_res_status};
}

# Set/get response content type
sub content_type {
    my ($self, $type) = @_;
    if (defined $type) {
        $self->{_res_content_type} = $type;
        return $self;
    }
    return $self->{_res_content_type};
}

# Set a response header
sub set_header {
    my ($self, $name, $value) = @_;
    $self->{_res_headers}{$name} = $value;
    return $self;
}

# Get response headers
sub response_headers {
    my ($self) = @_;
    return $self->{_res_headers};
}

# Write to response body
sub write {
    my ($self, $content) = @_;
    $self->{_res_body} .= $content if defined $content;
    return $self;
}

# Alias for write
sub print { shift->write(@_) }

# Set response body (replaces existing)
sub body_set {
    my ($self, $content) = @_;
    $self->{_res_body} = $content // '';
    return $self;
}

# Get response body
sub response_body {
    my ($self) = @_;
    return $self->{_res_body};
}

# Send redirect
sub redirect {
    my ($self, $url, $code) = @_;
    $self->{_res_redirect} = $url;
    $self->{_res_status} = $code // 302;
    return $self;
}

# Render JSON response
sub render_json {
    my ($self, $data) = @_;
    $self->content_type('application/json');
    $self->write(_to_json($data));
    return $self;
}

# Render text response
sub render_text {
    my ($self, $text) = @_;
    $self->content_type('text/plain');
    $self->write($text);
    return $self;
}

# Render HTML response
sub render_html {
    my ($self, $html) = @_;
    $self->content_type('text/html');
    $self->write($html);
    return $self;
}

# Send error response
sub error {
    my ($self, $code, $message) = @_;
    $self->status($code // 500);
    $self->content_type('application/json');
    $self->body_set(_to_json({ error => $message // 'Internal Server Error' }));
    return $self;
}

# Send not found response
sub not_found {
    my ($self, $message) = @_;
    return $self->error(404, $message // 'Not Found');
}

# Send bad request response
sub bad_request {
    my ($self, $message) = @_;
    return $self->error(400, $message // 'Bad Request');
}

# Set cookie
sub set_cookie {
    my ($self, $name, $value, %opts) = @_;
    my $cookie = "$name=$value";
    $cookie .= "; Path=$opts{path}" if $opts{path};
    $cookie .= "; Domain=$opts{domain}" if $opts{domain};
    $cookie .= "; Max-Age=$opts{max_age}" if $opts{max_age};
    $cookie .= "; Expires=$opts{expires}" if $opts{expires};
    $cookie .= "; Secure" if $opts{secure};
    $cookie .= "; HttpOnly" if $opts{httponly};
    $cookie .= "; SameSite=$opts{samesite}" if $opts{samesite};
    $self->set_header('Set-Cookie', $cookie);
    return $self;
}

#
# Internal: Build response string for bridge
#
sub _build_response {
    my ($self) = @_;

    # Handle redirect
    if (defined $self->{_res_redirect}) {
        return "REDIRECT:" . $self->{_res_redirect};
    }

    # Build response with status and headers
    my $status = $self->{_res_status};
    my $body = $self->{_res_body};

    # If no body written, return empty for 404 handling
    return '' if $status == 200 && length($body) == 0;

    # Build header string
    my @header_parts;
    if (length($self->{_res_content_type})) {
        push @header_parts, "Content-Type:" . $self->{_res_content_type};
    }
    for my $name (keys %{$self->{_res_headers}}) {
        push @header_parts, "$name:" . $self->{_res_headers}{$name};
    }
    my $headers_str = join("\n", @header_parts);

    # Format: RESPONSE:status:headers_len:headers:body
    if (@header_parts || $status != 200) {
        return "RESPONSE:$status:" . length($headers_str) . ":$headers_str:$body";
    }

    # Simple case: just body (status 200, no custom headers)
    return $body;
}

#
# JSON helpers (no dependencies)
#

sub _to_json {
    my ($data) = @_;
    return 'null' unless defined $data;

    if (ref $data eq 'HASH') {
        my @pairs;
        for my $k (sort keys %$data) {
            my $v = $data->{$k};
            push @pairs, '"' . _json_escape($k) . '":' . _to_json($v);
        }
        return '{' . join(',', @pairs) . '}';
    }
    elsif (ref $data eq 'ARRAY') {
        my @items = map { _to_json($_) } @$data;
        return '[' . join(',', @items) . ']';
    }
    elsif (!ref $data) {
        # Check if numeric
        if ($data =~ /^-?\d+$/) {
            return $data;
        }
        elsif ($data =~ /^-?\d+\.\d+$/) {
            return $data;
        }
        elsif ($data eq 'true' || $data eq 'false') {
            return $data;
        }
        return '"' . _json_escape($data) . '"';
    }
    return 'null';
}

sub _json_escape {
    my ($s) = @_;
    return '' unless defined $s;
    $s =~ s/\\/\\\\/g;
    $s =~ s/"/\\"/g;
    $s =~ s/\n/\\n/g;
    $s =~ s/\r/\\r/g;
    $s =~ s/\t/\\t/g;
    return $s;
}

sub _parse_json {
    my ($str) = @_;
    return undef unless defined $str && length($str);

    # Very basic JSON parser
    $str =~ s/^\s+//;
    $str =~ s/\s+$//;

    if ($str =~ /^\{/) {
        return _parse_json_object($str);
    }
    elsif ($str =~ /^\[/) {
        return _parse_json_array($str);
    }
    return $str;
}

sub _parse_json_object {
    my ($str) = @_;
    my %result;

    # Remove outer braces
    $str =~ s/^\{//;
    $str =~ s/\}$//;

    # Simple key-value extraction
    while ($str =~ /"([^"]+)"\s*:\s*(?:"([^"]*)"|(-?\d+(?:\.\d+)?)|(\{[^}]*\})|\[([^\]]*)\]|(true|false|null))/g) {
        my ($key, $str_val, $num_val, $obj_val, $arr_val, $bool_val) = ($1, $2, $3, $4, $5, $6);
        if (defined $str_val) {
            $result{$key} = $str_val;
        } elsif (defined $num_val) {
            $result{$key} = $num_val + 0;
        } elsif (defined $obj_val) {
            $result{$key} = _parse_json_object($obj_val);
        } elsif (defined $arr_val) {
            $result{$key} = _parse_json_array("[$arr_val]");
        } elsif (defined $bool_val) {
            $result{$key} = $bool_val eq 'true' ? 1 : $bool_val eq 'false' ? 0 : undef;
        }
    }
    return \%result;
}

sub _parse_json_array {
    my ($str) = @_;
    my @result;

    $str =~ s/^\[//;
    $str =~ s/\]$//;

    for my $item (split /,/, $str) {
        $item =~ s/^\s+//;
        $item =~ s/\s+$//;
        if ($item =~ /^"(.*)"$/) {
            push @result, $1;
        } elsif ($item =~ /^-?\d+(?:\.\d+)?$/) {
            push @result, $item + 0;
        } elsif ($item =~ /^\{/) {
            push @result, _parse_json_object($item);
        } elsif ($item eq 'true') {
            push @result, 1;
        } elsif ($item eq 'false') {
            push @result, 0;
        } elsif ($item eq 'null') {
            push @result, undef;
        }
    }
    return \@result;
}

1;

__END__

=head1 NAME

Cannoli - Request/Response object for Cannoli Perl handlers

=head1 SYNOPSIS

    sub handle {
        my ($c) = @_;

        # Access request data
        my $method = $c->method;
        my $path = $c->path;
        my $name = $c->param('name');
        my $auth = $c->header('Authorization');

        # Send response
        $c->status(200);
        $c->content_type('application/json');
        $c->write('{"status":"ok"}');

        # Or use shortcuts
        $c->render_json({ status => 'ok' });
    }

=head1 DESCRIPTION

Cannoli provides an object-oriented interface for handling HTTP requests
in Cannoli web server Perl handlers.

=head1 REQUEST METHODS

=over 4

=item method() - HTTP method (GET, POST, etc.)

=item path() - Request path

=item path_info() - Path info after prefix match

=item query_string() - Raw query string

=item body() - Request body

=item remote_addr() - Client IP address

=item headers() - All headers as hash ref

=item header($name) - Get specific header

=item param($name) - Get request parameter

=item params() - All parameters as hash ref

=item json() - Parse body as JSON

=item is_get(), is_post(), is_head(), etc. - Check request method

=item user_agent() - Get User-Agent header

=item referer() - Get Referer header

=item host() - Get Host header

=item is_ajax() - Check if XMLHttpRequest

=back

=head1 RESPONSE METHODS

=over 4

=item status($code) - Set response status

=item content_type($type) - Set content type

=item set_header($name, $value) - Set response header

=item write($content) - Append to response body

=item print($content) - Alias for write()

=item redirect($url, $code) - Send redirect

=item render_json($data) - Render JSON response

=item render_html($html) - Render HTML response

=item render_text($text) - Render plain text response

=item error($code, $message) - Send error response

=item not_found($message) - Send 404 response

=item bad_request($message) - Send 400 response

=item set_cookie($name, $value, %opts) - Set response cookie

=item sendfile($path, $offset, $length) - Send file content

=item allow_ranges() - Enable byte-range support

=back

=head1 NGINX-COMPATIBLE METHODS

These methods provide compatibility with nginx's ngx_http_perl_module:

=over 4

=item args() - Returns query string (alias for query_string)

=item uri() - Returns request path (alias for path)

=item request_method() - Returns HTTP method (alias for method)

=item request_body() - Returns request body (alias for body)

=item header_in($field) - Get request header (alias for header)

=item header_out($field, $value) - Set response header (alias for set_header)

=item header_only() - Returns true if HEAD request

=item filename() - Returns document_root + path

=item unescape($text) - URL-decode text

=item variable($name, $value) - Get/set request variable

=item stash($name, $value) - Alias for variable (Mojolicious-style)

=item log_error($errno, $message) - Write to error log

=item send_http_header($type) - Set content type (headers sent automatically)

=item flush() - Flush output (no-op, synchronous model)

=item discard_request_body() - Discard body (no-op)

=item internal_redirect($uri) - Perform internal redirect

=item has_request_body($handler) - Check/handle request body

=item allow_ranges() - Enable byte-range responses

=back

=head1 NOTES

Some nginx methods are not fully implemented due to architectural differences:

=over 4

=item * sleep() - Async handlers not supported

=item * request_body_file() - Request body not stored in temp files

=back

=cut
