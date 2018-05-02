OBJS    = $(wildcard *.cpp)

ALLOBJS = $(OBJS)

COMPILER_FLAGS = -Wall -std=c++17 -O2

LIBS = -I ./ -I ./lib/json/include/ -I ./lib/uWebSockets/src/ -L ./lib/uWebSockets/ -s -static -luWS -lssl -lz -lcrypto

ifeq ($(OS),Windows_NT)
	LIBS += -luv -lstdc++ -Wl,--whole-archive -lpthread -Wl,-Bdynamic,--no-whole-archive -lWs2_32 -lwsock32 -lGdi32 -lpsapi -liphlpapi -luserenv
endif

TARGET = out

all: uWS $(ALLOBJS)
	$(CXX) $(ALLOBJS) $(COMPILER_FLAGS) $(LIBS) -o $(TARGET)

vanilla: uWS $(ALLOBJS)
	$(CXX) $(ALLOBJS) $(COMPILER_FLAGS) -DVANILLA_SERVER $(LIBS) -o $(TARGET)

g: uWS $(ALLOBJS)
	$(CXX) $(ALLOBJS) -Wall -std=gnu++0x -Og -g $(LIBS) -o $(TARGET)

uWS:
	$(MAKE) -C lib/uWebSockets -f ../uWebSockets.mk
