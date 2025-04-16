#ifndef THRUM_HPP
#define THRUM_HPP

#include "rack.hpp"

using namespace rack;

// Extern declaration for the global plugin instance.
extern rack::Plugin* pluginInstance;

struct Thrum : Module {
	enum ParamIds {
		PULSE_DURATION_PARAM, // Active envelope duration (seconds)
		PULSE_RATE_PARAM,     // Pulse rate (Hz)
		BIAS_PARAM,           // Bias/slant (0=early peak, 0.5=symmetric, 1=late peak)
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

	// Phase accumulator (seconds); resets every period
	float phase = 0.f;

	Thrum();
	void process(const ProcessArgs& args) override;
};

struct ThrumWidget : ModuleWidget {
	ThrumWidget(Thrum* module);
};

#endif // THRUM_HPP
