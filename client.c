/* ** client.c -- a stream socket client demo 
*/ 

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <time.h> 

#include <arpa/inet.h> 

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 1024 // max number of bytes we can get at once 

// Encryption key - must match server
#define ENCRYPTION_KEY "NetworksCS522Key"

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

int main(int argc, char *argv[]) 
{ 
	int sockfd, numbytes;  
	char buf[MAXDATASIZE]; 
	struct addrinfo hints, *servinfo, *p; 
	int rv; 
	char s[INET6_ADDRSTRLEN]; 

	if (argc != 2) { 
	    fprintf(stderr,"usage: client hostname\n"); 
	    exit(1); 
	} 

	memset(&hints, 0, sizeof hints);  // prepare hints structure
	hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) { 
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); 
		return 1; 
	} 

	// loop through all the results and connect to the first we can 
	for(p = servinfo; p != NULL; p = p->ai_next) { 
		if ((sockfd = socket(p->ai_family, p->ai_socktype, 
				p->ai_protocol)) == -1) { 
			perror("client: socket"); 
			continue; 
		} 
// Convert the IP to a string and print it
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), 
			s, sizeof s); 
		printf("client: attempting connection to %s\n", s); 
// Try to connect
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) { 
			perror("client: connect"); 
			close(sockfd); 
			continue; 
		} 

		break; 
	} 

	if (p == NULL) { 
		fprintf(stderr, "client: failed to connect\n"); 
		return 2; 
	} 

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), 
			s, sizeof s); 
	printf("client: connected to %s\n", s); 

	freeaddrinfo(servinfo); // all done with this structure 

	// Receive welcome message
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) { 
	    perror("recv"); 
	    exit(1); 
	} 

	buf[numbytes] = '\0'; 
	printf("%s", buf);
	
	// Prompt for username
	char username[64];
	printf("Enter your name/identifier: ");
	fflush(stdout);
	if (fgets(username, sizeof(username), stdin) == NULL) {
		fprintf(stderr, "Failed to read username\n");
		close(sockfd);
		return 1;
	}
	
	// Remove newline from username
	username[strcspn(username, "\n")] = '\0';
	
	// Send encrypted username to server
	int username_len = strlen(username);
	xor_encrypt_decrypt(username, username_len, ENCRYPTION_KEY);
	if (send(sockfd, username, username_len, 0) == -1) {
		perror("send username");
		close(sockfd);
		return 1;
	}
	
	// Wait for server acknowledgment
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		perror("recv");
		close(sockfd);
		return 1;
	}
	buf[numbytes] = '\0';
	xor_encrypt_decrypt(buf, numbytes, ENCRYPTION_KEY);
	printf("%s\n", buf); 

	// Chat loop - bidirectional communication
	fd_set master_fds, read_fds;
	int fdmax;
	
	FD_ZERO(&master_fds);
	FD_SET(STDIN_FILENO, &master_fds);
	FD_SET(sockfd, &master_fds);
	fdmax = sockfd;
	
	printf("\n"); // Add newline after welcome message
	
	while(1) {
		read_fds = master_fds;
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			break;
		}
		
		// Check if server sent data
		if (FD_ISSET(sockfd, &read_fds)) {
			numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0);
			if (numbytes <= 0) {
				if (numbytes == 0) {
					printf("\nServer disconnected\n");
				} else {
					perror("recv");
				}
				break;
			}
			buf[numbytes] = '\0';
			
			// Decrypt the received message
			xor_encrypt_decrypt(buf, numbytes, ENCRYPTION_KEY);
			
			// Display the message (already includes timestamp and username from server)
			printf("%s", buf);
			fflush(stdout);
		}
		
		// Check if user typed something
		if (FD_ISSET(STDIN_FILENO, &read_fds)) {
			// User typed something
			char plaintext[MAXDATASIZE];
			unsigned char ciphertext[MAXDATASIZE + 16];
			
			if (fgets(plaintext, MAXDATASIZE, stdin) == NULL) {
				break;
			}
			
			// Remove newline
			plaintext[strcspn(plaintext, "\n")] = '\0';
			
			// Check for quit
			if (strcmp(plaintext, "quit") == 0) {
				printf("Disconnecting...\n");
				break;
			}
			
			// Encrypt and send
			int plaintext_len = strlen(plaintext);
			int ciphertext_len = aes_encrypt((unsigned char*)plaintext, plaintext_len, ciphertext);
			
			if (ciphertext_len < 0) {
				fprintf(stderr, "Encryption failed\n");
				continue;
			}
			
			if (send(sockfd, ciphertext, ciphertext_len, 0) == -1) {
				perror("send");
				break;
			}
		}
	}

	close(sockfd); 

	return 0; 
}