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
#define HEAP_SIZE 16

//Some setup for the priority heap
struct request{
	uint8_t package[MESSAGE_LEN];
	uint8_t priority;
	int socket;
};

struct heap{
	uint size;
	uint count;
	request* data;
};

request initReq(struct request* r, uint8_t* package, int sock){
	memcpy(r->package, package, MESSAGE_LEN*sizeof(uint8_t));
	r->socket = sock;
	r->priority = package[48];
}

heap initHeap(struct heap* h, int size){
	h->data = malloc(size*sizeof(struct request));
	h->size = size;
	h->count = 0;
}

void heap_push(struct heap* h, struct request value){
	/*Potential rezising of the heap
	if (h->count == h->size){
		h->size <<= 1;
		h->data = realloc(h->data, sizeof(struct request) * h->size)
		if(!h->data) _exit(1); //Exit if the memory allocation fails
	}
	*/
	int i, j;
	for (i = 0; i < h->count; i++){
		if (value->priority > h->data[i]->priority) {
			for (j = h->count; j>i; j--){
				h->data[j] = h->data[j-1];
			}
			h->data[i] = value;
			h->count++;
			return;
		}
	}
	h->data[i] = value;
	h->count++;
}

request heap_pop(struct heap* h){
	request temp = h->data[0];
	h->count--;
	int i;
	for (i = 0; i < h->count; i++){
		h->data[i] = h->data[i+1];
	}
	h->data[h->count] = null;
	/* Potebtial resizing of the heap
	if ((h->count <= (h->size >> 2)) && (h->size > HEAP_SIZE)){
		h->size >>=1;
		h->data = realloc(h->data, sizeof(type) * h->size);
		if (!h->data) _exit(1); //Exit if the memory allocation fails
	}
	*/
	return temp;
}

void sha256(uint64_t *v, unsigned char out_buff[SHA256_DIGEST_LENGTH])
{
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, v, sizeof(v));
	SHA256_Final(out_buff, &sha256);
}

// *big_endian_arr is an array of bytes, response_arr is a pointer to an array of the same size
void rev_hash(uint8_t *big_endian_arr, uint8_t *response_arr)
{
	uint8_t i;
	uint64_t start = 0;
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

		int listFilled = 0;
		heap prioList;
		initHeap(&prioList, HEAP_SIZE);
		while ((n = recv(sock, pbuffer, maxlen, 0)) > 0)
		{
			pbuffer += n;
			maxlen -= n;
			len += n;

			request req;
			initReq(&req, buffer, sock);
			heap_push(&prioList, req);
			listFilled++;

			if (listFilled > 9){
				request highestPrio = heap_pop(prioList);
				rev_hash(highestPrio->package, response);     //rev_hash(buffer, response);
				send(highestPrio->socket, response, RESPONSE_LEN, 0);		//send(sock, response, RESPONSE_LEN, 0);
			}
		}
		close(sock);
	}
	close(listen_sock);
	return 0;
}
