RACK_DIR ?= ../../Rack-SDK
PLUGIN_NAME := Terroir
BUILD_DIR := build
SRC := plugin.cpp \
       src/Lure.cpp
OBJ := $(SRC:.cpp=.o)

# Compiler and linker flags
CXXFLAGS += -std=c++11 -Wsuggest-override -fPIC -D_USE_MATH_DEFINES -I./src -I/c/Users/ShockValue/Documents/VCV-Dev/Rack-SDK/include -I/c/Users/ShockValue/Documents/VCV-Dev/Rack-SDK/dep/include


LDFLAGS += -shared -L$(RACK_DIR) -lRack -static-libstdc++

all: plugin.dll

plugin.dll: $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o src/*.o plugin.dll
