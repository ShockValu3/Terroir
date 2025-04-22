#include "Thrum.hpp" // Module header FIRST
#include "rack.hpp" // Explicitly include rack.hpp here too

// VCV Rack Headers
#include "plugin.hpp"
#include "asset.hpp"
#include "dsp/approx.hpp" // For clamp/common math

// Standard Library Headers
#include <vector>
#include <cmath>    // Provides powf, fmaxf, fminf, fmod, exp
#include <stdexcept>
#include <string> // For std::string, std::to_string
#include <utility> // For std::pair
#include <cstdio> // For snprintf

// --- dr_wav ---
#include "dr_wav.h"
// --- end dr_wav ---

// Custom Widget/Component Headers
#include "componentlibrary.hpp"
#include "widgets/Magpie125.hpp"
#include "widgets/Song60.hpp"

extern rack::Plugin* pluginInstance;

// --- Custom ParamQuantity for Duration Display ---
struct DurationParamQuantity : rack::engine::ParamQuantity {
	std::string getDisplayValueString() override {
		float internalValue = getValue();
		const float minDuration = 0.05f;
		const float maxDuration = 3.0f;
		float calculatedDuration = minDuration * powf(maxDuration / minDuration, internalValue);
		char buffer[50];
		snprintf(buffer, sizeof(buffer), "%.2f s", calculatedDuration);
		return std::string(buffer);
	}
};


// --- Helper Functions: Envelope Segments ---
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
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    // --- Configure Parameters ---
    configParam<DurationParamQuantity>(TOTAL_DURATION_PARAM, 0.f, 1.f, 0.5f, "Duration");
    configParam(DUTY_CYCLE_PARAM, 0.f, 1.f, 0.5f, "Duty Cycle", "%", 0.f, 100.f);
    configParam(BIAS_PARAM, 0.f, 1.f, 0.5f, "Bias");
    configParam(DURATION_ATTEN_PARAM, -1.f, 1.f, 0.f, "Duration CV Atten");
    configParam(DUTY_ATTEN_PARAM, -1.f, 1.f, 0.f, "Duty Cycle CV Atten");
    configParam(BIAS_ATTEN_PARAM, -1.f, 1.f, 0.f, "Bias CV Atten");

    // --- Configure Inputs (with Tooltips/Labels) ---
    configInput(CLOCK_INPUT, "Clock/Trigger Input");
    configInput(AUDIO_INPUT, "Audio Input");
    configInput(DURATION_CV_INPUT, "Duration CV Input");
    configInput(DUTY_CV_INPUT, "Duty Cycle CV Input");
    configInput(BIAS_CV_INPUT, "Bias CV Input");

    // --- Configure Outputs (with Tooltips/Labels) ---
    configOutput(AUDIO_OUTPUT, "Audio Output");
    configOutput(ENV_OUTPUT, "Envelope Output");


    // --- Sample Loading Logic ---
    isRunning = false; prevGateHigh = false; clockPhase = 0.f; phase = 0.f;
    samplePlaybackPhase = 0.0; currentSampleIndex = 0;
    const std::vector<std::pair<std::string, std::string>> sampleFiles = {
        {"res/sounds/EngineRoom.wav", "Engine Room"},
        {"res/sounds/AlienBreath.wav", "Alien Breath"},
        {"res/sounds/Siren.wav", "Siren"}
    };
    loadedSamples.clear();
    std::vector<std::string> sampleDisplayNames;
    loadedSamples.reserve(sampleFiles.size());
    sampleDisplayNames.reserve(sampleFiles.size());
    for (const auto& filePair : sampleFiles) {
        const std::string& relPath = filePair.first; const std::string& displayName = filePair.second;
        try {
            std::string fullPath = asset::plugin(pluginInstance, relPath); SampleData loadedData;
            if (loadSample(fullPath, loadedData)) {
                loadedSamples.push_back(loadedData); sampleDisplayNames.push_back(displayName);
            } else { WARN("Skipping sample: %s", relPath.c_str()); }
        } catch (...) { WARN("Unknown exception loading: %s", relPath.c_str()); }
    }
    INFO("Finished loading samples. %zu samples loaded.", loadedSamples.size());
    // --- End Sample Loading ---

    // Configure Sample Select Param using configSwitch
    int numSamples = loadedSamples.size();
    float maxIndex = (numSamples > 0) ? (float)(numSamples - 1) : 0.f;
    configSwitch(SAMPLE_SELECT_PARAM, 0.f, maxIndex, 0.f, "Sample", sampleDisplayNames);

    onReset();
}

