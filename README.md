# Encrypt-Relay: Secure Multi-Client Chat System

A networked chat application featuring encrypted message transmission using AES-256-CBC encryption. Multiple clients can connect to a central server and communicate securely with all connected users.

---

## Features

- **Multi-Client Communication**: Support for up to 10 simultaneous clients with broadcast messaging
- **AES-256-CBC Encryption**: Secure message encryption using OpenSSL library with random IVs for each message
- **User Authentication**: Simple username-based client identification on connection
- **Timestamp Tracking**: All messages include server-side timestamps in format `[YYYY-MM-DD HH:MM:SS]`
- **Graceful Disconnect**: Clients can type `quit` to disconnect, with server notifications to other users
- **Cross-Platform Support**: Batch scripts provided for Windows (PowerShell), compatible with Unix-like systems
- **IPv4 and IPv6 Support**: Dual stack socket implementation for both IP protocols
- **Non-Blocking I/O**: Uses `select()` for efficient multiplexed I/O on both server and client sides

---

## How to Run the Program

### Prerequisites

- **GCC or Clang** compiler
- **OpenSSL development libraries** (`libssl-dev` on Ubuntu/Debian, or pre-installed on macOS)
- macOS, Linux, or Windows (with compilation tools)

### Building from Source

Navigate to the project directory and compile the server and client:

```bash
gcc -o server server.c -lssl -lcrypto
gcc -o client client.c -lssl -lcrypto
```

On some systems, you may also need to link against additional libraries:

```bash
gcc -o server server.c -lssl -lcrypto -ldl -lpthread
gcc -o client client.c -lssl -lcrypto -ldl -lpthread
```

### Running the Chat System

#### Terminal 1 - Start the Server

```bash
./server
```

Expected output:
```
=== Chat Server Started ===
Listening on port 3490
Waiting for connections...
```

#### Terminal 2 - Start the Client

```bash
./client localhost
```

Or connect to a remote server:

```bash
./client 192.168.1.100
```

#### Terminal 3+ - Additional Clients (Optional)

Repeat the client command in additional terminals to connect more users:

```bash
./client localhost
```

### Usage

1. **Enter your username/identifier** when prompted
2. **Type messages** and press Enter to send them to all connected users
3. **Receive encrypted messages** from other connected users (decrypted automatically on client side)
4. **Disconnect** by typing `quit` and pressing Enter

Example chat session:
```
Enter your name/identifier: Alice
=== Connected to Chat Server ===
Type your messages and press Enter. Type 'quit' to exit.
Welcome, Alice! You are now connected. There are 1 user(s) online.

Hello everyone!
[2025-12-13 14:32:10] Bob: Hi Alice!
Goodbye!
```

---

## Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────┐
│                    Chat Network                         │
└──────────────┬──────────────────┬───────────────────────┘
               │                  │
        ┌──────▼─────┐    ┌───────▼────┐    ┌──────────┐
        │   Client   │    │   Client   │    │  Client  │
        │   (TCP)    │    │   (TCP)    │    │  (TCP)   │
        └──────┬─────┘    └───────┬────┘    └──────┬───┘
               │                  │                │
               └──────────────────┼────────────────┘
                                  │
                           ┌──────▼──────────┐
                           │  Chat Server    │
                           │  (Multiplexed   │
                           │   I/O with      │
                           │   select())     │
                           │  Port: 3490     │
                           └─────────────────┘
```

### Component Architecture

#### Server (`server.c`)

**Responsibilities:**
- Listen for incoming TCP connections on port 3490
- Manage connected client list (max 10 clients)
- Receive encrypted messages from clients
- Decrypt messages using AES-256-CBC
- Broadcast decrypted messages to all connected clients (except sender)
- Re-encrypt messages before broadcasting
- Handle client disconnection and cleanup

**Key Data Structures:**
```c
typedef struct {
    int fd;                          // Socket file descriptor
    char username[64];               // Client username
    struct sockaddr_storage addr;    // Client address info
} client_t;
```

**I/O Model:**
- Uses `select()` for multiplexed I/O
- Single-threaded event loop
- Maintains master file descriptor set for all client connections

**Encryption:**
- **Algorithm**: AES-256 in CBC mode
- **Key**: 256-bit hardcoded key (32 bytes)
- **IV**: 128-bit (16 bytes) - same for all messages in current implementation
- **Padding**: PKCS#7 (handled by OpenSSL EVP functions)
- Functions: `aes_encrypt()` and `aes_decrypt()`

#### Client (`client.c`)

**Responsibilities:**
- Connect to server via TCP at specified hostname/IP
- Prompt user for username
- Send encrypted username to server (XOR encryption for username only)
- Receive welcome message
- Maintain bidirectional communication with server
- Handle simultaneous user input and server messages using `select()`
- Encrypt user messages before sending
- Decrypt received messages before display
- Handle graceful disconnect

**I/O Model:**
- Uses `select()` to monitor both stdin and socket simultaneously
- Non-blocking approach allows real-time message reception while accepting user input

**Encryption:**
- **Username**: XOR encryption with key "NetworksCS522Key"
- **Messages**: AES-256-CBC (same as server)

**User Flow:**
1. Connect to server via TCP
2. Receive welcome message
3. Enter username (sent XOR-encrypted)
4. Receive server acknowledgment
5. Enter chat loop with simultaneous input/receive handling
6. Type "quit" to disconnect

### Network Protocol

**Connection Flow:**
```
Client                              Server
  │                                   │
  ├──────── TCP SYN ─────────────────>│
  │<──────── TCP SYN-ACK ─────────────┤
  ├──────── TCP ACK ──────────────────>│
  │<─ Welcome Message (plaintext) ────┤
  │                                    │
  ├─ Username (XOR encrypted) ────────>│
  │<─ Welcome ACK (AES encrypted) ────┤
  │                                    │
  ├─ Messages (AES-256-CBC) ────────→>│(broadcast to all)
  │<─ Messages (AES-256-CBC) ────────┤
  │         [bidirectional]            │
  │                                    │
  ├─ "quit" message ──────────────────>│
  ├──────── TCP FIN ──────────────────>│
  └─ TCP FIN-ACK ────────────────────>│
