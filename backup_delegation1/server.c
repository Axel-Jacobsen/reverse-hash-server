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
#include <semaphore.h>
#include "messages.h"

#define SERVER_IP "192.168.101.10"
#define MESSAGE_LEN 49
#define SHA_LEN 32
#define RESPONSE_LEN 8
#define MAX_THREADS 4


typedef struct Thread_args{
	sem_t mutex;
	int sock;
}Thread_args;

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
	
	//Loop from and including to but not including end
        for(k = start; k < end; k++){
                sha_good = 1;
                sha256(&k, sha256_test);
		for(i = 0; i < SHA_LEN; i++)
		{
			if(big_endian_arr[i] != sha256_test[i])
			{
				sha_good = 0; //If a mismatching byte is found, stop for this hash
				break;
			}
		}	
                
		if(sha_good){
                        k_conv = htobe64(k);
                        memcpy(response_arr, &k_conv, sizeof(k_conv));
                        return;
                }
        }
}


void request_handler(int sock){
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
                rev_hash(buffer, response);		//Break hash
                send(sock, response, RESPONSE_LEN, 0);  //Send reponse to client
        }
        close(sock);
}

//Create worker threads running this function
void* worker_thread(void* args){
	
	Thread_args* thread_args = (Thread_args*)args;
	int* int_ptr = malloc(sizeof(int));
	*int_ptr = 0;
	while(1){
		while(!*int_ptr)
		{
			sem_getvalue(&(thread_args->mutex),int_ptr);
		}
		
		request_handler(thread_args->sock);
		sem_wait(&(thread_args->mutex));
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
	
	pthread_t rh[MAX_THREADS];
	Thread_args thread_args[MAX_THREADS];
	int i;

	//Create threads and initialize mutexes
	for(i = 0; i < MAX_THREADS; i++){
		sem_init(&(thread_args[i].mutex), 0, 0);
		pthread_create(&rh[i], NULL, worker_thread, (void*)&(thread_args[i]));
	}

	printf("\nServing on %s:%d\n", inet_ntoa(server_addr.sin_addr), PORT);
	int sock;
	int* semVal = malloc(sizeof(int));

	while (1)
	{	
		int j = 0;
		if ((sock = accept(listen_sock, (struct sockaddr *) &client_addr, &client_address_len)) < 0)
                {
                        perror("couldn't open a socket to accept data");
                        return 1;
                }

		while(1)
		{	//Check sem value of all worker threads
			sem_getvalue(&(thread_args[j].mutex), semVal);
			
			//If one is ready, give it the request
			if(*semVal == 0){
				
				thread_args[j].sock = sock;
				sem_post(&(thread_args[j].mutex));
				break;
			}	
	
			j++;
			if(j >= MAX_THREADS)
			{
				j = 0;		//Reset
				sleep(1);	//No threads are available, wait
								
			}
		}	
        }
		
	close(listen_sock);
	return 0;
}

