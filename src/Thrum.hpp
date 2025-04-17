#ifndef THRUM_HPP
#define THRUM_HPP

#include "rack.hpp"
using namespace rack;

// Extern plugin instance
extern rack::Plugin* pluginInstance;

struct Thrum : Module {
    enum ParamIds {
        TOTAL_DURATION_PARAM,  // 0…1 mapped to our RATE_STEPS when clocked, or to seconds when free‑run
        DUTY_CYCLE_PARAM,      // 0…1 fraction of active window
        BIAS_PARAM,            // 0…1 peak position
        NUM_PARAMS
    };
    enum InputIds {
        CLOCK_INPUT,           // clock/gate
        AUDIO_INPUT,           // audio in to be shaped
        NUM_INPUTS
    };
    enum OutputIds {
        ENV_OUTPUT,            // raw envelope (0–10 V)
        AUDIO_OUTPUT,          // shaped audio out
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    // Free‑run phase
    float phase = 0.f;

    // Clocked mode state
    float clockPhase = 0.f;
    float clockTimer = 0.f;
    float clockPeriod = 1.f;
    bool haveClockPeriod = false;
    bool prevGateHigh = false;
	bool clockSynced = false;
	int   edgeCount = 0;       // counts rising edges
	bool  firstEdge = true;    // to align on first clock


    Thrum();
    void process(const ProcessArgs& args) override;
};

struct ThrumWidget : ModuleWidget {
    ThrumWidget(Thrum* module);
};

#endif // THRUM_HPP
