###### chttp(1) — Minimal HTTP/1.1 Server
###### NAME
`chttp-server` — Minimal HTTP/1.1 server implemented with POSIX sockets.
###### DESCRIPTION
`chttp` is a deliberately minimal HTTP server written in C using POSIX socket interfaces.

The program creates an IPv4 TCP socket, binds it to port 8080 on all local interfaces, puts the socket into listening mode, accepts incoming connections, sends a fixed HTTP/1.1 response, closes the client connection, and waits for the next client.

The server does not parse the HTTP request and does not inspect the request method, URI, headers, or body. Every accepted connection receives the same response.

###### Server Execution Flow
1. `socket()` — Creates the listening socket descriptor.
2. `setsockopt()` — *(Optional)* Disables the Nagle algorithm.
3. `bind()` — Binds the socket to local address `0.0.0.0:8080`.
4. `listen()` — Places socket in passive listening mode.
5. `accept()` — Waits for an incoming connection.
6. `send()` — Transmits the static HTTP response.
7. `close()` — Closes the client connection socket.
8. Loop back to `accept()`.

---

###### SOCKET CREATION
`socket()` creates the kernel socket object used by the server.

```c
socket(AF_INET, SOCK_STREAM, 0);
```

The parameters select:
* **`AF_INET`**: IPv4 addressing.
* **`SOCK_STREAM`**: Connection-oriented byte stream provided by TCP.
* **`0`**: Default protocol for the selected address family and socket type.

The returned file descriptor is stored in `fd` and becomes the listening socket. Without `socket()`, there is no base socket on which remaining operations can be performed.

---

###### SOCKET OPTIONS
`setsockopt()` is used to configure TCP socket behavior.

```c
int yes = 1;
setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
```

This call disables the Nagle algorithm (`TCP_NODELAY`) for the TCP socket.

This call is not required for a minimally functional HTTP server. It is an explicit TCP performance configuration choice made by the program and can be safely omitted for the strict minimum implementation.

---

###### ADDRESS CONFIGURATION
`bind()` associates the socket with a local address and port.

The program configures the address structure:
```c
.sin_family      = AF_INET;
.sin_port        = htons(8080);
.sin_addr.s_addr = INADDR_ANY;
```

* **Effective Listening Endpoint**: `0.0.0.0:8080`
* **`INADDR_ANY`**: Specifies that the socket binds to all available local IPv4 interfaces.
* **`htons()`**: Converts the port number from host byte order to network byte order.

A newly created socket is not automatically associated with a local port. The `bind()` operation establishes that association.

---

###### LISTENING
`listen()` changes the socket into a passive listening socket capable of receiving incoming requests.

```c
listen(fd, 5);
```

* **`5`**: Represents the requested connection backlog limit.

After calling `listen()`, the file descriptor `fd` acts strictly as an entry point for accepting TCP connections rather than communicating directly with a specific client.

---

###### CONNECTION ACCEPTANCE
`accept()` blocks execution until an incoming connection arrives, returning a distinct file descriptor for the client interaction.

```c
int client_fd = accept(fd, NULL, NULL);
```

Two distinct descriptors are active:
* **`fd`**: The persistent listening socket.
* **`client_fd`**: The temporary socket dedicated to one client connection.

The listening socket (`fd`) remains open throughout the server lifecycle, while each client receives an individual connection descriptor (`client_fd`).

If `accept()` encounters an error, the program skips the iteration and resumes waiting:
```c
if (client_fd < 0)
    continue;
```

---

###### HTTP RESPONSE
The server transmits a fixed HTTP/1.1 response:

```http
HTTP/1.1 200 OK
Content-Type: text/plain; charset=utf-8
Content-Length: 21
Connection: close

Hola mundo, probando
```

The response consists of:
1. Status Line
2. Headers
3. Empty Line (`\r\n`)
4. Response Body

The empty CRLF line serves as the mandatory delimiter between header metadata and response content.

The payload is sent using:
```c
send(client_fd, response, strlen(response), 0);
```

No request data is read via `recv()` prior to transmitting this payload.

---

