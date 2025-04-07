#include <rack.hpp>
#include "Lure.hpp"

using namespace rack;

Model* modelLure;

rack::Plugin* pluginInstance = nullptr;

extern "C" void init(Plugin* p) {
    pluginInstance = p;
    p->addModel(createModel<Lure, LureWidget>("Lure"));
}