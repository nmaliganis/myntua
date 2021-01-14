###################################################
#
# Makefile
# Simple TCP/IP communication using sockets
#
# Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
#
###################################################

CC = gcc

CFLAGS = -Wall
CFLAGS += -g
CFLAGS += -O2 -fomit-frame-pointer -finline-functions

#LIBS = 

BINS = socket-server socket-client

all: $(BINS)

socket-server: socket-server.c socket-common.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS) socket-common.c

socket-client: socket-client.c socket-common.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)  socket-common.c

clean:
	rm -f *.o *~ $(BINS)
