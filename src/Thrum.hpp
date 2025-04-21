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
    enum ParamIds {
        // Main Controls
        TOTAL_DURATION_PARAM,
        DUTY_CYCLE_PARAM,
        BIAS_PARAM,
        SAMPLE_SELECT_PARAM,
        // Attenuverters for CV Inputs
        DURATION_ATTEN_PARAM, // New
        DUTY_ATTEN_PARAM,     // New
        BIAS_ATTEN_PARAM,     // New
        // --- End New Params ---
        NUM_PARAMS            // NUM_PARAMS must be last
    };
    enum InputIds {
        CLOCK_INPUT,
        AUDIO_INPUT,
        // CV Inputs for Main Controls
        DURATION_CV_INPUT,    // New
        DUTY_CV_INPUT,        // New
        BIAS_CV_INPUT,        // New
        // --- End New Inputs ---
        NUM_INPUTS            // NUM_INPUTS must be last
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        ENV_OUTPUT,
        NUM_OUTPUTS           // NUM_OUTPUTS must be last
    };
    enum LightIds {
        // Add any lights here if needed later
        NUM_LIGHTS            // NUM_LIGHTS must be last
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
    void onReset() override; // Add onReset declaration if not present
    bool loadSample(const std::string& path, SampleData& outData); // Sample loading helper

};

// Widget declaration
struct ThrumWidget : rack::app::ModuleWidget {
    ThrumWidget(Thrum* module);
};

#endif // THRUM_HPP
