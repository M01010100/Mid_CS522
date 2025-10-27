# Compilation Instructions

```bash
gcc -o <program_name> <program_name>.c
```

| Program | Command |
|---------|---------|
| TCP Server | `gcc -o server server.c` |
| TCP Client | `gcc -o client client.c` |
| UDP Listener | `gcc -o listener listener.c` |
| UDP Talker | `gcc -o talker talker.c` |

```bash
#server
./server
./listener

# client - samedevice
./client localhost
./talker localhost "This is a UDP message"
```
