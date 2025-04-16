#include "Thrum.hpp"
#include "rack.hpp"
#include "componentlibrary.hpp"
#include "widgets/Magpie125.hpp"
#include "widgets/Song60.hpp"
#include <cmath>

// Extern declaration for the plugin instance.
extern Plugin* pluginInstance;

using namespace rack;

// Shape parameter for both the attack and decay curves.
// Larger k = more pronounced knee near the peak.
static const float k = 5.f;

/**
 * Attack Segment
 *
 * Mirrors the decay formula so the shape is reversed in time.
 * For t in [0, p], we define a normalized variable v = (p - t) / p.
 * That ensures v=1 at t=0 (start of attack => envelope=0) and v=0 at t=p (peak => envelope=10).
 *
 * Then we compute:
 *    envelope = 10 * [ (exp(-k * v) - exp(-k)) / (1 - exp(-k)) ]
 *
 * => Attack(0)=0 V, Attack(p)=10 V, matching the slope of the decay but in reverse.
 */
static float attackSegment(float t, float p) {
    // Normalize time from 1 down to 0.
    float v = (p - t) / p;
    // Plug into the exponential formula.
    float numerator   = std::exp(-k * v) - std::exp(-k);
    float denominator = 1.f - std::exp(-k);
    float value       = 10.f * (numerator / denominator);

    // Clamp to safety in case of small floating discrepancies.
    return clamp(value, 0.f, 10.f);
}

/**
 * Decay Segment
 *
 * The original exponential decay function.
 * For t in [p, pulseDuration], define u = (t - p) / (pulseDuration - p).
 * => at t=p => u=0 => envelope=10
 * => at t=pulseDuration => u=1 => envelope=0
 */
static float decaySegment(float t, float pulseDuration, float p) {
    // Normalize time from 0..1 across [p..pulseDuration].
    float u = (t - p) / (pulseDuration - p);
    float numerator   = std::exp(-k * u) - std::exp(-k);
    float denominator = 1.f - std::exp(-k);
    float value       = 10.f * (numerator / denominator);
    return clamp(value, 0.f, 10.f);
}

Thrum::Thrum() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);

    // 1) Pulse Duration: how long the envelope stays active each cycle (0.05..2 s).
    configParam(PULSE_DURATION_PARAM, 0.05f, 2.f, 1.f, "Pulse Duration", " s");

    // 2) Pulse Rate: how many pulses per second (0.5..20 Hz).
    configParam(PULSE_RATE_PARAM, 0.5f, 20.f, 2.f, "Pulse Rate", " Hz");

    // 3) Bias (0=peak very early, 1=peak very late, 0.5=symmetric).
    configParam(BIAS_PARAM, 0.f, 1.f, 0.5f, "Bias", "");
}

void Thrum::process(const ProcessArgs& args) {
    // Read parameter values from knobs.
    float pulseDuration = params[PULSE_DURATION_PARAM].getValue();
    float pulseRate     = params[PULSE_RATE_PARAM].getValue();
    float bias          = params[BIAS_PARAM].getValue();

    // Convert pulse rate to period (seconds per cycle).
    float period = 1.f / pulseRate;

    // Increment phase (time since last trigger).
    phase += args.sampleTime;
    // If we've passed one full period, wrap the phase = re-trigger.
    if (phase >= period) {
        phase -= period;
    }

    float env = 0.f;
    // The envelope is active only in [0, pulseDuration].
    if (phase < pulseDuration) {
        // Determine the time of the peak p based on bias.
        // e.g. bias=0 => ~10% in; bias=1 => ~90% in; bias=0.5 => 50% in, etc.
        float peakFraction = 0.1f + 0.8f * bias; // shift range from [0,1] => [0.1,0.9]
        float p = pulseDuration * peakFraction;

        if (phase <= p) {
            // Attack portion: mirror of decay.
            env = attackSegment(phase, p);
        } else {
            // Decay portion: from p to pulseDuration.
            env = decaySegment(phase, pulseDuration, p);
        }
    }
    // Otherwise, env remains 0 for the remainder of the period (the "pause").

    // Output the computed envelope voltage.
    outputs[ENV_OUTPUT].setVoltage(env);
}

ThrumWidget::ThrumWidget(Thrum* module) {
    setModule(module);
    // Set up the SVG panel (ensure "res/Thrum.svg" is present).
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Thrum.svg")));

    // Position the knobs at (20,20), (20,40), (20,60).
    addParam(createParamCentered<Magpie125>(mm2px(Vec(20, 20)), module, Thrum::PULSE_DURATION_PARAM));
    addParam(createParamCentered<Magpie125>(mm2px(Vec(20, 40)), module, Thrum::PULSE_RATE_PARAM));
    addParam(createParamCentered<Magpie125>(mm2px(Vec(20, 60)), module, Thrum::BIAS_PARAM));

    // The ENV output jack at (20, 110).
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20, 110)), module, Thrum::ENV_OUTPUT));

    // Add four screws for the typical Terroir aesthetic.
    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
}
