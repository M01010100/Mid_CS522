/* ** server_broadcast.c -- a chat server that broadcasts messages to all clients
*/ 

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <arpa/inet.h> 
#include <time.h> 

#define PORT "3490" // the port users will be connecting to 
#define BACKLOG 10   // how many pending connections queue will hold 
#define MAX_CLIENTS 10
#define MAXDATASIZE 1024

// Encryption key - must match client
#define ENCRYPTION_KEY "NetworksCS522Key"

// Client structure
typedef struct {
	int fd;
	char username[64];
	struct sockaddr_storage addr;
} client_t;

client_t clients[MAX_CLIENTS];
int client_count = 0;

// Simple XOR encryption/decryption function
void xor_encrypt_decrypt(char *data, int length, const char *key) {
	int key_len = strlen(key);
	for (int i = 0; i < length; i++) {
		data[i] ^= key[i % key_len];
	}
}

// Get current timestamp as string
void get_timestamp(char *buffer, size_t size) {
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	strftime(buffer, size, "[%Y-%m-%d %H:%M:%S]", t);
}

// get sockaddr, IPv4 or IPv6: 
void *get_in_addr(struct sockaddr *sa) 
{ 
	if (sa->sa_family == AF_INET) { 
		return &(((struct sockaddr_in*)sa)->sin_addr); 
	} 
	return &(((struct sockaddr_in6*)sa)->sin6_addr); 
}

// Broadcast message to all clients except sender
void broadcast_message(const char *message, int sender_fd, const char *sender_name) {
	char buf[MAXDATASIZE];
	char timestamp[64];
	get_timestamp(timestamp, sizeof(timestamp));
	
	// Format: [timestamp] Username: message
	snprintf(buf, sizeof(buf), "%s %s: %s", timestamp, sender_name, message);
	
	int msg_len = strlen(buf);
	xor_encrypt_decrypt(buf, msg_len, ENCRYPTION_KEY);
	
	for (int i = 0; i < client_count; i++) {
		if (clients[i].fd != sender_fd && clients[i].fd != -1) {
			send(clients[i].fd, buf, msg_len, 0);
		}
	}
}

// Add client to list
int add_client(int fd, struct sockaddr_storage *addr) {
	if (client_count >= MAX_CLIENTS) {
		return -1;
	}
	
	clients[client_count].fd = fd;
	clients[client_count].addr = *addr;
	clients[client_count].username[0] = '\0';
	client_count++;
	return client_count - 1;
}

// Remove client from list
void remove_client(int index) {
	if (index < 0 || index >= client_count) return;
	
	printf("%s disconnected\n", clients[index].username);
	close(clients[index].fd);
	
	// Shift remaining clients down
	for (int i = index; i < client_count - 1; i++) {
		clients[i] = clients[i + 1];
	}
	client_count--;
}

// Find client index by fd
int find_client(int fd) {
	for (int i = 0; i < client_count; i++) {
		if (clients[i].fd == fd) {
			return i;
		}
	}
	return -1;
}

int main(void) 
{ 
	int listener;  // listening socket descriptor
	int newfd;     // newly accepted socket descriptor
	struct sockaddr_storage remoteaddr; // client address
	socklen_t addrlen;
	
	char buf[MAXDATASIZE];
	int nbytes;
	
	char remoteIP[INET6_ADDRSTRLEN];
	
	int yes=1;
	int i, rv;
	
	fd_set master;   // master file descriptor list
	fd_set read_fds; // temp file descriptor list for select()
	int fdmax;       // maximum file descriptor number
	
	struct addrinfo hints, *ai, *p;
	
	// Initialize client list
	for (i = 0; i < MAX_CLIENTS; i++) {
		clients[i].fd = -1;
	}
	
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	
	// Get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != NULL; p = p->ai_next) {
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) { 
			continue;
		}
		
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		
		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}
		
		break;
	}
	
	if (p == NULL) {
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}
	
	freeaddrinfo(ai);
	
	if (listen(listener, BACKLOG) == -1) {
		perror("listen");
		exit(3);
	}
	
	// Add the listener to the master set
	FD_SET(listener, &master);
	fdmax = listener;
	
	printf("=== Chat Server Started ===\n");
	printf("Listening on port %s\n", PORT);
	printf("Waiting for connections...\n\n");
	
	// Main loop
	while(1) {
		read_fds = master;
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}
		
		// Run through the existing connections looking for data to read
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == listener) {
					// Handle new connections
					addrlen = sizeof remoteaddr;
					newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
					
					if (newfd == -1) {
						perror("accept");
					} else {
						if (add_client(newfd, &remoteaddr) == -1) {
							char *msg = "Server is full. Please try again later.\n";
							send(newfd, msg, strlen(msg), 0);
							close(newfd);
						} else {
							FD_SET(newfd, &master);
							if (newfd > fdmax) {
								fdmax = newfd;
							}
							
							inet_ntop(remoteaddr.ss_family,
								get_in_addr((struct sockaddr*)&remoteaddr),
								remoteIP, INET6_ADDRSTRLEN);
							
							printf("New connection from %s on socket %d\n", remoteIP, newfd);
							
							// Send welcome message
							char welcome[] = "=== Connected to Chat Server ===\nType your messages and press Enter. Type 'quit' to exit.\n";
							send(newfd, welcome, strlen(welcome), 0);
						}
					}
				} else {
					// Handle data from a client
					int client_idx = find_client(i);
					if (client_idx == -1) continue;
					
					if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
						// Connection closed or error
						if (nbytes == 0) {
							printf("%s disconnected\n", 
								clients[client_idx].username[0] ? clients[client_idx].username : "Unknown");
						} else {
							perror("recv");
						}
						close(i);
						FD_CLR(i, &master);
						remove_client(client_idx);
					} else {
						// We got some data from a client
						buf[nbytes] = '\0';
						xor_encrypt_decrypt(buf, nbytes, ENCRYPTION_KEY);
						
						// If username not set, this is the username
						if (clients[client_idx].username[0] == '\0') {
							strncpy(clients[client_idx].username, buf, sizeof(clients[client_idx].username) - 1);
							clients[client_idx].username[sizeof(clients[client_idx].username) - 1] = '\0';
							
							printf("User '%s' joined the chat\n", clients[client_idx].username);
							
							// Send acknowledgment
							char ack[256];
							snprintf(ack, sizeof(ack), "Welcome, %s! You are now connected. There are %d user(s) online.", 
								clients[client_idx].username, client_count);
							int ack_len = strlen(ack);
							xor_encrypt_decrypt(ack, ack_len, ENCRYPTION_KEY);
							send(i, ack, ack_len, 0);
							
							// Notify other users
							char join_msg[256];
							snprintf(join_msg, sizeof(join_msg), "%s has joined the chat\n", clients[client_idx].username);
							broadcast_message(join_msg, i, "Server");
						} else {
							// Regular message - check for quit
							if (strncmp(buf, "quit", 4) == 0) {
								printf("%s is leaving the chat\n", clients[client_idx].username);
								
								// Notify other users
								char leave_msg[256];
								snprintf(leave_msg, sizeof(leave_msg), "%s has left the chat\n", clients[client_idx].username);
								broadcast_message(leave_msg, i, "Server");
								
								close(i);
								FD_CLR(i, &master);
								remove_client(client_idx);
							} else {
								// Broadcast to all other clients
								printf("[%s]: %s", clients[client_idx].username, buf);
								broadcast_message(buf, i, clients[client_idx].username);
							}
						}
					}
				}
			}
		}
	}
	
	return 0;
}
