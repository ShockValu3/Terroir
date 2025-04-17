#include "Thrum.hpp"
#include "rack.hpp"
#include "componentlibrary.hpp"
#include "widgets/Magpie125.hpp"
#include "widgets/Song60.hpp"
#include <cmath>

extern Plugin* pluginInstance;
using namespace rack;

// Exponential shape constant
static const float k = 5.f;
// Discrete clock‐rate table (musical divisions/multiples)
static constexpr int RATE_STEPS = 10;
static const float rateTable[RATE_STEPS] = {
    1/16.f, 1/12.f, 1/8.f,  1/4.f,  1/2.f,
    1.f,    2.f,    4.f,    8.f,    16.f
};

/**
 * Attack segment: mirror of decay.
 */
static float attackSegment(float t, float p) {
    float v = (p - t) / p;
    float num = std::exp(-k * v) - std::exp(-k);
    float den = 1.f - std::exp(-k);
    return clamp(10.f * (num / den), 0.f, 10.f);
}

/**
 * Decay segment.
 */
static float decaySegment(float t, float activeDuration, float p) {
    float u = (t - p) / (activeDuration - p);
    float num = std::exp(-k * u) - std::exp(-k);
    float den = 1.f - std::exp(-k);
    return clamp(10.f * (num / den), 0.f, 10.f);
}

Thrum::Thrum() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
    configParam(TOTAL_DURATION_PARAM, 0.f, 1.f, 0.5f, "Rate / Duration", "");
    configParam(DUTY_CYCLE_PARAM,   0.f, 1.f, 0.5f, "Duty Cycle", "");
    configParam(BIAS_PARAM,         0.f, 1.f, 0.5f, "Bias", "");
}


ThrumWidget::ThrumWidget(Thrum* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Thrum.svg")));

    addParam(createParamCentered<Magpie125>(mm2px(Vec(20, 10)), module, Thrum::TOTAL_DURATION_PARAM));
    addParam(createParamCentered<Magpie125>(mm2px(Vec(20, 30)), module, Thrum::DUTY_CYCLE_PARAM));
    addParam(createParamCentered<Magpie125>(mm2px(Vec(20, 50)), module, Thrum::BIAS_PARAM));

    addInput (createInputCentered<PJ301MPort>(mm2px(Vec(10, 70)), module, Thrum::CLOCK_INPUT));
    addInput (createInputCentered<PJ301MPort>(mm2px(Vec(30, 70)), module, Thrum::AUDIO_INPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10, 90 )), module, Thrum::AUDIO_OUTPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30,90)), module, Thrum::ENV_OUTPUT));

    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
}
void Thrum::process(const ProcessArgs& args) {
    // 0) Knobs
    float dKnob = params[TOTAL_DURATION_PARAM].getValue();  // 0..1
    float duty  = params[DUTY_CYCLE_PARAM].getValue();      // 0..1
    float bias  = params[BIAS_PARAM].getValue();            // 0..1

    bool clocked = inputs[CLOCK_INPUT].isConnected();
    float env = 0.f;
    float in  = inputs[AUDIO_INPUT].getVoltage();

    // These will get filled in below:
    float effectivePeriod;
    float activeDuration;
    float t;  // time since our last retrigger

    if (clocked) {
        // 1) Edge‑detect & measure clock
        float gate     = inputs[CLOCK_INPUT].getVoltage();
        bool  gateHigh = gate >= 1.f;
        if (!prevGateHigh && gateHigh) {
            // Rising edge!
            // — Measure period —
            if (haveClockPeriod) {
                clockPeriod = clockTimer;
            } else {
                haveClockPeriod = true;
            }
            clockTimer = 0.f;

            // — Count edges for divisions —
            edgeCount++;

            // — First ever edge: lock to downbeat —
            if (firstEdge) {
                clockPhase = 0.f;
                firstEdge  = false;
            }

            // — If division mode, retrigger on the Nth beat —
            // We'll fill in divisionFactor below once we know rateMul.
        }
        prevGateHigh = gateHigh;
        clockTimer  += args.sampleTime;

        // 2) Quantize the Rate/Dur knob into your table
        int idx = int(std::roundf(dKnob * (RATE_STEPS - 1)));
        idx = clamp(idx, 0, RATE_STEPS - 1);
        float rateMul = rateTable[idx];

        // 3) Compute the scaled period
        effectivePeriod = haveClockPeriod
            ? (clockPeriod / rateMul)  // CW = faster
            : 1.f;                     // fallback

        // 4) Handle division vs subdivision
        if (rateMul <= 1.f) {
            // Division (incl. 1×)
            // e.g. rateMul=0.5 → divisionFactor=2 → one pulse every 2 beats
            int divisionFactor = int(std::roundf(1.f / rateMul));
            if (edgeCount >= divisionFactor) {
                edgeCount   = 0;
                clockPhase  = 0.f;  // retrigger now
            }
            // phase just counts up from that retrigger
            clockPhase += args.sampleTime;
        } else {
            // Subdivision (>1×)
            // only wrap, no retrigger on edges
            clockPhase += args.sampleTime;
            if (clockPhase >= effectivePeriod) {
                clockPhase -= effectivePeriod;
            }
        }

        // 5) Build envelope off clockPhase
        t = clockPhase;
        activeDuration = effectivePeriod * duty;
        if (t < activeDuration) {
            float peakFraction = 0.1f + 0.8f * bias;
            float p = activeDuration * peakFraction;
            env = (t <= p)
                ? attackSegment(t, p)
                : decaySegment(t, activeDuration, p);
        } else {
            env = 0.f;
        }

    } else {
        // — Free‑run (no clock) —
        float totalDuration = 0.05f + 1.95f * dKnob;  // [0.05..2.0]s
        effectivePeriod = totalDuration;
        activeDuration  = effectivePeriod * duty;

        phase += args.sampleTime;
        if (phase >= effectivePeriod)
            phase -= effectivePeriod;
        t = phase;

        if (t < activeDuration) {
            float peakFraction = 0.1f + 0.8f * bias;
            float p = activeDuration * peakFraction;
            env = (t <= p)
                ? attackSegment(t, p)
                : decaySegment(t, activeDuration, p);
        } else {
            env = 0.f;
        }
    }

    // 6) Outputs
    outputs[ENV_OUTPUT].setVoltage(env);
    outputs[AUDIO_OUTPUT].setVoltage(in * (env / 10.f));
}

