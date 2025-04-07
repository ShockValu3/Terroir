#include "Lure.hpp"
#include "rack.hpp"
#include "componentlibrary.hpp"



// ----------------------------------------------
// Tunable constants for behavior shaping
// ----------------------------------------------
constexpr float VOLTAGE_MIN = -5.f;
constexpr float VOLTAGE_MAX = 10.f;

constexpr float ATTENUVERTER_RANGE = 1.f;
constexpr float KNOB_BIAS_DEFAULT = 0.5f;
constexpr float KNOB_PULL_DEFAULT = 0.5f;
constexpr float KNOB_SPEED_DEFAULT = 0.5f;

constexpr float STEP_SIZE = 0.05f;             // Fixed movement per step
constexpr float EPSILON = 1e-3f;               // Prevent division by zero at edges
constexpr float EDGE_REPEL_FACTOR = 0.03f;
constexpr float EDGE_EXPONENT = 1.51f;

constexpr float SPEED_EXPONENT = 1.f;          // Speed curve shape
constexpr float SPEED_INTERVAL_MIN_MS = 0.1f;     // or lower if needed
constexpr float SPEED_INTERVAL_MAX_MS = 1000.f;

constexpr float PULL_EXPONENT = 2.0f;          // Perceptual pull shaping
constexpr float GRAVITY_EXPONENT = 2.5f;       // Distance-squared-ish gravity curve

constexpr float LED_RANGE_NORM = 10.f;         // Range scale for LED normalization

using namespace rack;
using namespace rack::app;

Lure::Lure() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, 0);
    
    configParam(MIN_PARAM, -5.f, 10.f, 0.f, "Minimum output voltage");
    configParam(MAX_PARAM, -5.f, 10.f, 10.f, "Maximum output voltage");
    configParam(BIAS_PARAM, 0.f, 1.f, 0.5f, "Bias\nDrift center between Min and Max");
    configParam(PULL_PARAM, 0.f, 1.f, 0.5f, "Pull\nGravity toward center\n0 = free walk, 1 = strongly centered");
    configParam(SPEED_PARAM, 0.f, 1.f, 0.5f, "Speed\nDrift speed\n0 = slow, 1 = fast");
    
    configParam(MIN_ATTENUVERTER, -1.f, 1.f, 0.f, "Min Attenuverter");
    configParam(MAX_ATTENUVERTER, -1.f, 1.f, 0.f, "Max Attenuverter");
    configParam(BIAS_ATTENUVERTER, -1.f, 1.f, 0.f, "Bias Attenuverter");
    configParam(PULL_ATTENUVERTER, -1.f, 1.f, 0.f, "Pull Attenuverter");
    configParam(SPEED_ATTENUVERTER, -1.f, 1.f, 0.f, "Speed Attenuverter");
}

LureWidget::LureWidget(Lure* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Lure.svg")));

        addParam(createParamCentered<RoundBlackKnob>(
            window::mm2px(math::Vec(9, 20)), module, Lure::MIN_PARAM));
        addParam(createParamCentered<Trimpot>(
            mm2px(Vec(23, 20)), module, Lure::MIN_ATTENUVERTER));
        addInput(createInputCentered<PJ301MPort>(
            mm2px(Vec(34, 20)), module, Lure::MIN_INPUT));

        addParam(createParamCentered<RoundBlackKnob>(
            mm2px(Vec(9, 40)), module, Lure::MAX_PARAM));
        addParam(createParamCentered<Trimpot>(
            mm2px(Vec(23, 40)), module, Lure::MAX_ATTENUVERTER));
        addInput(createInputCentered<PJ301MPort>(
            mm2px(Vec(34, 40)), module, Lure::MAX_INPUT));    

        addParam(createParamCentered<RoundBlackKnob>(
            mm2px(Vec(9, 60)), module, Lure::BIAS_PARAM));
        addParam(createParamCentered<Trimpot>(
            mm2px(Vec(23, 60)), module, Lure::BIAS_ATTENUVERTER));
        addInput(createInputCentered<PJ301MPort>(
            mm2px(Vec(34, 60)), module, Lure::BIAS_INPUT));
        
        addParam(createParamCentered<RoundBlackKnob>(
            mm2px(Vec(9, 80)), module, Lure::PULL_PARAM));
        addParam(createParamCentered<Trimpot>(
            mm2px(Vec(23, 80)), module, Lure::PULL_ATTENUVERTER));
        addInput(createInputCentered<PJ301MPort>(
            mm2px(Vec(34, 80)), module, Lure::PULL_INPUT));

        addParam(createParamCentered<RoundBlackKnob>(
            mm2px(Vec(9, 100)), module, Lure::SPEED_PARAM));
        addParam(createParamCentered<Trimpot>(
            mm2px(Vec(23, 100)), module, Lure::SPEED_ATTENUVERTER));
        addInput(createInputCentered<PJ301MPort>(
            mm2px(Vec(34, 100)), module, Lure::SPEED_INPUT));

        addOutput(createOutputCentered<componentlibrary::PJ301MPort>(
           mm2px(math::Vec(20.32, 112)), module, Lure::CV_OUTPUT));

		   addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
		   addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		   addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		   addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
}

