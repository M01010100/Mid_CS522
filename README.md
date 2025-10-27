<!-- filepath: c:\Users\matth\Classes\CS_522_Networks\Midterm\README.md -->
# Terminal-Based Chat Relay System

A bidirectional chat application using TCP sockets in C. This allows real-time text communication between a server and multiple clients in a chat room style - messages from any client are broadcast to all other connected clients.

## Features

- **Chat Room Broadcast**: Messages from any client are broadcast to all other connected clients
- **Multiple Concurrent Clients**: Server uses `select()` to handle up to 10 simultaneous client connections
- **User Identification**: Clients provide a username/identifier when connecting
- **Message Encryption**: XOR-based encryption for secure message transmission
- **Timestamps**: Each received message is displayed with a timestamp showing when it was received
- **Join/Leave Notifications**: All users are notified when someone joins or leaves the chat
- **Simple Terminal Interface**: Type messages directly in the terminal
- **Connection Management**: Graceful handling of disconnections and quit commands
- **Cross-platform Scripts**: PowerShell and batch file support for easy compilation and execution

## Files

- `server.c` - Chat server implementation
- `client.c` - Chat client implementation
- `start_server.ps1` / `start_server.bat` - Server startup scripts
- `start_client.ps1` / `start_client.bat` - Client startup scripts

## Quick Start

### Option 1: Using PowerShell Scripts 

#### Start the Server
```powershell
cd Midterm
.\start_server.ps1
```

#### Start a Client (in a new terminal)
```powershell
cd Midterm
.\start_client.ps1 localhost
```

### Option 2: Using Batch Files (WSL)

#### Start the Server
```cmd
cd Midterm
start_server.bat
```

#### Start a Client (in a new terminal)
```cmd
cd Midterm
start_client.bat localhost
```

### Option 3: Manual Compilation (Linux/WSL)

#### Compile
```bash
cd Midterm
gcc -o server server.c -Wall
gcc -o client client.c -Wall
```

#### Run Server
```bash
./server
```

#### Run Client (in a new terminal)
```bash
./client localhost
```

## Usage

### Server
1. Run the server script - it will start listening on port 3490
2. Wait for client connections
3. When clients connect and provide usernames, they enter a chat room
4. The server displays all chat activity with usernames and timestamps
5. The server monitors and logs all messages but does not send messages itself
6. Users are notified when others join or leave the chat
7. Press Ctrl+C to shut down the server

### Client
1. Run the client script with the server hostname/IP
   - Examples: `localhost`, `127.0.0.1`, `192.168.1.100`
2. You'll see a welcome message when connected
3. Enter your name/identifier when prompted
4. After authentication, you enter a chat room with other connected users
5. Messages you type are broadcast to all other connected clients
6. You'll see messages from other users with their usernames and timestamps
7. You'll be notified when other users join or leave
8. Type `quit` to disconnect from the server

## Chat Commands

- **quit** - End the chat session and disconnect

## Security Features

### User Identification
- Each client must provide a username/identifier upon connection
- Username is encrypted during transmission
- Server displays the username with each message for context
- Helps identify participants in multi-client scenarios

### Message Encryption
- All messages are encrypted using XOR cipher before transmission
- Encryption key is shared between client and server (defined in source code)
- Key: `NetworksCS522Key`
- Both client and server automatically encrypt outgoing messages and decrypt incoming messages
- Provides basic confidentiality for chat messages over the network

### Timestamps
- Each received message is automatically timestamped
- Timestamp format: `[YYYY-MM-DD HH:MM:SS]`
- Example: `[2025-10-25 14:30:45] Server: Hello!`
- Timestamps show when messages were received, not when they were sent
- Helps track conversation flow and message timing

## Network Details

- **Port**: 3490 (defined in both server.c and client.c)
- **Protocol**: TCP (SOCK_STREAM)
- **Address Family**: IPv4 and IPv6 compatible
- **Buffer Size**: 1024 bytes

## Connection Examples

### Same Machine
```powershell
.\start_client.ps1 localhost
```

### Different Machine (same network)
```powershell
.\start_client.ps1 192.168.1.100
```

### Using Hostname
```powershell
.\start_client.ps1 my-server-name
```

## How It Works

### Server Operation
1. Creates a socket and binds to port 3490
2. Listens for incoming connections
3. Uses `select()` to monitor all client connections simultaneously (single-process, non-blocking)
4. Maintains a list of up to 10 connected clients with their usernames
5. When a message arrives from any client, broadcasts it to all other connected clients
6. Handles client disconnections and notifies remaining users

### Client Operation
1. Connects to the specified server hostname/IP on port 3490
2. Receives welcome message from server
3. Sends username to server for identification
4. Uses `select()` to monitor both stdin and the socket simultaneously
5. Sends user input to server, which broadcasts to all other clients
6. Displays received messages from other users with timestamps
7. Maintains connection until "quit" is typed or server disconnects

## Technical Notes

### select() System Call
Both server and client use `select()` with file descriptor sets to enable non-blocking I/O:
- **Server**: Monitors listener socket + all connected client sockets
- **Client**: Monitors stdin for user input + socket for incoming messages
- Allows simultaneous handling of multiple connections/events without threads

### Server Architecture
- **Single-process design**: Uses `select()` instead of `fork()` for scalability
- Maintains array of up to 10 active client connections
- Tracks username for each connected client
- Broadcasts messages from one client to all others
- No inter-process communication needed (all in one process)

## Troubleshooting


### "Connection refused" Error
- Ensure the server is running before starting the client
- Check that port 3490 is not blocked by firewall
- Verify the hostname/IP address is correct

### "Address already in use" Error
- Another instance of the server is already running
- Wait a few seconds for the OS to release the port
- Or kill the existing server process


## Port Configuration

To change the port, modify the `PORT` definition in both files:
```c
#define PORT "3490"  // Change to your desired port
```
