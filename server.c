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
pthread_t startfunction;
pthread_t startmiddlefunction;
pthread_t middleendfunction;
pthread_t endfunction;
typedef struct Thread_input{
	uint64_t start;
	uint64_t end;
	int sock;
	uint8_t *big_endian_arr;
	uint8_t response_arr[RESPONSE_LEN];
}Thread_input;

void sha256(uint64_t *v, unsigned char out_buff[SHA256_DIGEST_LENGTH])
{
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, v, sizeof(v));
	SHA256_Final(out_buff, &sha256);
}
void* thread_start_function(void* args){
        //pthreadcanceltype is the only way to make sure it stops properly for now
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
	Thread_input* thread_inputs = (Thread_input*)args;
        uint8_t sha_good = 1;
        uint8_t sha256_test[SHA_LEN] = {0};
        uint64_t k;
        uint64_t k_conv;
	uint64_t diff = ((thread_inputs->end) - (thread_inputs->start))/4; 
         for(k = thread_inputs->start; k < (thread_inputs->start)+diff; k++){
		pthread_testcancel();
                sha_good = 1;
                sha256(&k, sha256_test);
		int i;
                for(i = 0; i < 32; i++){
                        if(thread_inputs->big_endian_arr[i] != sha256_test[i]){
                                sha_good = 0;
                                break;
                        }
                }
                if(sha_good){
			pthread_cancel(startmiddlefunction);
			pthread_cancel(middleendfunction);
			pthread_cancel(endfunction);
			//printf("START FOUND\n");
                        k_conv = htobe64(k);
                        memcpy(thread_inputs->response_arr, &k_conv, sizeof(k_conv));
			send(thread_inputs->sock, thread_inputs->response_arr, RESPONSE_LEN, 0);
                        return;
                }
        }
printf("Start exited normally\n");
}
void* thread_start_middle_function(void* args){
        //pthreadcanceltype is the only way to make sure it stops properly for now
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
        Thread_input* thread_inputs = (Thread_input*)args;
        uint8_t sha_good = 1;
        uint8_t sha256_test[SHA_LEN] = {0};
        uint64_t k;
        uint64_t k_conv;
        uint64_t diff = ((thread_inputs->end) - (thread_inputs->start))/4;
         for(k = (thread_inputs->start)+diff; k < (thread_inputs->end)-(2*diff); k++){
                pthread_testcancel();
                sha_good = 1;
                sha256(&k, sha256_test);
                int i;
                for(i = 0; i < 32; i++){
                        if(thread_inputs->big_endian_arr[i] != sha256_test[i]){
                                sha_good = 0;
                                break;
                        }
                }
                if(sha_good){
			pthread_cancel(startfunction);
			pthread_cancel(middleendfunction);
                        pthread_cancel(endfunction);
                        //printf("START FOUND\n");
                        k_conv = htobe64(k);
                        memcpy(thread_inputs->response_arr, &k_conv, sizeof(k_conv));
                        send(thread_inputs->sock, thread_inputs->response_arr, RESPONSE_LEN, 0);
                        return;
                }
        }
printf("Start middle exited normally\n");
}
void* thread_middle_end_function(void* args){
        //pthreadcanceltype is the only way to make sure it stops properly for now
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
        Thread_input* thread_inputs = (Thread_input*)args;
        uint8_t sha_good = 1;
        uint8_t sha256_test[SHA_LEN] = {0};
        uint64_t k;
        uint64_t k_conv;
        uint64_t diff = ((thread_inputs->end) - (thread_inputs->start))/4;
         for(k = (thread_inputs->end)-(2*diff); k < (thread_inputs->end)-diff; k++){
                pthread_testcancel();
                sha_good = 1;
                sha256(&k, sha256_test);
                int i;
                for(i = 0; i < 32; i++){
                        if(thread_inputs->big_endian_arr[i] != sha256_test[i]){
                                sha_good = 0;
                                break;
                        }
                }
                if(sha_good){
			pthread_cancel(startfunction);
			pthread_cancel(startmiddlefunction);
                        pthread_cancel(endfunction);
                        //printf("START FOUND\n");
                        k_conv = htobe64(k);
                        memcpy(thread_inputs->response_arr, &k_conv, sizeof(k_conv));
                        send(thread_inputs->sock, thread_inputs->response_arr, RESPONSE_LEN, 0);
                        return;
                }
        }
printf("middle end exited normally\n");
}

void* thread_end_function(void* args){
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
        Thread_input* thread_inputs = (Thread_input*)args;
        uint8_t sha_good = 1;
        uint8_t sha256_test[SHA_LEN] = {0};
        uint64_t k;
        uint64_t k_conv;
	uint64_t diff = ((thread_inputs->end) - (thread_inputs->start))/4;
         for(k = (thread_inputs->end)-diff; k < thread_inputs->end; k++){
		pthread_testcancel();
                sha_good = 1;
                sha256(&k, sha256_test);
		int i;
                for(i = 0; i < 32; i++){
                        if(thread_inputs->big_endian_arr[i] != sha256_test[i]){
                                sha_good = 0;
                                break;
                        }
                }
                if(sha_good){
			pthread_cancel(startfunction);
			pthread_cancel(startmiddlefunction);
			pthread_cancel(middleendfunction);
                        //printf("END FOUND\n");
                        k_conv = htobe64(k);
                        memcpy(thread_inputs->response_arr, &k_conv, sizeof(k_conv));
			send(thread_inputs->sock,thread_inputs->response_arr, RESPONSE_LEN, 0);
                        return;
                }
        }
printf("End exited normally\n");
}
                          
// *big_endian_arr is an array of bytes, response_arr is a pointer to an array of the same size
void rev_hash(uint8_t *big_endian_arr, int sock)
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
	Thread_input* threadinput = malloc(sizeof(Thread_input));
	threadinput->response_arr;
	threadinput->sock = sock;
	threadinput->start = start; 
	threadinput->end = end;
	threadinput->big_endian_arr = big_endian_arr;
	pthread_create(&startfunction, NULL, thread_start_function, (void*)(threadinput));
	pthread_create(&startmiddlefunction, NULL, thread_start_middle_function, (void*)(threadinput));
	pthread_create(&middleendfunction, NULL, thread_middle_end_function, (void*)(threadinput));
	pthread_create(&endfunction, NULL, thread_end_function, (void*)(threadinput));
	pthread_join(startfunction, NULL);
	pthread_join(startmiddlefunction,NULL);
	pthread_join(middleendfunction,NULL);
	//printf("Start returned\n");
	pthread_join(endfunction, NULL);	
	//printf("Both threads returned\n");
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
		while ((n = recv(sock, pbuffer, maxlen, 0)) > 0)
		{
			pbuffer += n;
			maxlen -= n;
			len += n;
			rev_hash(buffer, sock);
			//send(sock, response, RESPONSE_LEN, 0);
		}
		close(sock);
	}
	close(listen_sock);
	return 0;
}

