# app.pl - Example Perl web application for Cannoli
#
# This demonstrates how to write a Perl handler for Cannoli.
# The handler receives a Cannoli object with request/response methods.

package MyApp;

use strict;
use warnings;

# In-memory data store (demo purposes)
my %users = (
    1 => { id => 1, name => 'Alice', email => 'alice@example.com' },
    2 => { id => 2, name => 'Bob', email => 'bob@example.com' },
    3 => { id => 3, name => 'Charlie', email => 'charlie@example.com' },
);
my $next_id = 4;

my %todos = (
    1 => { id => 1, task => 'Learn Strada', done => 0 },
    2 => { id => 2, task => 'Build with Cannoli', done => 0 },
    3 => { id => 3, task => 'Write Perl handlers', done => 1 },
);
my $next_todo_id = 4;

# Main request handler - receives Cannoli object
sub handle {
    my ($c) = @_;

    my $method = $c->method;
    my $path = $c->path;

    # Home page
    if ($path eq '/' && $c->is_get) {
        $c->content_type('text/html');
	my $remote_addr = $c->remote_addr;
	warn($remote_addr);
        $c->write(html_page('Welcome', qq{
<h1>Cannoli + Perl Example</h1>
<p>This is a sample web application running Perl handlers through Cannoli.</p>
<h2>Available Endpoints</h2>
<ul>
    <li><a href="/api/users">GET /api/users</a> - List all users</li>
    <li><a href="/api/users/1">GET /api/users/:id</a> - Get user by ID</li>
    <li><a href="/api/todos">GET /api/todos</a> - List all todos</li>
    <li><a href="/info">GET /info</a> - Server info</li>
    <li><a href="/echo?msg=hello&foo=bar">GET /echo?msg=hello</a> - Echo test</li>
</ul>
<h2>Try POST</h2>
<pre>curl -X POST -H "Content-Type: application/json" -d '{"name":"Dave","email":"dave\@example.com"}' http://localhost:8080/api/users</pre>
<h2>Request Info</h2>
<p>Your IP: $remote_addr</p>
}));
        return;
    }

    # Server info - demonstrates accessing request data
    if ($path eq '/info' && $c->is_get) {
        $c->render_json({
            server => 'Cannoli',
            language => 'Perl',
            perl_version => "$^V",
            pid => $$,
            time => scalar localtime(),
            remote_addr => $c->remote_addr,
            user_agent => $c->user_agent,
            host => $c->host,
        });
        return;
    }

    # Echo endpoint - demonstrates query params
    if ($path eq '/echo' && $c->is_get) {
        $c->render_json({
            echo => $c->params,
            method => $method,
            query_string => $c->query_string,
            has_msg => $c->has_param('msg') ? 'yes' : 'no',
            msg_value => $c->param('msg'),
        });
        return;
    }

    # Headers endpoint - demonstrates header access
    if ($path eq '/headers' && $c->is_get) {
        $c->render_json({
            all_headers => $c->headers,
            user_agent => $c->header('user-agent'),
            accept => $c->header('accept'),
        });
        return;
    }

    # API: List users
    if ($path eq '/api/users' && $c->is_get) {
        my @user_list = map { $users{$_} } sort { $a <=> $b } keys %users;
        $c->render_json({ users => \@user_list, count => scalar @user_list });
        return;
    }

    # API: Get user by ID
    if ($path =~ m{^/api/users/(\d+)$} && $c->is_get) {
        my $id = $1;
        if (exists $users{$id}) {
            $c->render_json($users{$id});
            return;
        }
        $c->not_found("User not found: $id");
        return;
    }

    # API: Create user - demonstrates POST body and JSON parsing
    if ($path eq '/api/users' && $c->is_post) {
        my $data = $c->json;
        if (!$data || !$data->{name}) {
            $c->bad_request('Name is required');
            return;
        }
        my $user = {
            id => $next_id++,
            name => $data->{name},
            email => $data->{email} // '',
        };
        $users{$user->{id}} = $user;
        $c->status(201)->render_json($user);
        return;
    }

    # API: Delete user
    if ($path =~ m{^/api/users/(\d+)$} && $c->is_delete) {
        my $id = $1;
        if (exists $users{$id}) {
            delete $users{$id};
            $c->render_json({ deleted => $id + 0 });
            return;
        }
        $c->not_found('User not found');
        return;
    }

    # API: List todos
    if ($path eq '/api/todos' && $c->is_get) {
        my @todo_list = map { $todos{$_} } sort { $a <=> $b } keys %todos;
        $c->render_json({ todos => \@todo_list });
        return;
    }

    # API: Get todo by ID
    if ($path =~ m{^/api/todos/(\d+)$} && $c->is_get) {
        my $id = $1;
        if (exists $todos{$id}) {
            $c->render_json($todos{$id});
            return;
        }
        $c->not_found('Todo not found');
        return;
    }

    # API: Create todo
    if ($path eq '/api/todos' && $c->is_post) {
        my $data = $c->json;
        if (!$data || !$data->{task}) {
            $c->bad_request('Task is required');
            return;
        }
        my $todo = {
            id => $next_todo_id++,
            task => $data->{task},
            done => 0,
        };
        $todos{$todo->{id}} = $todo;
        $c->status(201)->render_json($todo);
        return;
    }

    # API: Toggle todo done status
    if ($path =~ m{^/api/todos/(\d+)/toggle$} && $c->is_post) {
        my $id = $1;
        if (exists $todos{$id}) {
            $todos{$id}{done} = $todos{$id}{done} ? 0 : 1;
            $c->render_json($todos{$id});
            return;
        }
        $c->not_found('Todo not found');
        return;
    }

    # Health check
    if ($path eq '/health') {
        $c->render_json({ status => 'ok' });
        return;
    }

    # Redirect example
    if ($path eq '/old-path') {
        $c->redirect('/info');
        return;
    }

    # Custom headers example
    if ($path eq '/custom-headers') {
        $c->set_header('X-Custom-Header', 'Hello from Perl!')
          ->set_header('X-Powered-By', 'Cannoli')
          ->render_json({ message => 'Check the response headers!' });
        return;
    }

    # Cookie example
    if ($path eq '/set-cookie') {
        $c->set_cookie('session', 'abc123', path => '/', httponly => 1)
          ->render_json({ message => 'Cookie set!' });
        return;
    }

    # Plain text response
    if ($path eq '/text') {
        $c->render_text("Hello, World!\nThis is plain text.");
        return;
    }

    # nginx-compatible methods demo
    if ($path eq '/nginx-compat') {
        # Using nginx-style method names
        $c->render_json({
            # Request info using nginx aliases
            args => $c->args,                    # query string
            uri => $c->uri,                      # path
            request_method => $c->request_method, # method
            request_body => $c->request_body,    # body
            header_only => $c->header_only ? 1 : 0, # is HEAD?
            user_agent => $c->header_in('User-Agent'),

            # Variable/stash demo
            stash_test => do {
                $c->variable('foo', 'bar');
                $c->stash('num', 42);
                { foo => $c->variable('foo'), num => $c->stash('num') };
            },

            # URL decoding
            decoded => $c->unescape('hello%20world%21'),
        });
        return;
    }

    # Serve static file demo
    if ($path =~ m{^/static/(.+)$}) {
        my $file = $1;
        # Prevent directory traversal
        $file =~ s/\.\.//g;
        my $filepath = "/home/mflickin/p/strada/cannoli/examples/perl/$file";
        if (-f $filepath) {
            $c->sendfile($filepath);
        } else {
            $c->not_found("File not found: $file");
        }
        return;
    }

    # Return nothing - lets Cannoli handle 404
    return;
}

# Helper to generate HTML pages
sub html_page {
    my ($title, $content) = @_;
    return <<"HTML";
<!DOCTYPE html>
<html>
<head>
    <title>$title</title>
    <style>
        body { font-family: system-ui, sans-serif; max-width: 800px; margin: 2em auto; padding: 0 1em; }
        h1 { color: #333; }
        a { color: #0066cc; }
        pre { background: #f5f5f5; padding: 1em; overflow-x: auto; }
        ul { line-height: 1.8; }
    </style>
</head>
<body>
$content
</body>
</html>
HTML
}

1;
