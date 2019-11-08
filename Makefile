compile:
	@gcc -o server server.c -O3 -lcrypto -lpthread

clean:
	@rm server

