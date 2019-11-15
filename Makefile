compile:
	@gcc -o server server.c -O3 -flto -lcrypto

clean:
	@rm server

