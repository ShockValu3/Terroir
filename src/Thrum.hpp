#ifndef THRUM_HPP
#define THRUM_HPP

#include "rack.hpp"
#include "plugin.hpp"
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

    // State for clocked mode
    float clockPhase = 0.f;
    bool prevGateHigh = false;
    bool isRunning = false;

    // Free-run state
    float phase = 0.f;

    // Debug logging counter (member variable) ADD THIS:
    int processCounter = 0; // Or uint64_t if you prefer

    Thrum(); // Make sure constructor exists
    void process(const ProcessArgs& args) override;

    // Optional: If using JSON persistence
    // json_t *dataToJson() override;
    // void dataFromJson(json_t *rootJ) override;
};

struct ThrumWidget : ModuleWidget {
    ThrumWidget(Thrum* module);
};

#endif // THRUM_HPP
