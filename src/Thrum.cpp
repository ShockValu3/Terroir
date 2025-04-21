#include "Thrum.hpp" // Module header FIRST
#include "rack.hpp" // Explicitly include rack.hpp here too

// VCV Rack Headers
#include "plugin.hpp"
#include "asset.hpp"
#include "dsp/approx.hpp" // For clamp/common math

// Standard Library Headers
#include <vector>
#include <cmath>
#include <stdexcept>

// --- dr_wav ---
#include "dr_wav.h"
// --- end dr_wav ---

// Custom Widget/Component Headers
#include "componentlibrary.hpp"
#include "widgets/Magpie125.hpp"
#include "widgets/Song60.hpp"

extern rack::Plugin* pluginInstance;

// --- Helper Functions: Envelope Segments ---
// (Assuming attackSegment and decaySegment functions are defined here as before)
static const float k = 5.f;
static float attackSegment(float t, float p) {
    if (p <= 1e-6f) return 10.f;
    float v = (p - t) / p;
    v = fmaxf(v, 0.f);
    float num = std::exp(-k * v) - std::exp(-k);
    float den = 1.f - std::exp(-k);
    if (std::abs(den) < 1e-6f) return (t <= p) ? 10.f : 0.f;
    return rack::clamp(10.f * (num / den), 0.f, 10.f);
}
static float decaySegment(float t, float activeDuration, float p) {
    float decayDur = activeDuration - p;
    if (decayDur <= 1e-6f) return 0.f;
    float u = (t - p) / decayDur;
    u = rack::clamp(u, 0.f, 1.f);
    float num = std::exp(-k * u) - std::exp(-k);
    float den = 1.f - std::exp(-k);
    if (std::abs(den) < 1e-6f) return 0.f;
    return rack::clamp(10.f * (num / den), 0.f, 10.f);
}


// --- Helper Function: Sample Loading ---
// (Assuming loadSample function is defined here as before)
bool Thrum::loadSample(const std::string& path, SampleData& outData) {
    unsigned int channels;
    unsigned int loadedSampleRate;
    drwav_uint64 totalPcmFrameCount;
    float* pSampleData = nullptr;
    outData.buffer.clear();
    outData.nativeRate = 0.f;
    pSampleData = drwav_open_file_and_read_pcm_frames_f32(path.c_str(), &channels, &loadedSampleRate, &totalPcmFrameCount, NULL);
    if (pSampleData == NULL) { WARN("Failed to load WAV file: %s", path.c_str()); return false; }
    INFO("Loaded sample: %s, Channels: %d, Sample Rate: %d, Frames: %llu", path.c_str(), channels, loadedSampleRate, totalPcmFrameCount);
    outData.nativeRate = (float)loadedSampleRate;
    try {
        outData.buffer.resize(totalPcmFrameCount);
        if (channels == 1) { memcpy(outData.buffer.data(), pSampleData, totalPcmFrameCount * sizeof(float)); }
        else if (channels > 1) {
            for (drwav_uint64 i = 0; i < totalPcmFrameCount; ++i) {
                float monoSample = 0.f;
                for (unsigned int c = 0; c < channels; ++c) { monoSample += pSampleData[i * channels + c]; }
                outData.buffer[i] = monoSample / channels;
            }
        } else { WARN("Sample has 0 channels? %s", path.c_str()); drwav_free(pSampleData, NULL); return false; }
    } catch (const std::exception& e) { WARN("Exception processing sample buffer for %s: %s", path.c_str(), e.what()); drwav_free(pSampleData, NULL); return false; }
    drwav_free(pSampleData, NULL);
    return true;
}


