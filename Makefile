compile:
	@gcc -o server server.c -O3 -Wall -lcrypto

clean:
	@rm server

