#include "Thrum.hpp"
#include "rack.hpp"
#include "componentlibrary.hpp"

using namespace rack;
using namespace rack::app;

Thrum::Thrum() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, 0);
    
    configParam(RATE_PARAM, -5.f, 10.f, 0.f, "How often we pulse");
    configParam(DURATION_PARAM, -5.f, 10.f, 10.f, "Pulse length");
    
    configParam(RATE_ATTENUVERTER, -1.f, 1.f, 0.f, "Rate Attenuverter");
    configParam(DURATION_ATTENUVERTER, -1.f, 1.f, 0.f, "Duration Attenuverter");
}

ThrumWidget::ThrumWidget(Thrum* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Thrum.svg")));

        addParam(createParamCentered<RoundBlackKnob>(
            window::mm2px(math::Vec(9, 20)), module, Thrum::RATE_PARAM));
        addParam(createParamCentered<Trimpot>(
            mm2px(Vec(23, 20)), module, Thrum::RATE_ATTENUVERTER));
        addInput(createInputCentered<PJ301MPort>(
            mm2px(Vec(34, 20)), module, Thrum::RATE_INPUT));

        addParam(createParamCentered<RoundBlackKnob>(
            mm2px(Vec(9, 40)), module, Thrum::DURATION_PARAM));
        addParam(createParamCentered<Trimpot>(
            mm2px(Vec(23, 40)), module, Thrum::DURATION_ATTENUVERTER));
        addInput(createInputCentered<PJ301MPort>(
            mm2px(Vec(34, 40)), module, Thrum::DURATION_INPUT));    

        addOutput(createOutputCentered<componentlibrary::PJ301MPort>(
           mm2px(math::Vec(15, 112)), module, Thrum::THRUM_OUT));
		addOutput(createOutputCentered<componentlibrary::PJ301MPort>(
			mm2px(math::Vec(25, 112)), module, Thrum::ENVELOPE_OUT));

		   addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
		   addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		   addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		   addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
}


void Thrum::process(const ProcessArgs& args) {
	
}
