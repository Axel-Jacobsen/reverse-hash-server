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

#include "messages.h"

#define SERVER_IP "192.168.101.10"
#define MESSAGE_LEN 49
#define SHA_LEN 32
#define RESPONSE_LEN 8
#define NUMBER_OF_PRIOS 16

//Some setup for the priority heap
struct request{
	uint8_t package[MESSAGE_LEN];
	uint8_t priority;
	int socket;
};

struct QNode{
	struct request key;
	struct QNode* next;
};

struct Queue{
	struct QNode *front, *rear;
};

// Function for creating new requests
void initReq(struct request* r, uint8_t* package, int sock){
	memcpy(r->package, package, MESSAGE_LEN*sizeof(uint8_t));
	r->socket = sock;
	r->priority = package[48];
}

//Function for creating new queue nodes
struct QNode* newNode(struct request k){
	struct QNode* temp = (struct QNode*)malloc(sizeof(struct QNode));
	temp->key = k;
	temp->next = NULL;
	return temp;
}

// Function for creating new queue
struct Queue* createQueue(){
	struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue));
	q->front = q->rear = NULL;
	return q;
}

// Function for adding request to queue
void enQueue(struct Queue* q, struct request k){
	// create new node
	struct QNode* temp = newNode(k);

	//Find index for queue
	int i = k.priority-1;
	// If queue is empty, then new node is both front and rear
	if (q[i].rear == NULL){
		q[i].front = q[i].rear = temp;
		return;
	}
	//Add new node at the end of queue
	q[i].rear->next = temp;
	q[i].rear = temp;
}

//Function for extracting node from highest prio queue with elements
struct QNode* deQueue(struct Queue* q){
	//Find highest prio queue with elements
	int i;
	for (i=NUMBER_OF_PRIOS-1; i>=0; i--){
		if (q[i].front!=NULL) {break;}
	}
	// If empty 
	if (q[i].front==NULL) printf("queue is empty");

	//Store previous front and move front one ahead
	struct QNode* temp = q[i].front;
	
	q[i].front = q[i].front->next;

	//If front becomes NULL, then change rear to NULL
	if (q[i].front==NULL) q[i].rear=NULL;

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

	int listFilled = 0;
	// Setup priority Queue
	struct Queue prioList[NUMBER_OF_PRIOS];
	int i;
	for (i=0; i<NUMBER_OF_PRIOS; i++) prioList[i] = *createQueue();

	// Set up timeout feature
	fd_set rset;
	FD_ZERO(&rset);
	FD_SET(listen_sock,&rset);
	struct timeval tv;

	while (1)
	{
		int sock;
		FD_ZERO(&rset);
		FD_SET(listen_sock,&rset);
		tv.tv_sec = 0;
		tv.tv_usec = 1000;
		int n = 0;
		int len = 0, maxlen=MESSAGE_LEN;
		uint8_t buffer[MESSAGE_LEN] = {0};
		uint8_t *pbuffer = buffer;
		uint8_t response[RESPONSE_LEN] = {0};
		if (select(listen_sock+1, &rset, NULL, NULL, &tv)){
		if ((sock = accept(listen_sock, (struct sockaddr *) &client_addr, &client_address_len)) < 0)
		{
			perror("couldn't open a socket to accept data");
			return 1;
		}

		if ((n = recv(sock, pbuffer, maxlen, 0)) > 0)
		{
			pbuffer += n;
			maxlen -= n;
			len += n;

			struct request req;
			initReq(&req, buffer, sock);
			enQueue(prioList, req);
			listFilled++;

		}
		}



		if (listFilled > 9){
			struct QNode* highestPrio = deQueue(prioList);
			rev_hash(highestPrio->key.package, response);//rev_hash(buffer, response);
			send(highestPrio->key.socket, response, RESPONSE_LEN, 0);	//send(sock, response, RESPONSE_LEN, 0);
			close(highestPrio->key.socket);
			free(highestPrio);
		}


	}
	close(listen_sock);
	return 0;
}
