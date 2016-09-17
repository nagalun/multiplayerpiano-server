OBJS = server.cpp msg.cpp limiter.cpp database.cpp
UWS = uWebSockets/src/Extensions.cpp uWebSockets/src/HTTPSocket.cpp uWebSockets/src/Network.cpp uWebSockets/src/Server.cpp uWebSockets/src/UTF8.cpp uWebSockets/src/WebSocket.cpp uWebSockets/src/EventSystem.cpp

CC = g++

COMPILER_FLAGS = -Wall -std=gnu++0x -O2

LIBS = -I ./uWebSockets/src -luv -lcrypto -lssl -lz

OBJ_NAME = out

all : $(OBJS)
	$(CC) $(OBJS) $(UWS) $(COMPILER_FLAGS) $(LIBS) -o $(OBJ_NAME)
