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

#include "messages.h"

#define SERVER_IP "192.168.101.10"
#define MESSAGE_LEN 49
#define SHA_LEN 32
#define RESPONSE_LEN 8


void sha256(uint64_t *v, unsigned char out_buff[SHA256_DIGEST_LENGTH])
{
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, v, sizeof(v));
	SHA256_Final(out_buff, &sha256);
}

// *big_endian_arr is an array of bytes, lil_endian_arr is a pointer to an array of the same size
void read_client_msg(uint8_t *big_endian_arr, uint8_t *response_arr)
{
	int num = 1;	
	if(*(char *)&num == 1)
	{
		//printf("Little-Endian\n");
	}
	else
	{
		//printf("Big-Endian\n");
	}
	/*
	hash = big_endian_arr[0:32];
	start = big_endain_arr[32:40];
	end = big_endian_arr[40:48];
	priority = big_endain_arr[48];
	*/
/*
	//Test
	uint8_t outbuf[SHA_LEN] = {0};
	uint64_t f = 1;
	char foo[8];
	sprintf(foo, "%" PRIu64, f);
	sha256(&f, outbuf);

	printf("0x");
	int j;
	for (j = 0; j < 32; j++)
	{
		printf("%02x", outbuf[j]);
	}
	printf("\n");
*/	

	//Process input
	uint8_t sha256_test[SHA_LEN] = {0};
	int i;
	//printf("0x");
	for (i = 0; i < 32; i++)
	{
		//printf("%02x", big_endian_arr[i]);
	}
	//printf(" ");

	uint64_t start = 0;
	for (i = 32; i < 40; i++)
	{
		start = start | (((uint64_t)big_endian_arr[i]) << (8 * (39 - i)));
	}
		
		//printf(" %lu ", start);
		
			
	uint64_t end = 0;
	for (i = 40; i < 48; i++)
	{
		end = end | (((uint64_t)big_endian_arr[i]) << (8 * (47 - i)));
	}
	//printf("%lu ", end);
			
	//printf("%d\n", big_endian_arr[48]);

	int sha_good = 1;
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

int main(int argc, char *argv[])
{	int PORT;
	if (argc > 1)
		PORT = atoi(argv[1]);

	// Create a socketaddr_in to hold server socket address data, and zero it
	struct sockaddr_in server_addr = {0};

	// htons takes host byte ordering to short value in network byte ordering
	// htonl same as htons, except takes it to network long instead of short
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

	// TCP Socket Creation
	int listen_sock;
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
	int wait_size = 65535; // number of clients that we queue before connection is busy
	if (listen(listen_sock, wait_size) < 0)
	{
		perror("couldn't open socket for listening");
		return 1;
	}

	printf("\nserving on %s:%d\n", inet_ntoa(server_addr.sin_addr), PORT);

	struct sockaddr_in client_addr;
	uint client_address_len = 0;
	while (1)
	{
		int sock;
		if ((sock = accept(listen_sock, (struct sockaddr *) &client_addr, &client_address_len)) < 0)
		{
			perror("couldn't open a socket to accept data");
			return 1;
		}

		int n = 0;
		int len = 0, maxlen=MESSAGE_LEN;
		uint8_t buffer[MESSAGE_LEN] = {0};
		uint8_t *pbuffer = buffer;
		uint8_t response[RESPONSE_LEN] = {0};

		char *client_ip = inet_ntoa(client_addr.sin_addr);
		//uint8_t lil_endian[MESSAGE_LEN] = {0};

		//printf("\nclient connected with ip address: %s\n", client_ip);
		while ((n = recv(sock, pbuffer, maxlen, 0)) > 0)
		{
			pbuffer += n;
			maxlen -= n;
			len += n;

			read_client_msg(buffer, response);
			send(sock, response, RESPONSE_LEN, 0);
		}
		close(sock);
	}
	close(listen_sock);
	return 0;
}

