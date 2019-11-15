compile:
	@gcc -o server server_with_int.c -O3 -lcrypto

clean:
	@rm server