// --- onReset Method ---
void Thrum::onReset() {
    phase = 0.f; clockPhase = 0.f; isRunning = false; prevGateHigh = false;
    samplePlaybackPhase = 0.0;
    if (loadedSamples.empty()) { currentSampleIndex = -1; }
    else { currentSampleIndex = rack::math::clamp(0, 0, (int)loadedSamples.size() - 1); }
}


// --- process Method ---
void Thrum::process(const ProcessArgs& args) {

    // --- Read Main Parameters ---
    float durationKnobValue = params[TOTAL_DURATION_PARAM].getValue(); // Raw 0-1 value
    float dutyBase = params[DUTY_CYCLE_PARAM].getValue();
    float biasBase = params[BIAS_PARAM].getValue();

    // --- Read CV Inputs and Attenuverters ---
    float durationCv = inputs[DURATION_CV_INPUT].isConnected() ? inputs[DURATION_CV_INPUT].getVoltageSum() : 0.f;
    float dutyCv = inputs[DUTY_CV_INPUT].isConnected() ? inputs[DUTY_CV_INPUT].getVoltageSum() : 0.f;
    float biasCv = inputs[BIAS_CV_INPUT].isConnected() ? inputs[BIAS_CV_INPUT].getVoltageSum() : 0.f;

    float durationAtten = params[DURATION_ATTEN_PARAM].getValue();
    float dutyAtten = params[DUTY_ATTEN_PARAM].getValue();
    float biasAtten = params[BIAS_ATTEN_PARAM].getValue();

    // --- Apply CV Modulation ---
    const float durationLinearCvScale = 0.1f;
    const float dutyBiasCvScale = 0.1f;

    float linearDurationValue = durationKnobValue + (durationCv * durationAtten * durationLinearCvScale);
    linearDurationValue = rack::math::clamp(linearDurationValue, 0.f, 1.f);

    const float minDuration = 0.05f;
    const float maxDuration = 3.0f;
    float totalDuration = minDuration * powf(maxDuration / minDuration, linearDurationValue);

    float duty = dutyBase + (dutyCv * dutyAtten * dutyBiasCvScale);
    float bias = biasBase + (biasCv * biasAtten * dutyBiasCvScale);

    duty = rack::math::clamp(duty, 0.f, 1.f);
    bias = rack::math::clamp(bias, 0.f, 1.f);

    // --- Read Other Inputs ---
    bool audioInputConnected = inputs[AUDIO_INPUT].isConnected();
    float inVal = audioInputConnected ? inputs[AUDIO_INPUT].getVoltageSum() : 0.f;

    // --- Sample Selection Logic ---
    int desiredSampleIndex = static_cast<int>(params[SAMPLE_SELECT_PARAM].getValue());
    if (!loadedSamples.empty()) {
        if (desiredSampleIndex < 0) desiredSampleIndex = 0;
        if (desiredSampleIndex >= (int)loadedSamples.size()) desiredSampleIndex = (int)loadedSamples.size() - 1;
        if (desiredSampleIndex != currentSampleIndex) samplePlaybackPhase = 0.0;
        currentSampleIndex = desiredSampleIndex;
    } else { currentSampleIndex = -1; }
    // --- End Sample Selection ---


    // --- Envelope Calculation Logic ---
    float envelopeDuration = totalDuration * rack::math::clamp(duty, 0.01f, 0.99f);
    envelopeDuration = fmaxf(envelopeDuration, 1e-6f);
    float peakFraction = rack::math::clamp(bias, 0.f, 1.f);
    float p = envelopeDuration * peakFraction;
    p = fmaxf(1e-6f, fminf(p, envelopeDuration - 1e-6f));

    float env = 0.f; float t = 0.f; float calculatedEnv = 0.f;
    bool printDebug = (++processCounter % 4096 == 0);

    bool clocked = inputs[CLOCK_INPUT].isConnected();
    if (clocked) { // Clocked Mode Logic
        float gate = inputs[CLOCK_INPUT].getVoltageSum(); bool gateHigh = gate >= 1.f;
        if (!prevGateHigh && gateHigh) { isRunning = true; clockPhase = 0.f; }
        prevGateHigh = gateHigh;
        if (isRunning) {
            clockPhase += args.sampleTime; t = clockPhase;
            if (clockPhase >= totalDuration) { isRunning = false; calculatedEnv = 0.f; }
            else {
                if (t <= envelopeDuration) { calculatedEnv = (t <= p) ? attackSegment(t, p) : decaySegment(t, envelopeDuration, p); }
                else { calculatedEnv = 0.f; }
            }
        } else { calculatedEnv = 0.f; }
        env = calculatedEnv;
    } else { // Free-running Mode Logic
        if (isRunning || prevGateHigh) { isRunning = false; clockPhase = 0.f; prevGateHigh = false; }
        phase += args.sampleTime;
        if (phase >= totalDuration) { phase -= totalDuration; if (phase < 0.f) phase = 0.f; }
        t = phase;
        if (t <= envelopeDuration) { calculatedEnv = (t <= p) ? attackSegment(t, p) : decaySegment(t, envelopeDuration, p); }
        else { calculatedEnv = 0.f; }
        env = calculatedEnv;
    }
    env = rack::math::clamp(env, 0.f, 10.f);
    // --- End Envelope Calculation ---


    // --- Audio Output Logic ---
    float audioOutputValue = 0.f;
    if (audioInputConnected) { audioOutputValue = inVal * (env / 10.f); }
    else {
        if (currentSampleIndex >= 0 && currentSampleIndex < (int)loadedSamples.size() && !loadedSamples[currentSampleIndex].buffer.empty()) {
            const SampleData& currentSample = loadedSamples[currentSampleIndex]; const std::vector<float>& buffer = currentSample.buffer; size_t bufferSize = buffer.size();
            if (bufferSize > 0) {
                double phaseIncrement = currentSample.nativeRate / args.sampleRate; samplePlaybackPhase += phaseIncrement;
                samplePlaybackPhase = fmod(samplePlaybackPhase, (double)bufferSize); if (samplePlaybackPhase < 0.0) { samplePlaybackPhase += bufferSize; }
                int index0 = static_cast<int>(samplePlaybackPhase); int index1 = index0 + 1;
                if (index1 >= (int)bufferSize) { index1 -= bufferSize; } if (index0 < 0 || index0 >= (int)bufferSize) { index0 = 0; }
                float frac = samplePlaybackPhase - index0; float sampleValue = rack::math::crossfade(buffer[index0], buffer[index1], frac);
                audioOutputValue = (sampleValue * 5.0f) * (env / 10.0f);
            } else { audioOutputValue = 0.f; }
        } else { audioOutputValue = 0.f; }
    }
    // --- End Audio Output ---

    // Set Outputs
    outputs[ENV_OUTPUT].setVoltage(env);
    outputs[AUDIO_OUTPUT].setVoltage(audioOutputValue);
}


