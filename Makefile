compile:
	@gcc -pthread -o server server.c -O3 -flto -lcrypto

clean:
	@rm server