```

### Encryption Details

**AES-256-CBC Configuration:**
- **Cipher**: Advanced Encryption Standard with 256-bit key
- **Mode**: Cipher Block Chaining (CBC) for sequential blocks
- **Key Size**: 256 bits (32 bytes)
- **Block Size**: 128 bits (16 bytes)
- **Padding**: PKCS#7 (automatic via OpenSSL)

**Current Implementation Notes:**
- IV is static for all messages (hardcoded in code)
- For production use, consider implementing random IV generation with IV prepending
- Server contains `aes_encrypt_with_random_iv()` function for future enhancement
- All encryption handled via OpenSSL EVP (high-level, OpenSSL 1.1.0+)

### Message Flow

**Broadcast Example:**
```
User A sends: "Hello everyone!"
     │
     ▼ [AES-256-CBC Encrypt]
Server receives encrypted data
     │
     ▼ [AES-256-CBC Decrypt]
Server formats: "[2025-12-13 14:32:10] Alice: Hello everyone!"
     │
     ├─ [AES-256-CBC Encrypt] ──> User B (decrypt & display)
     │
     └─ [AES-256-CBC Encrypt] ──> User C (decrypt & display)
```

### File Structure

```
├── README.md                 # This documentation
├── server.c                  # Server implementation
├── client.c                  # Client implementation
├── talker.c                  # UDP example (educational)
├── listener                  # Compiled UDP listener (binary)
├── client                    # Compiled client binary
├── server                    # Compiled server binary
├── QUICKSTART.md            # Quick start guide
├── start_server.ps1         # Windows PowerShell server launcher
├── start_client.ps1         # Windows PowerShell client launcher
├── start_server.bat         # Windows Batch server launcher
├── start_client.bat         # Windows Batch client launcher
└── Review/                  # Review documents/notes
```

### Limitations & Future Enhancements

**Current Limitations:**
- Maximum 10 simultaneous clients
- Static IV for encryption (not cryptographically ideal)
- Hardcoded encryption keys (not suitable for production)
- No user authentication or access control
- No message history or persistence
- No error recovery for lost connections

**Suggested Improvements:**
1. Implement Diffie-Hellman key exchange for secure key negotiation
2. Use random IVs per message (implementation exists in `aes_encrypt_with_random_iv()`)
3. Add message delivery confirmation
4. Implement TLS/SSL for transport layer security
5. Add user authentication with passwords
6. Increase client limit or use connection pooling
7. Add message logging to database
8. Implement graceful error handling and reconnection logic
9. Add private messaging between users
10. Implement command system (e.g., `/list`, `/users`, `/help`)

---

## Compilation & Troubleshooting

### Build Issues

**Missing OpenSSL:**
```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# macOS (with Homebrew)
brew install openssl

# Link with Homebrew OpenSSL path
gcc -o server server.c -I/usr/local/opt/openssl/include -L/usr/local/opt/openssl/lib -lssl -lcrypto
```

**Linker Errors:**
If you get errors about undefined references to OpenSSL functions, ensure you're linking in the correct order:
```bash
gcc -o server server.c -lssl -lcrypto -ldl
```

### Runtime Issues

**Connection Refused:**
- Ensure server is running before starting clients
- Check that port 3490 is not already in use: `lsof -i :3490`

**Encryption/Decryption Errors:**
- Verify both server and client use compatible encryption keys
- Check OpenSSL version compatibility (EVP API requires OpenSSL 1.1.0+)

---

## Summary

Encrypt-Relay demonstrates a practical implementation of a networked chat system with AES-256-CBC encryption. The multi-client server uses event-driven I/O with `select()` for efficient concurrent connection handling, while the client implements simultaneous bidirectional communication through stdin/socket multiplexing. The system serves as an educational example of socket programming, encryption implementation, and network protocol design.