// --- Thrum Constructor ---
Thrum::Thrum() {
    // Configure the number of parameters, inputs, outputs, and lights
    // using the updated enums from Thrum.hpp
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    // Configure main parameters
    configParam(TOTAL_DURATION_PARAM, 0.05f, 2.0f, 1.0f, "Duration", " s");
    configParam(DUTY_CYCLE_PARAM, 0.f, 1.f, 0.5f, "Duty Cycle", "%", 0.f, 100.f);
    configParam(BIAS_PARAM, 0.f, 1.f, 0.5f, "Bias"); // Removed % display for simplicity, add back if needed

    // Configure attenuverter parameters (range 0-1, default 1 for unipolar attenuation)
    configParam(DURATION_ATTEN_PARAM, 0.f, 1.f, 1.f, "Duration CV Attenuation", "%", 0.f, 100.f);
    configParam(DUTY_ATTEN_PARAM, 0.f, 1.f, 1.f, "Duty Cycle CV Attenuation", "%", 0.f, 100.f);
    configParam(BIAS_ATTEN_PARAM, 0.f, 1.f, 1.f, "Bias CV Attenuation", "%", 0.f, 100.f);

    // --- Sample Loading Logic (as before) ---
    isRunning = false;
    prevGateHigh = false;
    clockPhase = 0.f;
    phase = 0.f;
    samplePlaybackPhase = 0.0;
    currentSampleIndex = 0;
    const std::vector<std::string> sampleRelPaths = { "res/sounds/AlienBreath.wav", "res/sounds/siren.wav" };
    loadedSamples.clear();
    loadedSamples.reserve(sampleRelPaths.size());
    for (const auto& relPath : sampleRelPaths) {
        try {
            std::string fullPath = asset::plugin(pluginInstance, relPath); SampleData loadedData;
            if (loadSample(fullPath, loadedData)) { loadedSamples.push_back(loadedData); }
            else { WARN("Skipping sample: %s", relPath.c_str()); }
        } catch (...) { WARN("Unknown exception loading: %s", relPath.c_str()); }
    }
    INFO("Finished loading samples. %zu samples loaded.", loadedSamples.size());
    // --- End Sample Loading Logic ---

    // Configure Sample Select Param (after samples are loaded)
    int numSamples = loadedSamples.size();
    float maxIndex = (numSamples > 0) ? (float)(numSamples - 1) : 0.f;
    configParam(SAMPLE_SELECT_PARAM, 0.f, maxIndex, 0.f, "Sample", "", 0.f, 1.f, 1.f);

    // Apply snapping via ParamQuantity (Discord Suggestion)
    if (paramQuantities[SAMPLE_SELECT_PARAM]) {
         paramQuantities[SAMPLE_SELECT_PARAM]->snapEnabled = true;
         paramQuantities[SAMPLE_SELECT_PARAM]->smoothEnabled = false;
    }

    // Set initial state
    onReset(); // Call onReset to ensure consistent starting state
}

// --- onReset Method ---
void Thrum::onReset() {
    phase = 0.f;
    clockPhase = 0.f;
    isRunning = false;
    prevGateHigh = false;
    samplePlaybackPhase = 0.0;
    // Reset currentSampleIndex to default if needed
    if (loadedSamples.empty()) {
        currentSampleIndex = -1;
    } else {
        currentSampleIndex = rack::math::clamp(0, 0, (int)loadedSamples.size() - 1); // Default to first sample
    }
}


