OBJS = server.cpp msg.cpp limiter.cpp database.cpp
UWS = uWebSockets/src/Extensions.cpp uWebSockets/src/Group.cpp uWebSockets/src/WebSocketImpl.cpp uWebSockets/src/Networking.cpp uWebSockets/src/Hub.cpp uWebSockets/src/Node.cpp uWebSockets/src/WebSocket.cpp uWebSockets/src/HTTPSocket.cpp uWebSockets/src/Socket.cpp

CC = g++

COMPILER_FLAGS = -Wall -std=gnu++0x -O2

LIBS = -I ./uWebSockets/src -I ./json/src -luv -lcrypto -lssl -lz

OBJ_NAME = out

all : $(OBJS)
	$(CC) $(OBJS) $(UWS) $(COMPILER_FLAGS) $(LIBS) -o $(OBJ_NAME)

vanilla : $(OBJS)
	$(CC) $(OBJS) $(UWS) $(COMPILER_FLAGS) -DVANILLA_SERVER $(LIBS) -o $(OBJ_NAME)
