#include "Wend.hpp"
extern rack::Plugin* pluginInstance;
#include "componentlibrary.hpp"
#include "widgets/Magpie125.hpp"
#include "widgets/Song60.hpp"

extern Plugin* pluginInstance;

WendWidget::WendWidget(Wend* module) {
    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Wend.svg")));

    // Screws
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    // Frequency knob
    addParam(createParamCentered<Magpie125>(
        Vec(30.f, 60.f), module, Wend::FREQ_PARAM));

    // Audio output
    addOutput(createOutputCentered<PJ301MPort>(
        Vec(30.f, 120.f), module, Wend::AUDIO_OUTPUT));
}

void Wend::process(const ProcessArgs& args) {
    float freqMult = params[FREQ_PARAM].getValue();
    phase += freqMult * args.sampleTime * 2.f * M_PI;
    if (phase > 2.f * M_PI)
        phase -= 2.f * M_PI;

    float signal = sinf(phase);
    outputs[AUDIO_OUTPUT].setVoltage(5.f * signal);
}

