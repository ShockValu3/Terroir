#ifndef THRUM_HPP
#define THRUM_HPP

#include "rack.hpp"
using namespace rack;

extern rack::Plugin* pluginInstance;

struct Thrum : Module {
    enum ParamIds {
        TOTAL_DURATION_PARAM,
        DUTY_CYCLE_PARAM,
        BIAS_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CLOCK_INPUT,
        AUDIO_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        ENV_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    // Minimal state for one-shot triggering
    float clockPhase = 0.f;
    bool prevGateHigh = false;

    // Free-run
    float phase = 0.f;

    Thrum();
    void process(const ProcessArgs& args) override;
};

struct ThrumWidget : ModuleWidget {
    ThrumWidget(Thrum* module);
};

#endif // THRUM_HPP
