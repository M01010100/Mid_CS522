# Quick Start Guide - Chat Relay System


### Step 1: Open Two PowerShell terminals

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

