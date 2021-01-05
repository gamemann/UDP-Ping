COMMOBJS += src/common.o
SERVERSRC += src/server.c
CLIENTSRC += src/client.c

all: client server

client: $(COMMOBJS)
	clang -O2 -o ping $(COMMOBJS) $(CLIENTSRC)

server: $(COMMOBJS)
	clang -O2 -o server $(COMMOBJS) $(SERVERSRC)

clean:
	rm -f src/*.o
	rm -f ping
	rm -f server

.PHONY: all
.DEFAULT: all