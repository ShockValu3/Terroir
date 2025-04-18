#include "Thrum.hpp" // Your module header
#include "rack.hpp" // Core Rack header
#include "plugin.hpp" // For INFO() logging macro
#include <cmath>     // For fmaxf, fminf, etc.

// Include headers for any custom widgets if not already in Thrum.hpp
#include "componentlibrary.hpp"
#include "widgets/Magpie125.hpp"
#include "widgets/Song60.hpp"

// If pluginInstance is used for assets, ensure it's declared (usually in plugin.cpp)
extern rack::Plugin* pluginInstance;

// --- Helper Functions (Attack/Decay Segments) ---
// These should be the versions with safety checks we developed earlier
static const float k = 5.f; // Exponential shape constant

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


// --- Thrum Constructor ---
Thrum::Thrum() {
    // Initialize parameters, inputs, outputs, and lights count
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    // Configure Duration Param: Direct mapping 0.05s to 1.0s
    configParam(TOTAL_DURATION_PARAM,
                0.05f, // minValue = 0.05 seconds
                1.0f,  // maxValue = 1.0 seconds
                0.505f, // defaultValue (~0.5s)
                "Duration", // Label
                " s");      // Unit (seconds)

    // Configure Duty Cycle Param (standard 0.0 to 1.0)
    configParam(DUTY_CYCLE_PARAM,
                0.f, 1.f, 0.5f,
                "Duty Cycle", // Label
                "%", 0.f, 100.f); // Unit %, display multiplier

    // Configure Bias Param (standard 0.0 to 1.0)
    configParam(BIAS_PARAM,
                0.f, 1.f, 0.5f,
                "Bias"); // Label

    // Initialize state variables
    isRunning = false;
    prevGateHigh = false;
    clockPhase = 0.f;
    phase = 0.f;
    processCounter = 0; // Initialize the per-instance debug counter
}

// --- Thrum Process Function ---
void Thrum::process(const ProcessArgs& args) {
    // Get params
    // Get Duration directly in seconds (0.05 to 1.0)
    float totalDuration = params[TOTAL_DURATION_PARAM].getValue();
    // Optional safety clamp (in case value somehow goes out of bounds)
    totalDuration = rack::clamp(totalDuration, 0.05f, 1.0f);

    float duty  = params[DUTY_CYCLE_PARAM].getValue(); // 0.0 to 1.0
    float bias  = params[BIAS_PARAM].getValue();     // 0.0 to 1.0
    float inVal = inputs[AUDIO_INPUT].isConnected() ? inputs[AUDIO_INPUT].getVoltage() : 0.f;

    // --- Calculate envelope shape parameters ---
    float envelopeDuration = totalDuration * rack::clamp(duty, 0.f, 1.f);
    envelopeDuration = fmaxf(envelopeDuration, 1e-6f); // Ensure non-negative & slightly positive

    float peakFraction = 0.5f + (rack::clamp(bias, 0.f, 1.f) - 0.5f) * 0.8f; // 0.1 to 0.9
    float p = envelopeDuration * peakFraction; // Peak time
    p = fmaxf(1e-6f, fminf(p, envelopeDuration - 1e-6f)); // Clamp peak time safely

    // --- Initialize output variables ---
    float env = 0.f;
    float t = 0.f;

    // --- Logging Setup ---
    bool printDebug = (++processCounter % 4096 == 0); // Use member counter

    if (printDebug) {
        INFO("Duty: %.2f, Bias: %.2f, totalDur: %.3fs, envDur: %.3fs, p: %.3fs",
              duty, bias, totalDuration, envelopeDuration, p);
    }

    // --- Determine mode (Clocked or Free-running) ---
    bool clocked = inputs[CLOCK_INPUT].isConnected();

    if (clocked) {
        // --- Clocked (Triggered) Mode ---
        float gate = inputs[CLOCK_INPUT].getVoltage();
        bool gateHigh = gate >= 1.f;

        if (!prevGateHigh && gateHigh) {
             if (printDebug) INFO("Clocked: TRIGGER received.");
             isRunning = true;
             clockPhase = 0.f;
        }
        prevGateHigh = gateHigh;

        if (isRunning) {
            clockPhase += args.sampleTime;

            if (clockPhase >= totalDuration) {
                 if (printDebug) INFO("Clocked: Hit end. phase=%.3f", clockPhase);
                 isRunning = false;
                 env = 0.f;
                 clockPhase = totalDuration;
            } else {
                t = clockPhase;
                if (t <= envelopeDuration) {
                     if (t <= p) { env = attackSegment(t, p); }
                     else { env = decaySegment(t, envelopeDuration, p); }
                } else {
                     if (printDebug) INFO("Clocked: Post-Env (Duty < 1.0). t=%.3f", t);
                     env = 0.f;
                }
            }
        } else {
             env = 0.f;
        }

    } else {
        // --- Free-running Mode ---
        if (isRunning) { isRunning = false; clockPhase = 0.f; }

        phase += args.sampleTime;
        if (phase >= totalDuration) {
             phase -= totalDuration;
             if (phase < 0.f) phase = 0.f;
        }
        t = phase;

         if (t <= envelopeDuration) {
             if (t <= p) { env = attackSegment(t, p); }
             else { env = decaySegment(t, envelopeDuration, p); }
        } else {
             if (printDebug) INFO("FreeRun: Post-Env (Duty < 1.0). t=%.3f", t);
             env = 0.f;
        }
    }

    // Final safety clamp
    env = rack::clamp(env, 0.f, 10.f);

    // --- Set Outputs ---
    outputs[ENV_OUTPUT].setVoltage(env);
    outputs[AUDIO_OUTPUT].setVoltage(inVal * (env / 10.f));
}


// --- ThrumWidget Definition (Make sure this is UNCOMMENTED in your file) ---
ThrumWidget::ThrumWidget(Thrum* module) {
    setModule(module);
    // Assuming 'pluginInstance' is correctly defined/extern'd for asset::plugin
    // If not, you might need to adjust how the panel SVG is loaded
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Thrum.svg")));

    // Add Params using appropriate widgets (Magpie125, Song60, etc.)
    // Ensure widget types match your includes (e.g., Magpie125.hpp)
    addParam(createParamCentered<Magpie125>(mm2px(Vec(20, 10)), module, Thrum::TOTAL_DURATION_PARAM));
    addParam(createParamCentered<Magpie125>(mm2px(Vec(20, 30)), module, Thrum::DUTY_CYCLE_PARAM));
    addParam(createParamCentered<Magpie125>(mm2px(Vec(20, 50)), module, Thrum::BIAS_PARAM));

    // Add Inputs/Outputs using PJ301MPort or appropriate widget
    addInput (createInputCentered<PJ301MPort>(mm2px(Vec(10, 70)), module, Thrum::CLOCK_INPUT));
    addInput (createInputCentered<PJ301MPort>(mm2px(Vec(30, 70)), module, Thrum::AUDIO_INPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10, 90 )), module, Thrum::AUDIO_OUTPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30,90)), module, Thrum::ENV_OUTPUT));

    // Add Screws using ThemedScrew or appropriate widget
    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
}