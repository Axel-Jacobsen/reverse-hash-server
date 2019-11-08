compile:
	@gcc -pthread -o server server.c -Ofast -lcrypto

clean:
	@rm server

