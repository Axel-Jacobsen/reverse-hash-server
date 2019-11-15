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


struct __attribute__((packed)) hashBuffer {
	uint64_t start;
	uint64_t end;
	uint64_t first_num;
	uint64_t second_num;
	uint64_t third_num;
	uint64_t fourth_num;
	uint8_t priority : 4;	
}hash;


void sha256(uint64_t *v, unsigned char out_buff[SHA256_DIGEST_LENGTH])
{
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, v, sizeof(v));
	SHA256_Final(out_buff, &sha256);
}

void initStruct(uint8_t *buffer)
{

	uint8_t i;
	uint64_t start = 0;
	for (i = 32; i < 40; i++)
	{
		start = start | (((uint64_t)buffer[i]) << (8 * (39 - i)));
	}		
	uint64_t end = 0;
	for (i = 40; i < 48; i++)
	{
		end = end | (((uint64_t)buffer[i]) << (8 * (47 - i)));
	}

	uint64_t first_num = 0;
	for (i = 0; i < 8; i++)
	{
		first_num = first_num | (((uint64_t)buffer[i]) << (8 * (7 - i)));
	}
	

	uint64_t second_num = 0;
	for (i = 8; i < 16; i++)
	{
		second_num = second_num | (((uint64_t)buffer[i]) << (8 * (15 - i)));
	}


	uint64_t third_num = 0;
	for (i = 16; i < 24; i++)
	{
		third_num = third_num | (((uint64_t)buffer[i]) << (8 * (23 - i)));
	}


	uint64_t fourth_num = 0;
	for (i = 24; i < 32; i++)
	{
		fourth_num = fourth_num | (((uint64_t)buffer[i]) << (8 * (31 - i)));
	}
		
	//printf("priority: %d\n", (int)buffer[48]);	
	//printf("hashing: %lld\n", hashing);
	
	hash.start = start;
	hash.end = end; 
	hash.first_num = first_num;
	hash.second_num = second_num;
	hash.third_num = third_num;
	hash.fourth_num = fourth_num;
	hash.priority = buffer[48];
	//printf("%d\n", (int)hash.priority);
	//memcpy((void *)hash.hash, buffer, SHA_LEN);
}

// *big_endian_arr is an array of bytes, response_arr is a pointer to an array of the same size
void rev_hash(uint8_t *response_arr)
{

	uint8_t sha_good = 1;
	uint8_t sha256_test[SHA_LEN] = {0};
	uint64_t k;
	uint64_t k_conv;
	
	for(k = hash.start; k < hash.end; k++){
		sha_good = 1;
		sha256(&k, sha256_test);
		
		uint8_t i;
		uint64_t first_num_test = 0;
		for (i = 0; i < 8; i++)
		{
			first_num_test = first_num_test | (((uint64_t)sha256_test[i]) << (8 * (7 - i)));
		}
	

		uint64_t second_num_test = 0;
		for (i = 8; i < 16; i++)
		{
			second_num_test = second_num_test | (((uint64_t)sha256_test[i]) << (8 * (15 - i)));
		}


		uint64_t third_num_test = 0;
		for (i = 16; i < 24; i++)
		{
			third_num_test = third_num_test | (((uint64_t)sha256_test[i]) << (8 * (23 - i)));
		}


		uint64_t fourth_num_test = 0;
		for (i = 24; i < 32; i++)
		{
			fourth_num_test = fourth_num_test | (((uint64_t)sha256_test[i]) << (8 * (31 - i)));
		}
	
		//printf("hash_first: %ld, test_first: %ld\n", hash.first_num, first_num_test);
		//printf("hash_second: %ld, test_second: %ld\n", hash.second_num, second_num_test);
		//printf("hash_third: %ld, test_third: %ld\n", hash.third_num, third_num_test);
		//printf("hash_fourth: %ld, test_fourth: %ld\n", hash.fourth_num, fourth_num_test);	
		//printf("-------------------------------------------------------------------------------\n");	
		if(hash.first_num != first_num_test && hash.second_num != second_num_test && hash.third_num != third_num_test && hash.fourth_num != fourth_num_test){
			sha_good = 0;
		}
	
		
		
		
		
		if(sha_good){
			k_conv = htobe64(k);
			memcpy(response_arr, &k_conv, sizeof(k_conv));
			break;
		}
	}
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
		while ((n = recv(sock, pbuffer, maxlen, 0)) > 0)
		{

				
		
			pbuffer += n;
			maxlen -= n;
			len += n;
		


			initStruct(buffer);
			rev_hash(response);
			send(sock, response, RESPONSE_LEN, 0);
	}
		close(sock);
	}
	close(listen_sock);
	return 0;
}

