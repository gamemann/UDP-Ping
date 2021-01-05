SERVERSRC += src/server.c
CLIENTSRC += src/client.c

all: client server

client:
	clang -O2 -o ping $(CLIENTSRC)

server:
	clang -O2 -o server $(SERVERSRC)

clean:
	rm -f src/*.o
	rm -f ping
	rm -f server

.PHONY: server all
.DEFAULT: all