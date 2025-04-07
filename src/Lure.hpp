#pragma once

#include <rack.hpp>

extern rack::Plugin* pluginInstance;

struct Lure : rack::Module {

	float brownianValue = 0.f;
	int stepCounter = 0;
	int currentStepInterval = 1000; // default = ~22ms @ 44.1kHz

	enum ParamIds {
		MIN_PARAM,
		MAX_PARAM,
		BIAS_PARAM,
		PULL_PARAM,
		SPEED_PARAM,
		MIN_ATTENUVERTER,
		MAX_ATTENUVERTER,
		BIAS_ATTENUVERTER,
		PULL_ATTENUVERTER,
		SPEED_ATTENUVERTER,
		NUM_PARAMS
	};

	enum InputIds {
		MIN_INPUT,
		MAX_INPUT,
		BIAS_INPUT,
		PULL_INPUT,
		SPEED_INPUT,
		NUM_INPUTS
	};

	enum OutputIds {
		CV_OUTPUT,
		NUM_OUTPUTS
	};

	Lure();

	void process(const ProcessArgs& args) override;

private:
	float getModulatedMin();
	float getModulatedMax();
	float getBias(float lower, float upper);
	float getPullStrength();
	int getStepInterval(float speedParam, float sampleRate);
	float calculateForce(float bias, float lower, float upper, float pullParam);
};

struct LureWidget : rack::ModuleWidget {
	LureWidget(Lure* module);
};
