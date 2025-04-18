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
    float dKnob = params[TOTAL_DURATION_PARAM].getValue();  
    float duty  = params[DUTY_CYCLE_PARAM].getValue();      
    float bias  = params[BIAS_PARAM].getValue();            

    bool clocked = inputs[CLOCK_INPUT].isConnected();
    float env = 0.f;
    float in  = inputs[AUDIO_INPUT].getVoltage();

    float totalDuration = 0.05f + 1.95f * dKnob;

    if (duty <= 0.f) {
        outputs[ENV_OUTPUT].setVoltage(0.f);
        outputs[AUDIO_OUTPUT].setVoltage(0.f);
        return;
    }

    float envelopeDuration = totalDuration * duty;
    float peakFraction = 0.5f + (bias - 0.5f) * 0.8f;
    float p = envelopeDuration * peakFraction;

    float t = 0.f;

    if (clocked) {
        float gate = inputs[CLOCK_INPUT].getVoltage();
        bool gateHigh = gate >= 1.f;

        if (!prevGateHigh && gateHigh) {
            clockPhase = 0.f;  // retrigger
        }
        prevGateHigh = gateHigh;

        clockPhase += args.sampleTime;
        if (clockPhase > totalDuration)
            clockPhase = totalDuration;

        if (clockPhase <= envelopeDuration) {
            t = clockPhase / duty;  // stretch envelope to fit within duty
            if (t <= p) {
                env = attackSegment(t, p);
            } else {
                env = decaySegment(t, envelopeDuration, p);
            }
        } else {
            env = 0.f;
        }

    } else {
        // Free-run
        phase += args.sampleTime;
        if (phase >= totalDuration)
            phase -= totalDuration;

        if (phase <= envelopeDuration) {
            t = phase / duty;  // squash into duty window
            if (t <= p) {
                env = attackSegment(t, p);
            } else {
                env = decaySegment(t, envelopeDuration, p);
            }
        } else {
            env = 0.f;
        }
    }

    outputs[ENV_OUTPUT].setVoltage(env);
    outputs[AUDIO_OUTPUT].setVoltage(in * (env / 10.f));
}