float Lure::getModulatedMin() {
	float min = params[MIN_PARAM].getValue();
	if (inputs[MIN_INPUT].isConnected()) {
		float cv = inputs[MIN_INPUT].getVoltage();
		float atten = params[MIN_ATTENUVERTER].getValue();
		min += cv * atten;
	}
	return clamp(min, VOLTAGE_MIN, VOLTAGE_MAX);
}

float Lure::getModulatedMax() {
	float max = params[MAX_PARAM].getValue();
	if (inputs[MAX_INPUT].isConnected()) {
		float cv = inputs[MAX_INPUT].getVoltage();
		float atten = params[MAX_ATTENUVERTER].getValue();
		max += cv * atten;
	}
	return clamp(max, VOLTAGE_MIN, VOLTAGE_MAX);
}

float Lure::getBias(float lower, float upper) {
	float biasParam = params[BIAS_PARAM].getValue();
	if (inputs[BIAS_INPUT].isConnected()) {
		float cv = inputs[BIAS_INPUT].getVoltage();
		float atten = params[BIAS_ATTENUVERTER].getValue();
		biasParam += (cv / 10.f) * atten;
	}
	biasParam = clamp(biasParam, 0.f, 1.f);
	return lower + biasParam * (upper - lower);
}

float Lure::getPullStrength() {
	float rawPull = params[PULL_PARAM].getValue();
	if (inputs[PULL_INPUT].isConnected()) {
		float cv = inputs[PULL_INPUT].getVoltage();
		float atten = params[PULL_ATTENUVERTER].getValue();
		rawPull += (cv / 10.f) * atten;
	}
	rawPull = clamp(rawPull, 0.f, 1.f);
	return powf(rawPull, PULL_EXPONENT);
}

int Lure::getStepInterval(float speedParam, float sampleRate) {
	// Clamp knob position safely between 0.0 and 1.0
	speedParam = clamp(speedParam, 0.f, 1.f);

	// Logarithmic mapping for perceptual linearity at fast end
	constexpr float SPEED_INTERVAL_MIN_MS = 1.0f;
	constexpr float SPEED_INTERVAL_MAX_MS = 1000.0f;

	float minLog = log10(SPEED_INTERVAL_MIN_MS);
	float maxLog = log10(SPEED_INTERVAL_MAX_MS);
	float logRange = maxLog - minLog;

	// Invert the curve so higher knob = faster
	float logValue = maxLog - speedParam * logRange;
	float msInterval = powf(10.f, logValue);

	// Convert ms → samples based on current sample rate
	int samples = static_cast<int>(roundf((msInterval / 1000.f) * sampleRate));
	return std::max(1, samples);  // Ensure we never return 0
}


float Lure::calculateForce(float bias, float lower, float upper, float pullParam) {
	float relative = 0.f;
	if (brownianValue < bias) {
		float denom = bias - lower;
		relative = (denom > 0.f) ? (bias - brownianValue) / denom : 0.f;
	} else {
		float denom = upper - bias;
		relative = (denom > 0.f) ? (brownianValue - bias) / denom : 0.f;
	}

	relative = clamp(relative, 0.f, 1.f);
	float gravity = pullParam * powf(relative, GRAVITY_EXPONENT);
	float F_center = (brownianValue < bias) ? +gravity : -gravity;

	// Edge repulsion
	float distFromMin = brownianValue - lower + EPSILON;
	float distFromMax = upper - brownianValue + EPSILON;
	float F_edge = EDGE_REPEL_FACTOR * (
		1.f / powf(distFromMin, EDGE_EXPONENT) -
		1.f / powf(distFromMax, EDGE_EXPONENT)
	);

	return F_center + F_edge;
}


void Lure::process(const ProcessArgs& args) {
	// Get fully modulated and clamped range
	float min = getModulatedMin();
	float max = getModulatedMax();

	float lower = fmin(min, max);
	float upper = fmax(min, max);
	float rangeSafe = fmaxf(upper - lower, EPSILON);

	// Compute speed modulation (handled inline since it's only used here)
	float speedParam = params[SPEED_PARAM].getValue();

	if (inputs[SPEED_INPUT].isConnected()) {
		float cv = inputs[SPEED_INPUT].getVoltage();     // Expecting 0–10V
		float atten = params[SPEED_ATTENUVERTER].getValue();  // -1 to +1
		speedParam += (cv / 10.f) * atten;
	}
	
	speedParam = clamp(speedParam, 0.f, 1.f);

	// Only update periodically, based on current interval
	if (++stepCounter >= currentStepInterval) {
		stepCounter = 0;

		// Recompute interval at step edge
		currentStepInterval = getStepInterval(speedParam, args.sampleRate);

		// Get bias and pull
		float bias = getBias(lower, upper);
		float pullParam = getPullStrength();

		// Calculate total force toward center and edge repel
		float F_net = calculateForce(bias, lower, upper, pullParam);

		// Direction probability
		float directionProb = 0.5f + 0.5f * clamp(F_net, -1.f, 1.f);
		float direction = (random::uniform() < directionProb) ? 1.f : -1.f;

		// Update value
		brownianValue += direction * STEP_SIZE;
		brownianValue = clamp(brownianValue, lower, upper);
	}

	// Output voltage
	outputs[CV_OUTPUT].setVoltage(brownianValue);
}
