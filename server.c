#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include <semaphore.h>

#include "messages.h"
#include "priority.h"
#include "caching.h"

#define SERVER_IP "192.168.101.10"
#define MESSAGE_LEN 49
#define SHA_LEN 32
#define RESPONSE_LEN 8
#define MAX_THREADS 4
#define SEM_FULL_INITIAL 0
#define SEM_EMPTY_INITIAL 1000
#define MUTEX_INITIAL 1


Queue queue_global[NUMBER_OF_PRIOS];
Node *cache[CACHE_SIZE] = {NULL};

sem_t mutexD, empty, full;


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

        uint8_t sha256_test[SHA_LEN] = {0};
        uint64_t k;
        uint64_t k_conv;
	
        for(k = start; k < end; k++){
                sha256(&k, sha256_test);
		if(memcmp(big_endian_arr, sha256_test, SHA_LEN) == 0)
		{
                        k_conv = htobe64(k);
                        memcpy(response_arr, &k_conv, sizeof(k_conv));
                        break;
                }
        }
}


void* request_handler_thread(void* args){
	
	while(1){
		uint8_t response[RESPONSE_LEN] = {0};
		QNode* qnode;

		sem_wait(&full);
		sem_wait(&mutexD);
		qnode = deQueue(queue_global);
		sem_post(&mutexD);
		sem_post(&empty);

		int key = cache_hash(qnode->key->hash);
		uint8_t *cache_res = cache_search(key, qnode->key->hash, cache);

		if (cache_res == NULL)
		{
			rev_hash(qnode->key->hash, response);
			send(qnode->key->sock, response, RESPONSE_LEN, 0);
			cache_insert(key, qnode->key->hash, response, cache);
		}
		else
		{
			uint8_t response_arr[RESPONSE_LEN] = {0};
                        memcpy(response_arr, cache_res, RESPONSE_LEN);
			send(qnode->key->sock, response_arr, RESPONSE_LEN, 0);
		}

		close(qnode->key->sock);
		free(qnode->key);
		free(qnode);
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
	
	pthread_t rht;

	//Initialize sempahores
	sem_init(&mutexD, 0, MUTEX_INITIAL);
	sem_init(&empty, 0, SEM_EMPTY_INITIAL);
	sem_init(&full, 0, SEM_FULL_INITIAL);

	//Create threads
	int i;
	for(i = 0; i < MAX_THREADS; i++){
		pthread_create(&rht, NULL, request_handler_thread, NULL);
	}
	sleep(1); //Give threads a chance to start

	printf("\nServing on %s:%d\n", inet_ntoa(server_addr.sin_addr), PORT);
	int sock;

	// Init Queue
	// 
	int j;
    	for (j=0; j<NUMBER_OF_PRIOS; j++)
		queue_global[j] = *createQueue();


	while (1)
	{
		if ((sock = accept(listen_sock, (struct sockaddr *) &client_addr, &client_address_len)) < 0)
                {
                        perror("couldn't open a socket to accept data");
                        return 1;
                }

		uint8_t buff[MESSAGE_LEN] = {0};
		uint8_t *pbuff = buff;
		request* req = (request *)malloc(sizeof(request));

		recv(sock, pbuff, MESSAGE_LEN, 0);

		int key = cache_hash(buff);
		uint8_t *cache_res = cache_search(key, buff, cache);
		if (cache_res == NULL)
		{
			initReq(req, buff, sock);
			sem_wait(&empty);
			sem_wait(&mutexD);
			enQueue(queue_global, req); // request packet pointer
			sem_post(&mutexD);
			sem_post(&full);

		}
		else
		{
			uint8_t response_arr[RESPONSE_LEN] = {0};
                        memcpy(response_arr, cache_res, RESPONSE_LEN);
			send(sock, response_arr, RESPONSE_LEN, 0);
		}

        }
	close(listen_sock);
	return 0;
}
