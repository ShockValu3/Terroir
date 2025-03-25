RACK_DIR ?= ../../Rack-SDK
PLUGIN_NAME := Terroir
BUILD_DIR := build
SRC := plugin.cpp src/TerroirTest.cpp
OBJ := $(SRC:.cpp=.o)

# Compiler and linker flags
CXXFLAGS += -std=c++11 -Wsuggest-override -fPIC \
  -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include

LDFLAGS += -shared -L$(RACK_DIR) -lRack -static-libstdc++

all: plugin.dll

plugin.dll: $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o src/*.o plugin.dll
