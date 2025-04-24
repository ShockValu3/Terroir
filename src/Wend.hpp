#pragma once
#include <rack.hpp>

using namespace rack;

struct Wend : engine::Module {
    enum ParamIds {
        FREQ_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    float phase = 0.f;

    Wend() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(FREQ_PARAM, 20.f, 20000.f, 1.f, "Frequency Multiplier");
    }

    void process(const ProcessArgs& args) override;
};

struct WendWidget : app::ModuleWidget {
    WendWidget(Wend* module);
};

extern Model* modelWend;
