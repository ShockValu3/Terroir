#define _USE_MATH_DEFINES  // Optional but safe to include
#include "plugin.hpp"

using namespace rack;

Plugin* pluginInstance = nullptr;
Model* modelTerroirTest = nullptr;

extern Model* createTerroirModel();  // Declare the function implemented in TerroirTest.cpp

extern "C" void init(Plugin* p) {
    pluginInstance = p;
    modelTerroirTest = createTerroirModel();
    p->addModel(modelTerroirTest);
}
