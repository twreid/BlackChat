# BlackChat
# -- Server
#

CC = clang 
CFLAGS = -Wall -Werror -g -std=c11 -I../protocol/include -Iinclude
LIBS = -lpthread ../bin/lib/libbcprotocol.a
BUILD_DIR = ../bin
OBJS = bc_network.o bc_server_queue.o server.o

.PHONY: clean

all: server

server: $(OBJS)
	$(CC) $(CFLAGS)  -o $(BUILD_DIR)/server $(OBJS) $(LIBS)

clean:
	rm $(OBJS)
	rm $(BUILD_DIR)/server
