#include <rack.hpp>
#include "Lure.hpp"
#include "Thrum.hpp"

using namespace rack;

Model* modelLure;
Model* modelThrum;

rack::Plugin* pluginInstance = nullptr;

extern "C" void init(Plugin* p) {
    pluginInstance = p;
    p->addModel(createModel<Lure, LureWidget>("Lure"));
    p->addModel(createModel<Thrum, ThrumWidget>("Thrum"));
}