// --- process Method ---
void Thrum::process(const ProcessArgs& args) {

    // --- Read Main Parameters ---
    float durationBase = params[TOTAL_DURATION_PARAM].getValue();
    float dutyBase = params[DUTY_CYCLE_PARAM].getValue();
    float biasBase = params[BIAS_PARAM].getValue();

    // --- Read CV Inputs and Attenuverters ---
    float durationCv = inputs[DURATION_CV_INPUT].getVoltageSum(); // Use getVoltageSum for polyphony safety
    float dutyCv = inputs[DUTY_CV_INPUT].getVoltageSum();
    float biasCv = inputs[BIAS_CV_INPUT].getVoltageSum();

    float durationAtten = params[DURATION_ATTEN_PARAM].getValue();
    float dutyAtten = params[DUTY_ATTEN_PARAM].getValue();
    float biasAtten = params[BIAS_ATTEN_PARAM].getValue();

    // --- Apply CV Modulation ---
    // Define sensitivity: How much does 1V of CV change the parameter?
    // Example: 1V CV changes duration by 0.2s, duty/bias by 10% (0.1)
    const float durationCvScale = 0.2f; // 1V = 0.2 seconds change
    const float dutyBiasCvScale = 0.1f; // 1V = 0.1 (10%) change (assuming +/-5V CV -> +/-0.5 range mod)

    float totalDuration = durationBase + (durationCv * durationAtten * durationCvScale);
    float duty = dutyBase + (dutyCv * dutyAtten * dutyBiasCvScale);
    float bias = biasBase + (biasCv * biasAtten * dutyBiasCvScale);

    // Clamp final modulated values to their valid ranges
    totalDuration = rack::math::clamp(totalDuration, 0.05f, 2.0f);
    duty = rack::math::clamp(duty, 0.f, 1.f);
    bias = rack::math::clamp(bias, 0.f, 1.f);

    // --- Read Other Inputs ---
    bool audioInputConnected = inputs[AUDIO_INPUT].isConnected();
    float inVal = audioInputConnected ? inputs[AUDIO_INPUT].getVoltageSum() : 0.f;

    // --- Sample Selection Logic (as before) ---
    float sampleSelectValue = params[SAMPLE_SELECT_PARAM].getValue();
    int desiredSampleIndex = static_cast<int>(roundf(sampleSelectValue));
    if (!loadedSamples.empty()) {
        if (desiredSampleIndex < 0) desiredSampleIndex = 0;
        if (desiredSampleIndex >= (int)loadedSamples.size()) desiredSampleIndex = (int)loadedSamples.size() - 1;
        if (desiredSampleIndex != currentSampleIndex) samplePlaybackPhase = 0.0;
        currentSampleIndex = desiredSampleIndex;
    } else { currentSampleIndex = -1; }
    // --- End Sample Selection ---


    // --- Envelope Calculation Logic (using modulated params) ---
    float envelopeDuration = totalDuration * rack::math::clamp(duty, 0.01f, 0.99f); // Use clamped duty
    envelopeDuration = fmaxf(envelopeDuration, 1e-6f);
    float peakFraction = rack::math::clamp(bias, 0.f, 1.f);
    float p = envelopeDuration * peakFraction;
    p = fmaxf(1e-6f, fminf(p, envelopeDuration - 1e-6f));

    float env = 0.f;
    float t = 0.f;
    float calculatedEnv = 0.f;
    bool printDebug = (++processCounter % 4096 == 0);

    bool clocked = inputs[CLOCK_INPUT].isConnected();
    if (clocked) { // Clocked Mode Logic (as before)
        float gate = inputs[CLOCK_INPUT].getVoltageSum();
        bool gateHigh = gate >= 1.f;
        if (!prevGateHigh && gateHigh) { isRunning = true; clockPhase = 0.f; }
        prevGateHigh = gateHigh;
        if (isRunning) {
            clockPhase += args.sampleTime; t = clockPhase;
            if (clockPhase >= totalDuration) { isRunning = false; calculatedEnv = 0.f; }
            else {
                if (t <= envelopeDuration) {
                    calculatedEnv = (t <= p) ? attackSegment(t, p) : decaySegment(t, envelopeDuration, p);
                } else { calculatedEnv = 0.f; }
            }
        } else { calculatedEnv = 0.f; }
        env = calculatedEnv;
    } else { // Free-running Mode Logic (as before)
        if (isRunning || prevGateHigh) { isRunning = false; clockPhase = 0.f; prevGateHigh = false; }
        phase += args.sampleTime;
        if (phase >= totalDuration) { phase -= totalDuration; if (phase < 0.f) phase = 0.f; }
        t = phase;
        if (t <= envelopeDuration) {
            calculatedEnv = (t <= p) ? attackSegment(t, p) : decaySegment(t, envelopeDuration, p);
        } else { calculatedEnv = 0.f; }
        env = calculatedEnv;
    }
    env = rack::math::clamp(env, 0.f, 10.f); // Final envelope clamp
    // --- End Envelope Calculation ---


    // --- Audio Output Logic (as before) ---
    float audioOutputValue = 0.f;
    if (audioInputConnected) {
        audioOutputValue = inVal * (env / 10.f);
    } else {
        if (currentSampleIndex >= 0 && currentSampleIndex < (int)loadedSamples.size() && !loadedSamples[currentSampleIndex].buffer.empty()) {
            const SampleData& currentSample = loadedSamples[currentSampleIndex];
            const std::vector<float>& buffer = currentSample.buffer;
            size_t bufferSize = buffer.size();
            if (bufferSize > 0) {
                double phaseIncrement = currentSample.nativeRate / args.sampleRate;
                samplePlaybackPhase += phaseIncrement;
                samplePlaybackPhase = fmod(samplePlaybackPhase, (double)bufferSize);
                if (samplePlaybackPhase < 0.0) { samplePlaybackPhase += bufferSize; }
                int index0 = static_cast<int>(samplePlaybackPhase);
                int index1 = index0 + 1;
                if (index1 >= (int)bufferSize) { index1 -= bufferSize; }
                if (index0 < 0 || index0 >= (int)bufferSize) { index0 = 0; }
                float frac = samplePlaybackPhase - index0;
                float sampleValue = rack::math::crossfade(buffer[index0], buffer[index1], frac);
                audioOutputValue = (sampleValue * 5.0f) * (env / 10.0f);
            } else { audioOutputValue = 0.f; }
        } else { audioOutputValue = 0.f; }
    }
    // --- End Audio Output ---

    // Set Outputs
    outputs[ENV_OUTPUT].setVoltage(env);
    outputs[AUDIO_OUTPUT].setVoltage(audioOutputValue);
}


