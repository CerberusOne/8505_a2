stego:stego.c ./wrappers/epoll.c ./wrappers/socketwrappers.c
	gcc -g -Wall -o stego stego.c ./wrappers/epoll.c ./wrappers/socketwrappers.c -lcrypto
