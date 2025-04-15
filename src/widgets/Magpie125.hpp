#pragma once
#include "rack.hpp"

using namespace rack;

struct Magpie125 : app::SvgKnob {
    SvgWidget* background = nullptr;
    Magpie125() {
        // Set foreground (indicator ring)
        auto knobSvg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Magpie125G.svg"));
        setSvg(knobSvg);

        // Match box size to SVG size
        if (!children.empty()) {
            auto knobWidget = dynamic_cast<SvgWidget*>(children.front());
            if (knobWidget) {
                box.size = knobWidget->box.size;
            }
        }

        // Set background (knob face)
        background = new SvgWidget();
        background->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Magpie125G_bg.svg")));
        background->box.size = box.size;
        background->box.pos = Vec(0, 0);
        addChildBelow(background, children.front());

        // Standard VCV-style 270° sweep (-135° to +135°)
        minAngle = -M_PI * 0.75f;
        maxAngle =  M_PI * 0.75f;
        if (shadow) shadow->hide();
    }

    void step() override {
        SvgKnob::step();

        // Ensure background stays sized and positioned correctly
        if (background) {
            background->box.size = box.size;
            background->box.pos = Vec(0, 0);
        }
    }
};
