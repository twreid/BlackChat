# BlackChat
#

CC = gcc
CFLAGS = -Wall -O3
LIBS = -lncurses
BUILD_DIR = bin

CLIENT_SRC_DIR = client
CLIENT_OBJECTS = $(CLIENT_SRC_DIR)/logger.o $(CLIENT_SRC_DIR)/client.o $(CLIENT_SRC_DIR)/clientsocket.o

SERVER_SRC_DIR = server
SERVER_OBJECTS = $(SERVER_SRC_DIR)/bcserver.o


all: client server

client: $(CLIENT_OBJECTS)
	$(CC) -o $(BUILD_DIR)/client $(CLIENT_OBJECTS) $(LIBS)

server: $(SERVER_OBJECTS)
	$(CC) -o $(BUILD_DIR)/server $(SERVER_OBJECTS) $(LIBS)

clean:
	rm $(SERVER_OBJECTS)
	rm $(CLIENT_OBJECTS)
	rm $(BUILD_DIR)/server
	rm $(BUILD_DIR)/client
	rm $(BUILD_DIR)/client.log
