#pragma once
#include "rack.hpp"

using namespace rack;

struct Song60 : app::SvgKnob {
    Song60() {
        // Load the single-layer SVG (indicator + knob face combined)
        auto knobSvg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Song60.svg"));
        setSvg(knobSvg);

        // Set the widget size to match the SVG (assumed to be 6mm x 6mm)
        if (!children.empty()) {
            auto knobWidget = dynamic_cast<SvgWidget*>(children.front());
            if (knobWidget) {
                box.size = knobWidget->box.size;
            }
        }

        // Standard VCV sweep: -135° to +135° (270° total)
        minAngle = -M_PI * 0.75f;
        maxAngle =  M_PI * 0.75f;

        // Optional: hide VCV shadow for clean attenuverter visuals
        if (shadow) {
            shadow->hide();
        }
    }
};
