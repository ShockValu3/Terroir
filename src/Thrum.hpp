#ifndef THRUM_HPP
#define THRUM_HPP

#include "rack.hpp"
#include "plugin.hpp"
#include <vector>
#include <string> // Include string

using namespace rack;

extern rack::Plugin* pluginInstance;

// SampleData struct remains the same
struct SampleData {
    std::vector<float> buffer;
    float nativeRate = 44100.f;
};


struct Thrum : Module {
    // Updated Enums: Added Duration & Duty CV/Atten Params/Inputs
    enum ParamIds {
        // Main Controls
        TOTAL_DURATION_PARAM, // 0
        DUTY_CYCLE_PARAM,     // 1
        BIAS_PARAM,           // 2
        SAMPLE_SELECT_PARAM,  // 3
        // Attenuverters for CV Inputs
        DURATION_ATTEN_PARAM, // 4 (New)
        DUTY_ATTEN_PARAM,     // 5 (New)
        BIAS_ATTEN_PARAM,     // 6
        // --- End New Params ---
        NUM_PARAMS            // NUM_PARAMS should be last (Value is now 7)
    };
    enum InputIds {
        CLOCK_INPUT,          // 0
        AUDIO_INPUT,          // 1
        // CV Inputs for Main Controls
        DURATION_CV_INPUT,    // 2 (New)
        DUTY_CV_INPUT,        // 3 (New)
        BIAS_CV_INPUT,        // 4
        // --- End New Inputs ---
        NUM_INPUTS            // NUM_INPUTS should be last (Value is now 5)
    };
    enum OutputIds {
        AUDIO_OUTPUT,         // 0
        ENV_OUTPUT,           // 1
        NUM_OUTPUTS           // NUM_OUTPUTS should be last (Value is 2)
    };
    enum LightIds {
        NUM_LIGHTS            // NUM_LIGHTS should be last (Value is 0)
    };

    // State variables remain the same
    float clockPhase = 0.f;
    bool prevGateHigh = false;
    bool isRunning = false;
    float phase = 0.f;
    std::vector<SampleData> loadedSamples;
    double samplePlaybackPhase = 0.0;
    int currentSampleIndex = 0;
    int processCounter = 0;

    // --- Methods ---
    Thrum(); // Constructor
    void process(const ProcessArgs& args) override; // Main processing function
    void onReset() override; // Reset method
    bool loadSample(const std::string& path, SampleData& outData); // Sample loading helper

};

// Widget declaration
struct ThrumWidget : rack::app::ModuleWidget {
    ThrumWidget(Thrum* module);
};

#endif // THRUM_HPP