// --- ThrumWidget Constructor (Hardcoded Positions) ---
ThrumWidget::ThrumWidget(Thrum* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Thrum.svg")));

    // --- Add Controls Row by Row with Hardcoded Positions ---

    // Duration Row (Y ~ 20-25)
    addParam(createParamCentered<Magpie125>(mm2px(Vec(9,20)), module, Thrum::TOTAL_DURATION_PARAM));
    addParam(createParamCentered<Song60>(mm2px(Vec(23, 22.25)), module, Thrum::DURATION_ATTEN_PARAM));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34, 22.33)), module, Thrum::DURATION_CV_INPUT));

    // Duty Cycle Row (Y ~ 45-50)
    addParam(createParamCentered<Magpie125>(mm2px(Vec(9, 39)), module, Thrum::DUTY_CYCLE_PARAM));
    addParam(createParamCentered<Song60>(mm2px(Vec(23, 41.25)), module, Thrum::DUTY_ATTEN_PARAM));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34, 41.33)), module, Thrum::DUTY_CV_INPUT));

    // Bias Row (Y ~ 70-75)
    addParam(createParamCentered<Magpie125>(mm2px(Vec(9, 58)), module, Thrum::BIAS_PARAM));
    addParam(createParamCentered<Song60>(mm2px(Vec(23, 60.25)), module, Thrum::BIAS_ATTEN_PARAM));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34, 60.33)), module, Thrum::BIAS_CV_INPUT));

    // Sample Select Knob (Y ~ 95)
    addParam(createParamCentered<Song60>(mm2px(Vec(24,92.5)), module, Thrum::SAMPLE_SELECT_PARAM));
    // Snapping is handled in Module constructor now

    // Bottom Row Jacks (Y ~ 110)
    addInput (createInputCentered<PJ301MPort>(mm2px(Vec(9, 80)), module, Thrum::CLOCK_INPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(31, 105)), module, Thrum::AUDIO_OUTPUT));

    addInput (createInputCentered<PJ301MPort>(mm2px(Vec(9, 105)), module, Thrum::AUDIO_INPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(31, 80)), module, Thrum::ENV_OUTPUT));

    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
}

