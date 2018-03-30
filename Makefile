CC = gcc
FLAGS = -Wall -Werror -std=gnu11

SERVERFILES = pingserver.c
CLIENTFILES = pingclient.c
ROUTERFILES = router.c

CLEANFILES = bin/ping_client bin/ping_server bin/routing_server

all: client server router

client: $(CLIENTFILES)
	$(CC) $(FLAGS) $(CLIENTFILES) -o bin/ping_client

server: $(SERVERFILES)
	$(CC) $(FLAGS) $(SERVERFILES) -o bin/ping_server

router: $(ROUTERFILES)
	$(CC) $(FLAGS) $(ROUTERFILES) -o bin/routing_server

clean:
	rm -f $(CLEANFILES)
