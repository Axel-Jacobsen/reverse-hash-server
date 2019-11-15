compile:
	@gcc -o server server.c -O3 -flto -Wall -lcrypto

clean:
	@rm server

