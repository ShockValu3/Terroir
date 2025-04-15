#ifndef THRUM_HPP
#define THRUM_HPP

#include "rack.hpp"

using namespace rack;

// Extern declaration for the global plugin instance.
extern rack::Plugin* pluginInstance;

struct Thrum : Module {
	enum ParamIds {
		PULSE_DURATION_PARAM,  // Controls how long the pulse lasts (active envelope duration)
		PULSE_RATE_PARAM,      // Controls how often the pulse fires (Hz)
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		ENV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	// Phase accumulator in seconds
	float phase = 0.f;

	Thrum();
	void process(const ProcessArgs& args) override;
};

struct ThrumWidget : ModuleWidget {
	ThrumWidget(Thrum* module);
};

#endif // THRUM_HPP
