CPPFILES = $(wildcard src/*.cpp)
OBJFILES = $(CPPFILES:src/%.cpp=%.o)
INCLUDES = -I src -lssl -lz -lcrypto

ifeq ($(OS),Windows_NT)
	INCLUDES += -luv -lWs2_32 -lwsock32 -lGdi32 -lpsapi -liphlpapi -luserenv
endif

LIB = libuWS.a

CPP_STATIC := -std=c++11 -O3 $(INCLUDES) -c $(CPPFILES)

$(LIB):
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(CPP_STATIC) -s
	ar rcs $(LIB) $(OBJFILES)
	rm -f $(OBJFILES)

clean:
	rm -f $(LIB)
	rm -f $(OBJFILES)