###### RESPONSE HEADERS
* **Status Line** (`HTTP/1.1 200 OK`): Identifies the HTTP version and confirms a successful request handling.
* **`Content-Type`**: Identifies the payload as plain text formatted in UTF-8.
* **`Content-Length`**: Specifies exact body size in bytes (21 bytes).
* **`Connection: close`**: Indicates the server will close the TCP socket immediately after transmission.

---

###### CONNECTION TERMINATION
After transmission, the server destroys the client descriptor:

```c
close(client_fd);
```

The server follows a short-lived request lifecycle per client:
1. Accept connection (`accept`)
2. Write response payload (`send`)
3. Disconnect client socket (`close`)

HTTP keep-alive connections and persistent sessions are intentionally omitted.

---

###### MAIN LOOP
The server runs an infinite execution loop:

```c
while (1) {
    int client_fd = accept(fd, NULL, NULL);

    if (client_fd < 0)
        continue;

    send(client_fd, response, strlen(response), 0);
    close(client_fd);
}
```

###### Connection Hierarchy
* Main Process: Listens indefinitely on `fd`
  * Iteration 1: Accept `client_1` -> Send response -> Close `client_1`
  * Iteration 2: Accept `client_2` -> Send response -> Close `client_2`
  * Iteration N: Accept `client_N` -> Send response -> Close `client_N`

---

###### MINIMUM SERVER SEQUENCE
The bare minimum functional sequence required for this server model:

1. `socket()` — Allocate kernel networking resources.
2. `bind()` — Assign address `0.0.0.0:8080`.
3. `listen()` — Mark socket as passive.
4. `accept()` — Retrieve active client session.
5. `send()` — Transfer HTTP response payload.
6. `close()` — Release client descriptor.
7. Return to step 4 (`accept`).

---

###### WHY THIS IS MINIMAL
Every TCP network service must establish a local socket endpoint, listen for inbound traffic, and complete a handshake.

Because this server always returns static content regardless of client input, it skips reading, parsing, and routing logic entirely.

The simplified path is:
* Create
* Bind
* Listen
* Accept
* Send
* Close

The `setsockopt()` step is an optional tuning choice and can be excluded entirely without breaking functionality.

---

###### HTTP SCOPE
This program functions as a **minimal HTTP responder** rather than a full-featured web server framework.

###### Deliberately Omitted Features
* Request reading (`recv`)
* Request header parsing
* HTTP verb routing (GET, POST, etc.)
* URI/URL route handling
* Static file serving
* Dynamic content execution
* Keep-alive / persistent connections
* TLS/SSL encryption
* Sessions and Cookies
* Authentication and authorization
* Request logging
* Concurrency / Multi-threading / I/O Multiplexing

---

###### PROTOCOL LAYERING
The application spans two distinct system abstractions:

###### Transport Layer (POSIX Sockets)
Handles raw network bytes using system calls:
* `socket()`
* `setsockopt()`
* `bind()`
* `listen()`
* `accept()`
* `send()`
* `close()`

###### Application Layer (HTTP/1.1)
The textual content transmitted across the TCP stream:
```http
HTTP/1.1 200 OK
...
```

The operating system kernel treats the payload as an arbitrary sequence of bytes and does not inspect the HTTP syntax.

---

###### LIBRARY FUNCTIONS VS SYSTEM CALLS
The server uses both POSIX system calls and standard C library routines:

* **System Calls** (Kernel boundary operations): `socket()`, `bind()`, `listen()`, `accept()`, `send()`, `close()`.
* **Library Helper Functions**: `htons()`, `strlen()`.

---

###### FILE DESCRIPTORS SUMMARY
* **`fd`**: Long-lived passive socket descriptor created at startup. Remains open until process shutdown.
* **`client_fd`**: Short-lived active socket descriptor generated by `accept()` and immediately freed by `close()`.

---

###### DESIGN SUMMARY (STATE MACHINE)

```
[ SOCKET STATE ]
       │
[ BIND STATE ]
       │
[ LISTEN STATE ]
       │
┌─► [ ACCEPT STATE ]
│      │
│   [ SEND STATE ]
│      │
│   [ CLOSE STATE ]
└──────┘
```

---

## SEE ALSO
* `socket(2)`
* `setsockopt(2)`
* `bind(2)`
* `listen(2)`
* `accept(2)`
* `send(2)`
* `clone(2)`
