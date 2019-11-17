#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include "messages.h"

#define SERVER_IP "192.168.101.10"
#define MESSAGE_LEN 49
#define SHA_LEN 32
#define RESPONSE_LEN 8

typedef struct Socket_info{
        int sock;
}Socket_info;

void sha256(uint64_t *v, unsigned char out_buff[SHA256_DIGEST_LENGTH])
{
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, v, sizeof(v));
	SHA256_Final(out_buff, &sha256);
}

void rev_hash(uint8_t *big_endian_arr, uint8_t *response_arr)
{
        uint64_t start = 0;
	int i = 0;	

        for (i = 32; i < 40; i++)
        {
                start = start | (((uint64_t)big_endian_arr[i]) << (8 * (39 - i)));
        }

        uint64_t end = 0;
        for (i = 40; i < 48; i++)
        {
                end = end | (((uint64_t)big_endian_arr[i]) << (8 * (47 - i)));
        }

        uint8_t sha_good = 1;
        uint8_t sha256_test[SHA_LEN] = {0};
        uint64_t k;
        uint64_t k_conv;

        for(k = start; k < end; k++){
                sha_good = 1;
                sha256(&k, sha256_test);

                for(i = 0; i < 32; i++){
                        if(big_endian_arr[i] != sha256_test[i]){
                                sha_good = 0;
                                break;
                        }
                }
                if(sha_good){
                        k_conv = htobe64(k);
                        memcpy(response_arr, &k_conv, sizeof(k_conv));
                        break;
                }
        }
}

void* request_handler(void* socket_info){
	int sock = ((Socket_info*)socket_info)->sock;
	free(socket_info);
	int n = 0;
        int len = 0, maxlen=MESSAGE_LEN;
        uint8_t buffer[MESSAGE_LEN] = {0};
        uint8_t *pbuffer = buffer;
        uint8_t response[RESPONSE_LEN] = {0};
        while ((n = recv(sock, pbuffer, maxlen, 0)) > 0)
        {
        	pbuffer += n;
                maxlen -= n;
                len += n;
                rev_hash(buffer, response);
                send(sock, response, RESPONSE_LEN, 0);
  	}	
	close(sock);
	printf("Exiting thread!\n");
}


int main(int argc, char *argv[])
{	
	int PORT;
	if (argc > 1)
		PORT = atoi(argv[1]);

	// Create a socketaddr_in to hold server socket address data, and zero it
	struct sockaddr_in server_addr = {0};

	// htons takes host byte ordering to short value in network byte ordering
	// htonl same as htons, except takes it to network long instead of short
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

	int listen_sock;
	// TCP Socket Creation
	if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("could not create listen socket\n");
		return 1;
	}

	// Bind to TCP socket
	if ((bind(listen_sock, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0)
	{
		perror("could not bind to socket");
		return 1;
	}

	// Listen on TCP socket
	// number of clients that we queue before connection is busy
	uint16_t wait_size = 1 << 15;
	if (listen(listen_sock, wait_size) < 0)
	{
		perror("couldn't open socket for listening");
		return 1;
	}
	printf("\nServing on %s:%d\n", inet_ntoa(server_addr.sin_addr), PORT);
	struct sockaddr_in client_addr;
	uint client_address_len = 0;
	pthread_t rh;
	int sock;
	Socket_info socket_info;
	while (1)
	{
		if ((sock = accept(listen_sock, (struct sockaddr *) &client_addr, &client_address_len)) < 0)
		{
			perror("couldn't open a socket to accept data");
			return 1;
		}

		printf("Creating thread to handle request!\n");
		Socket_info* socket_info = (Socket_info*)malloc(sizeof(Socket_info));
		socket_info->sock = sock;
		if(pthread_create(&rh, NULL, request_handler, (void*)socket_info) != 0){
                	printf("Error creating a new thread going up\n");
                	exit(1);
		}
        }
		
	close(listen_sock);
	return 0;
}

