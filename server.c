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
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#define PORT "3490" // the port users will be connecting to 
#define BACKLOG 10   // how many pending connections queue will hold 
#define MAX_CLIENTS 10
#define MAXDATASIZE 1024

// Remove old XOR key:
// #define ENCRYPTION_KEY "NetworksCS522Key"

// Add AES-256 key (32 bytes) and IV (16 bytes)
// These should be securely exchanged in production (e.g., via Diffie-Hellman)
static const unsigned char AES_KEY[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

static const unsigned char AES_IV[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

// Client structure
typedef struct {
	int fd;
	char username[64];
	struct sockaddr_storage addr;
} client_t;

client_t clients[MAX_CLIENTS];
int client_count = 0;

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
    char plaintext[MAXDATASIZE];
    unsigned char ciphertext[MAXDATASIZE + 16]; // Extra space for padding
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));
    
    // Format: [timestamp] Username: message
    int plaintext_len = snprintf(plaintext, sizeof(plaintext), "%s %s: %s", 
                                  timestamp, sender_name, message);
    
    // Encrypt the message
    int ciphertext_len = aes_encrypt((unsigned char*)plaintext, plaintext_len, ciphertext);
    if (ciphertext_len < 0) {
        fprintf(stderr, "Encryption failed\n");
        return;
    }
    
    // Broadcast to all clients except sender
    for (int i = 0; i < client_count; i++) {
        if (clients[i].fd != sender_fd && clients[i].fd != -1) {
            send(clients[i].fd, ciphertext, ciphertext_len, 0);
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

// void xor_encrypt_decrypt(char *data, int length, const char *key) { ... }

// Add AES encryption function
int aes_encrypt(unsigned char *plaintext, int plaintext_len,
                unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    // Create and initialize the context
    if (!(ctx = EVP_CIPHER_CTX_new()))
        return -1;

    // Initialize encryption operation with AES-256-CBC
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, AES_KEY, AES_IV))
        return -1;

    // Encrypt the plaintext
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        return -1;
    ciphertext_len = len;

    // Finalize encryption (handles padding)
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        return -1;
    ciphertext_len += len;

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

// Add AES decryption function
int aes_decrypt(unsigned char *ciphertext, int ciphertext_len,
                unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;

    // Create and initialize the context
    if (!(ctx = EVP_CIPHER_CTX_new()))
        return -1;

    // Initialize decryption operation with AES-256-CBC
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, AES_KEY, AES_IV))
        return -1;

    // Decrypt the ciphertext
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        return -1;
    plaintext_len = len;

    // Finalize decryption (removes padding)
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        return -1;
    plaintext_len += len;

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

// Encrypt with random IV and prepend IV to output
int aes_encrypt_with_random_iv(unsigned char *plaintext, int plaintext_len,
                                unsigned char *output) {
    unsigned char iv[16];
    
    // Generate random IV
    if (RAND_bytes(iv, sizeof(iv)) != 1)
        return -1;
    
    // Copy IV to output (first 16 bytes)
    memcpy(output, iv, 16);
    
    // Encrypt with random IV
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, AES_KEY, iv);
    
    int len, ciphertext_len;
    EVP_EncryptUpdate(ctx, output + 16, &len, plaintext, plaintext_len);
    ciphertext_len = len;
    
    EVP_EncryptFinal_ex(ctx, output + 16 + len, &len);
    ciphertext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    return ciphertext_len + 16; // Total: IV + ciphertext
}

// Decrypt with IV extraction
int aes_decrypt_with_iv(unsigned char *input, int input_len,
                         unsigned char *plaintext) {
    // Extract IV (first 16 bytes)
    unsigned char iv[16];
    memcpy(iv, input, 16);
    
    // Decrypt (skip first 16 bytes)
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, AES_KEY, iv);
    
    int len, plaintext_len;
    EVP_DecryptUpdate(ctx, plaintext, &len, input + 16, input_len - 16);
    plaintext_len = len;
    
    EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    plaintext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    return plaintext_len;
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
	// Get address info
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	// Bind to the first available address
	for(p = ai; p != NULL; p = p->ai_next) {
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) { 
			continue;
		}
		// Allow reuse of the address
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		
		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}
		
		break;
	}
	// Check if bind was successful
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
						
						unsigned char decrypted[MAXDATASIZE];
						int decrypted_len = aes_decrypt((unsigned char*)buf, nbytes, decrypted);
						if (decrypted_len < 0) {
						    fprintf(stderr, "Decryption failed\n");
						    continue;
						}
						decrypted[decrypted_len] = '\0';
						
						// If username not set, this is the username
						if (clients[client_idx].username[0] == '\0') {
						    strncpy(clients[client_idx].username, (char*)decrypted, 
								    sizeof(clients[client_idx].username) - 1);
						    clients[client_idx].username[sizeof(clients[client_idx].username) - 1] = '\0';
						    
						    printf("User '%s' joined the chat\n", clients[client_idx].username);
						    
						    // Send acknowledgment
						    char ack_plain[256];
						    unsigned char ack_cipher[256 + 16];
						    int ack_plain_len = snprintf(ack_plain, sizeof(ack_plain), 
						        "Welcome, %s! You are now connected. There are %d user(s) online.", 
						        clients[client_idx].username, client_count);
						    
						    int ack_cipher_len = aes_encrypt((unsigned char*)ack_plain, ack_plain_len, ack_cipher);
						    if (ack_cipher_len > 0) {
						        send(i, ack_cipher, ack_cipher_len, 0);
						    }
						    
						    // Notify other users
						    char join_msg[256];
						    snprintf(join_msg, sizeof(join_msg), "%s has joined the chat\n", 
						             clients[client_idx].username);
						    broadcast_message(join_msg, i, "Server");
						} else {
						    // Regular message - check for quit
						    if (strncmp((char*)decrypted, "quit", 4) == 0) {
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
						        printf("[%s]: %s", clients[client_idx].username, decrypted);
						        broadcast_message((char*)decrypted, i, clients[client_idx].username);
						    }
						}
					}
				}
			}
		}
	}
	
	return 0;
}
