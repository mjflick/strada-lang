# Perl Example for Cannoli

This example demonstrates how to run a Perl web application with Cannoli.

## Quick Start

```bash
./run.sh
```

Then open http://localhost:8080 in your browser.

## Custom Port

```bash
./run.sh 3000
```

## Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Home page (HTML) |
| `/api/users` | GET | List all users |
| `/api/users/:id` | GET | Get user by ID |
| `/api/users` | POST | Create new user |
| `/api/users/:id` | DELETE | Delete user |
| `/api/todos` | GET | List all todos |
| `/api/todos/:id` | GET | Get todo by ID |
| `/api/todos` | POST | Create new todo |
| `/api/todos/:id/toggle` | POST | Toggle todo done status |
| `/info` | GET | Server information |
| `/health` | GET | Health check |

## Example Requests

```bash
# List users
curl http://localhost:8080/api/users

# Get specific user
curl http://localhost:8080/api/users/1

# Create user
curl -X POST -d '{"name":"Dave","email":"dave@example.com"}' http://localhost:8080/api/users

# Create todo
curl -X POST -d '{"task":"Learn Cannoli"}' http://localhost:8080/api/todos

# Toggle todo done
curl -X POST http://localhost:8080/api/todos/1/toggle

# Server info
curl http://localhost:8080/info
```

## Files

- `app.pl` - The Perl application with routing and handlers
- `run.sh` - Startup script
