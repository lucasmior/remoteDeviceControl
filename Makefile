CC=g++
CFLAGS+=-lpthread
#CFLAGS+=-std=c++11
INCLUDE_PATH=inc

NAME=server
BROADCAST_IP=192.168.25.255
UDP_PORT=8899
TCP_PORT=7070

all:
	$(CC) server.cpp src/memory.c -o server $(CFLAGS) -I $(INCLUDE_PATH)
	gcc -o client client.c

clean:
	rm client
	rm server

run:
	./server $(NAME) $(BROADCAST_IP) $(UDP_PORT) $(TCP_PORT)

