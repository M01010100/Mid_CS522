# Quick Start Guide - Chat Relay System

## Fastest Way to Get Started

### Step 1: Open TWO PowerShell terminals

**Terminal 1 - Server:**
```powershell
cd C:\Users\matth\Classes\CS_522_Networks\Midterm
.\start_server.ps1
```

**Terminal 2 - Client:**
```powershell
cd C:\Users\matth\Classes\CS_522_Networks\Midterm
.\start_client.ps1 localhost
```

### Step 2: Start Chatting!

In either terminal, type a message and press Enter. It will appear in the other terminal.

Type `quit` in either terminal to end the session.

---

## Testing on Same Machine

1. **Start server**: `.\start_server.ps1`
2. **Start client**: `.\start_client.ps1 localhost`
3. Type messages in either window
4. Type `quit` to disconnect

---

## Testing Across Network

### On Server Machine:
```powershell
.\start_server.ps1
# Note your IP address: ipconfig
```

### On Client Machine:
```powershell
.\start_client.ps1 192.168.1.XXX
# Replace XXX with server's IP address
```

---

## Common Commands

| Command | Description |
|---------|-------------|
| `.\start_server.ps1` | Start the chat server |
| `.\start_client.ps1 HOST` | Connect to server at HOST |
| `quit` | End chat session |
| `Ctrl+C` | Force stop server/client |

---

## Example Chat Session

**Server Terminal:**
```
=== Chat Server Startup ===
Compiling server.c...
Compilation successful!
Starting chat server on port 3490...
server: waiting for connections...
server: got connection from ::1
Chat session started with ::1
Type messages to send to client (quit to end session):

Hello from server!
Client: Hi from client!
How are you?
Client: I'm good, thanks!
quit
Ending chat session
```

**Client Terminal:**
```
=== Chat Client Startup ===
Compiling client.c...
Compilation successful!
Connecting to server at localhost on port 3490...
client: connected to ::1
=== Connected to Chat Server ===
Type your messages and press Enter. Type 'quit' to exit.

Server: Hello from server!
Hi from client!
Server: How are you?
I'm good, thanks!
Server has ended the chat.
```

---

## Troubleshooting

**Problem:** Scripts won't run  
**Solution:** Enable script execution:
```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

**Problem:** WSL not found  
**Solution:** Install WSL:
```powershell
wsl --install
```
Then restart your computer.

**Problem:** Connection refused  
**Solution:** Make sure server is running first!

**Problem:** Port already in use  
**Solution:** Only one server can run at a time. Close other server instances.


## Architecture

```
┌─────────────────┐         TCP           ┌─────────────────┐
│     SERVER      │◄─────────────────────►│     CLIENT      │
│   Port 3490     │    Socket Connection  │                 │
│                 │                       │                 │
│  - Listens      │                       │  - Connects     │
│  - Accepts      │                       │  - Sends/Recv   │
│  - Forks child  │                       │  - Select I/O   │
│  - Select I/O   │                       │                 │
└─────────────────┘                       └─────────────────┘
```

---


For detailed documentation, see `README.md`
