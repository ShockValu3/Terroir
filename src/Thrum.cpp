#include "Thrum.hpp"
#include "rack.hpp"
#include "componentlibrary.hpp"
#include "widgets/Magpie125.hpp"
#include "widgets/Song60.hpp"
#include <cmath>

// Extern for plugin instance.
extern Plugin* pluginInstance;

using namespace rack;

// This helper maps time t in [0, activeDuration] onto an x value in [-1, 1]
// and then returns an exponential envelope value that goes from 0 V at t=0,
// peaks at 10 V at t=activeDuration/2, and back down to 0 V at t=activeDuration.
static float exponentialBell(float t, float activeDuration, float k) {
    float center = activeDuration / 2.f;
    // Map t in [0, activeDuration] linearly to x in [-1, 1]:
    float x = 2.f * (t - center) / activeDuration;
    float numerator   = std::exp(-k * std::fabs(x)) - std::exp(-k);
    float denominator = 1.f - std::exp(-k);
    float E = 10.f * (numerator / denominator);
    return clamp(E, 0.f, 10.f);
}

Thrum::Thrum() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
    // Configure the pulse duration knob: range from 0.05 s to 2 s (active pulse length)
    configParam(PULSE_DURATION_PARAM, 0.05f, 2.f, 0.5f, "Pulse Duration", " s");
    // Configure the pulse rate knob: range from 0.5 Hz (slow) to 20 Hz (fast)
    configParam(PULSE_RATE_PARAM, 0.5f, 20.f, 2.f, "Pulse Rate", " Hz");
}

void Thrum::process(const ProcessArgs& args) {
    // Get our parameter values.
    float pulseDuration = params[PULSE_DURATION_PARAM].getValue();
    float pulseRate     = params[PULSE_RATE_PARAM].getValue();
    float period        = 1.f / pulseRate;
    
    // Increment phase; phase is the time since the last trigger.
    phase += args.sampleTime;
    
    // When the period is completed, reset phase (new trigger).
    if (phase >= period) {
        phase -= period;
    }
    
    // Choose the shape constant (k) for the exponential envelope.
    static const float k = 5.f;
    float env = 0.f;
    
    // Compute the envelope based on the time since last trigger, regardless of period.
    if (phase < pulseDuration) {
        env = exponentialBell(phase, pulseDuration, k);
    } else {
        env = 0.f;
    }
    
    outputs[ENV_OUTPUT].setVoltage(env);
}

ThrumWidget::ThrumWidget(Thrum* module) {
    setModule(module);
    // Set the panel SVG – make sure "res/Thrum.svg" exists.
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Thrum.svg")));

    // Add the Pulse Duration knob.
    addParam(createParamCentered<Magpie125>(mm2px(Vec(10, 20)), module, Thrum::PULSE_DURATION_PARAM));
    // Add the Pulse Rate knob.
    addParam(createParamCentered<Magpie125>(mm2px(Vec(30, 20)), module, Thrum::PULSE_RATE_PARAM));

    // Add the output jack.
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20, 110)), module, Thrum::ENV_OUTPUT));

    // Add screws for the Terroir aesthetic.
    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
}