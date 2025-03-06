CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lm

SERVER_SRC = server.c
CLIENT_SRC = client.c

all: server subscriber

server: $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $^

subscriber: $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: run-server run-subscriber clean

run-server: server
	cd ../Server && ./server

run-subscriber: subscriber
	cd ../Client && ./subscriber

clean:
	rm -f server subscriber