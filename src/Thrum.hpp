#pragma once

#include <rack.hpp>

extern rack::Plugin* pluginInstance;

struct Thrum : rack::Module {

	enum ParamIds {
		RATE_PARAM,
		RATE_ATTENUVERTER,
		DURATION_PARAM,
		DURATION_ATTENUVERTER,
		NUM_PARAMS
	};

	enum InputIds {
		RATE_INPUT,
		DURATION_INPUT,
		NUM_INPUTS
	};

	enum OutputIds {
		THRUM_OUT,
		ENVELOPE_OUT,
		NUM_OUTPUTS
	};

	Thrum();

	void process(const ProcessArgs& args) override;

};

struct ThrumWidget : rack::ModuleWidget {
	ThrumWidget(Thrum* module);
};
