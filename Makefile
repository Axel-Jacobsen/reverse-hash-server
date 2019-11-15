compile:
	@gcc -o server server.c -O3 -flto -lcrypto -pthread

clean:
	@rm server

