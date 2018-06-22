SRC_FILES = $(wildcard src/*.cpp)
OBJ_FILES = $(SRC_FILES:.cpp=.o)
DEP_FILES = $(OBJ_FILES:.o=.d)

CPPFLAGS += -std=c++11 -O3
CPPFLAGS += -I ./src/
CPPFLAGS += -MMD -MP

TARGET    = libuWS.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(AR) rcs $@ $^

src/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

clean:
	- $(RM) $(TARGET) $(OBJ_FILES) $(DEP_FILES)

-include $(DEP_FILES)