// --- ThrumWidget Constructor ---
ThrumWidget::ThrumWidget(Thrum* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Thrum.svg")));

    // --- Add Controls Row by Row with User's Hardcoded Positions ---
    // Tooltips are now handled by configInput/configOutput/configParam/configSwitch

    // Duration Row (Y ~ 20-25)
    addParam(createParamCentered<Magpie125>(mm2px(Vec(9.f, 20.f)), module, Thrum::TOTAL_DURATION_PARAM));
    addParam(createParamCentered<Song60>(mm2px(Vec(23.f, 22.25f)), module, Thrum::DURATION_ATTEN_PARAM));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.f, 22.33f)), module, Thrum::DURATION_CV_INPUT));

    // Duty Cycle Row (Y ~ 45-50)
    addParam(createParamCentered<Magpie125>(mm2px(Vec(9.f, 39.f)), module, Thrum::DUTY_CYCLE_PARAM));
    addParam(createParamCentered<Song60>(mm2px(Vec(23.f, 41.25f)), module, Thrum::DUTY_ATTEN_PARAM));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.f, 41.33f)), module, Thrum::DUTY_CV_INPUT));

    // Bias Row (Y ~ 70-75)
    addParam(createParamCentered<Magpie125>(mm2px(Vec(9.f, 58.f)), module, Thrum::BIAS_PARAM));
    addParam(createParamCentered<Song60>(mm2px(Vec(23.f, 60.25f)), module, Thrum::BIAS_ATTEN_PARAM));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.f, 60.33f)), module, Thrum::BIAS_CV_INPUT));

    // Sample Select Knob (Y ~ 95)
    addParam(createParamCentered<Song60>(mm2px(Vec(24.f, 92.5f)), module, Thrum::SAMPLE_SELECT_PARAM));

    // Bottom Row Jacks (User's Positions)
    addInput (createInputCentered<PJ301MPort>(mm2px(Vec(9.f, 80.f)), module, Thrum::CLOCK_INPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(31.f, 105.f)), module, Thrum::AUDIO_OUTPUT));
    addInput (createInputCentered<PJ301MPort>(mm2px(Vec(9.f, 105.f)), module, Thrum::AUDIO_INPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(31.f, 80.f)), module, Thrum::ENV_OUTPUT));

    // Screws
    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
}

