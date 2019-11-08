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
	
        for(k = start; k < end; k++){
                sha_good = 1;
                sha256(&k, sha256_test);
		if((*big_endian_arr != *sha256_test) || (*(big_endian_arr+1) != *(sha256_test+1))  || (*(big_endian_arr+2) != *(sha256_test+2)) || (*(big_endian_arr+3) != *(sha256_test+3)) || (*(big_endian_arr+4) != *(sha256_test+4)) || (*(big_endian_arr+5) != *(sha256_test+5)) || (*(big_endian_arr+6) != *(sha256_test+6)) || (*(big_endian_arr+7) != *(sha256_test+7)) || (*(big_endian_arr+8) != *(sha256_test+8)) || (*(big_endian_arr+9) != *(sha256_test+9))|| (*(big_endian_arr+10) != *(sha256_test+10))|| (*(big_endian_arr+11) != *(sha256_test+11))|| (*(big_endian_arr+12) != *(sha256_test+12))|| (*(big_endian_arr+13) != *(sha256_test+13))|| (*(big_endian_arr+14) != *(sha256_test+14))|| (*(big_endian_arr+15) != *(sha256_test+15))|| (*(big_endian_arr+16) != *(sha256_test+16))|| (*(big_endian_arr+17) != *(sha256_test+17))|| (*(big_endian_arr+18) != *(sha256_test+18))|| (*(big_endian_arr+19) != *(sha256_test+19))|| (*(big_endian_arr+20) != *(sha256_test+20))|| (*(big_endian_arr+21) != *(sha256_test+21))|| (*(big_endian_arr+22) != *(sha256_test+22))|| (*(big_endian_arr+23) != *(sha256_test+23))|| (*(big_endian_arr+24) != *(sha256_test+24))|| (*(big_endian_arr+25) != *(sha256_test+25))|| (*(big_endian_arr+26) != *(sha256_test+26))|| (*(big_endian_arr+27) != *(sha256_test+27))|| (*(big_endian_arr+28) != *(sha256_test+28))|| (*(big_endian_arr+29) != *(sha256_test+29))|| (*(big_endian_arr+30) != *(sha256_test+30))|| (*(big_endian_arr+31) != *(sha256_test+31))){

			sha_good = 0;
		}		

		/*
                for(i = 0; i < 32; i++){
                        if(big_endian_arr[i] != sha256_test[i]){
                                sha_good = 0;
                                break;
                        }
                }
		*/
                if(sha_good){
                        k_conv = htobe64(k);
                        memcpy(response_arr, &k_conv, sizeof(k_conv));
                        break;
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
                rev_hash(buffer, response);
                send(sock, response, RESPONSE_LEN, 0);
        }
        close(sock);
        //printf("Job done\n");
}


void* worker_thread(void* args){
	
	Thread_args* thread_args = (Thread_args*)args;
	int* int_ptr = malloc(sizeof(int));
	*int_ptr = 0;
	while(1){
		//printf("Waiting on request!\n");
		while(!*int_ptr)
		{
			sem_getvalue(&(thread_args->mutex),int_ptr);
		}
		//printf("Semaphore val:%d\n", *int_ptr);
		//printf("Handling request!\n");
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
	for(i = 0; i < MAX_THREADS; i++){
		sem_init(&(thread_args[i].mutex), 0, 0);
		pthread_create(&rh[i], NULL, worker_thread, (void*)&(thread_args[i]));
	}
	sleep(2); //Give threads a chance to start

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
		{	
			sem_getvalue(&(thread_args[j].mutex), semVal);
			
			if(*semVal == 0){
				//printf("Thread number[%d] takes care of the request!\n", j);
				thread_args[j].sock = sock;
				sem_post(&(thread_args[j].mutex));
				break;
			}	
	
			j++;
			if(j >= MAX_THREADS)
			{
				j = 0;	//Reset
				sleep(1);	//No threads are available, Later replacae with signal
								
			}
		}

		
		/*
		printf("Creating thread to handle request!\n");
		Socket_info* socket_info = (Socket_info*)malloc(sizeof(Socket_info));
		socket_info->sock = sock;
		if(pthread_create(&rh, NULL, request_handler, (void*)socket_info) != 0){
                	printf("Error creating a new thread going up\n");
                	exit(1);
		}
		*/
        }
		
	close(listen_sock);
	return 0;
}

