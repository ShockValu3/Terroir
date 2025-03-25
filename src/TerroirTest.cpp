#define _USE_MATH_DEFINES
#include "plugin.hpp"

using namespace rack;

extern Plugin* pluginInstance;

//-----------------------------------------------------
// Module logic
//-----------------------------------------------------
struct TerroirTest : rack::engine::Module {
    TerroirTest() {
        config(0, 0, 1);  // No params, no inputs, 1 output
        configOutput(0, "Output");
    }

    void process(const ProcessArgs& args) override {
        outputs[0].setVoltage(5.0f);  // Constant 5V output
    }
};

//-----------------------------------------------------
// Panel widget (UI rendering and layout)
//-----------------------------------------------------
struct TerroirTestWidget : rack::app::ModuleWidget {
    TerroirTestWidget(TerroirTest* module) {
        setModule(module);

        // Load SVG panel
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TerroirTest.svg")));

        // Output port
        addOutput(createOutputCentered<componentlibrary::PJ301MPort>(
            window::mm2px(math::Vec(15, 85)), module, 0));
    }
};

//-----------------------------------------------------
// Model factory
//-----------------------------------------------------
Model* createTerroirModel() {
    return createModel<TerroirTest, TerroirTestWidget>("TerroirTest");
}
