# === VCV Rack Plugin Makefile ===

RACK_DIR ?= ../../Rack-SDK
PLUGIN_NAME := Terroir
BUILD_DIR := build

SRC := plugin.cpp \
       src/Lure.cpp \
       src/Thrum.cpp \
	   src/Wend.cpp

OBJ := $(SRC:.cpp=.o)

# Compiler and linker flags
CXXFLAGS += -std=c++11 -Wsuggest-override -fPIC -D_USE_MATH_DEFINES -I./src -I$(RACK_SDK)/include -I$(RACK_SDK)/dep/include 
CXXFLAGS += -I$(USERPROFILE)/Documents/VCV-Dev/Libraries/include
LDFLAGS += -shared -L$(RACK_DIR) -lRack -static-libstdc++

# Default behavior: make clean, then build
default: all

all:
	$(MAKE) clean
	$(MAKE) build

build: plugin.dll

plugin.dll: $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o src/*.o plugin.dll

# === Distribution Packaging ===

DIST_NAME := Terroir
DIST_DIR := dist/$(DIST_NAME)
DIST_FILE := $(DIST_NAME).vcvplugin

dist: build
	@echo "Creating dist/ package..."
	mkdir -p $(DIST_DIR)
	cp plugin.dll plugin.json license.txt $(DIST_DIR)
	cp -r res $(DIST_DIR)
	cd dist && tar -cf - $(DIST_NAME) | zstd --force -o $(DIST_FILE)
	@echo "Created dist/$(DIST_FILE)"
