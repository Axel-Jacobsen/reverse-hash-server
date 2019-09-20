#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>

#include "messages.h"

#define SERVER_IP "192.168.101.10"
#define MESSAGE_LEN 49


void binprintf(int v)
{
	uint64_t mask=1<<((sizeof(uint32_t)<<3)-1);
	while(mask)
	{
		printf("%d", (v&mask ? 1 : 0));
		mask >>= 1;
	}
}

/*
 * big_endian_arr is the client message, lil_endian_arr is the client message in lil endian
 */
void read_client_msg(char *big_endian_arr, char *lil_endian_arr, int len)
{
	/*
	hash = big_endian_arr[0:32];
	start = big_endain_arr[32:40];
	end = big_endian_arr[40:48];
	priority = big_endain_arr[49];
	*/

	// We can not store the hash in a number, we have to use an array
	// of chars - this is because the hash is 256 bits, and there isn't
	// a `uint256_t`.
	int i;
	printf("0x");
	for (i = 0; i < 32; i++)
	{
		printf("%02x", (unsigned char)big_endian_arr[i]);
		lil_endian_arr[31 - i] = big_endian_arr[i];
	}
	printf(" ");

	uint64_t start = 0;
	for (i = 32; i < 40; i++)
	{
		start = start | ((unsigned char)big_endian_arr[i] << (8 * (39 - i)));
		lil_endian_arr[39 - i] = big_endian_arr[i];
	}
	printf("%lu ", start);

	uint64_t end = 0;
	for (i = 40; i < 48; i++)
	{
		end = end | ((unsigned char)big_endian_arr[i] << (8 * (47 - i)));
		lil_endian_arr[47 - i] = big_endian_arr[i];
	}
	printf("%lu ", end);
			
	printf("%d\n", big_endian_arr[48]);
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
	int wait_size = 16; // number of clients that we queue before connection is busy
	if (listen(listen_sock, wait_size) < 0)
	{
		perror("couldn't open socket for listening");
		return 1;
	}

	printf("serving on %s:%d\n", inet_ntoa(server_addr.sin_addr), PORT);

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
		int len = 0, maxlen = 49;
		char buffer[maxlen];
		char *pbuffer = buffer;

		char *client_ip = inet_ntoa(client_addr.sin_addr);
		char little_endian[MESSAGE_LEN];

		printf("client connected with ip address: %s\n", client_ip);
		while ((n = recv(sock, pbuffer, maxlen, 0)) > 0)
		{
			pbuffer += n;
			maxlen -= n;
			len += n;

			read_client_msg(buffer, little_endian, MESSAGE_LEN);

			send(sock, buffer, len, 0);
		}
		close(sock);
	}
	close(listen_sock);
	return 0;
}

