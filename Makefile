compile:
	@gcc -pthread -o server server.c -O3 -lcrypto

clean:
	@rm server

