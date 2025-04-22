# Terroir Modules

Welcome to **Terroir**, a growing collection of focused, thoughtfully designed Eurorack modules for VCV Rack, developed by **Peter Santerre (ShockValue)**.

---

## 🪵 Design Philosophy

The name "Terroir" reflects a focus on craftsmanship, intentional design, and organic, purposeful function. These modules embody a "less, but better" philosophy, inspired by **Dieter Rams** and principles of clarity and simplicity.

### Key Tenets:
- **Do One Thing Well**: Each module focuses on a specific task, encouraging modular patching rather than multifunctional complexity.
- **Hardware-Friendly**: Designs avoid software-only conventions (like context menus) with the long-term vision of potential hardware translation. Behavior should feel deterministic and grounded.
- **Intentionality**: Features are added purposefully, avoiding "cargo cult" additions simply because other modules have them.
- **Clarity & Simplicity**: Interfaces are designed to be clean, legible, and minimal, using only necessary controls.
- **Aesthetic**: A clean, minimal look featuring a muted "Pebble" faceplate, custom **Magpie125** (main) and **Song60** (trim/setting) knobs, and **IBM Plex Sans / Exo 2** typography.

---

## 🧭 Current Module(s)

### **Lure**
A bias-centered, force-field-driven Brownian walk.

**Lure** generates smooth, drifting voltages within a user-defined range. Its output is shaped by:
- **Min / Max**: Define output bounds.
- **Bias**: Sets the center point the voltage is attracted to.
- **Pull**: Controls gravitational strength toward the bias.
- **Speed**: Adjusts how quickly the output drifts.

All parameters can be CV-modulated, including through attenuverters.

#### Use Lure for:
- Random modulation with character.
- Slowly evolving control signals.
- Quantized melody walks.
- Generative systems with direction.

---

### **Thrum**
A pulsing CV/audio generator inspired by mechanical heartbeats or rotating beacons.

**Thrum** creates rhythmic envelopes or acts as a drone source using internal samples. It can be triggered externally or run free, offering versatile modulation and sound generation capabilities.

#### Features:
- **Envelope CV Output (0-10V)**: Available at the `ENV` output.
- **Audio Output**: Plays internal looping drone samples (selectable) gated by the envelope or processes external audio from the `AUDIO IN` input.
- **DURATION Control**: (0.05s - 3.0s, non-linear response) sets the overall cycle time.
- **DUTY CYCLE Control**: (0% - 100%) shapes the active portion of the envelope.
- **BIAS Control**: (0% - 100%) adjusts the envelope peak position.
- **SAMPLE Select Knob**: (stepped) chooses the internal drone sound.

#### Inputs:
- **CLOCK IN**: Input for external triggers/gates to reset the envelope cycle. Runs free if disconnected.
- **AUDIO IN**: Input for external audio processing.
- **CV Inputs**: Bipolar attenuverters (-1 to +1) for Duration, Duty Cycle, and Bias.

---

## 🧪 More Modules Coming Soon...
This project is evolving. Check back for new additions to the Terroir lineup — or open an issue if you'd like to contribute ideas or feedback.

---

## 🏗️ Building

- Built using the **VCV Rack 2 SDK**.
- Requires a standard **C++11 compiler toolchain** (e.g., GCC via MSYS2 on Windows).
- **External Dependency**: `dr_wav.h` (single-header library) for sample loading.
    - Place in `src/` or a configured include path (e.g., `Libraries/include`).
    - Ensure `#define DR_WAV_IMPLEMENTATION` is present in `plugin.cpp` before including the header.

---

## 📜 License

This plugin is licensed under the **GNU General Public License v3.0**.

You’re welcome to use, modify, and redistribute it, but you must:
- Attribute the original author (**ShockValue**).
- Keep derivative works under GPL-compatible licensing.

See `license.txt` for full terms.