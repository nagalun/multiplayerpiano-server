SRC_FILES = $(wildcard src/*.cpp)
OBJ_FILES = $(SRC_FILES:src/%.cpp=build/%.o)
DEP_FILES = $(OBJ_FILES:.o=.d)

TARGET    = out

OPT_REL   = -O2
LD_REL    = -s
OPT_DBG   = -Og -g
LD_DBG    = $(OPT_DBG)

CPPFLAGS += -std=c++11 -Wall -Wextra -Wno-unknown-pragmas -Wno-unused-parameter
CPPFLAGS += -MMD -MP

UWS       = ./lib/uWebSockets
JSON      = ./lib/json
LIB_FILES = $(UWS)/libuWS.a

CPPFLAGS += -I ./src/
CPPFLAGS += -I $(UWS)/src/
CPPFLAGS += -I $(JSON)/include/
LDFLAGS  += -L $(UWS)/

LDLIBS   += -lssl -lz -lcrypto

ifeq ($(OS),Windows_NT)
	LDLIBS += -luv -lWs2_32 -lpsapi -liphlpapi -luserenv
else
	LDLIBS += -ldl -lpthread
endif

.PHONY: all g preconditions clean clean-all

all: CPPFLAGS += $(OPT_REL)
all: LDFLAGS  += $(LD_REL)
all: preconditions $(TARGET)

owopp: CPPFLAGS += $(OPT_REL) -D FOR_OWOPP
owopp: LDFLAGS  += $(LD_REL)
owopp: preconditions $(TARGET)

g: CPPFLAGS += $(OPT_DBG)
g: LDFLAGS  += $(LD_DBG)
g: preconditions $(TARGET)

preconditions:
	mkdir -p build

$(TARGET): $(OBJ_FILES) $(LIB_FILES)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<


$(UWS)/libuWS.a:
	$(MAKE) -C $(UWS) -f ../uWebSockets.mk


clean:
	- $(RM) $(TARGET) $(OBJ_FILES) $(DEP_FILES)

clean-all: clean
	$(MAKE) -C $(UWS) -f ../uWebSockets.mk clean

-include $(DEP_FILES)
