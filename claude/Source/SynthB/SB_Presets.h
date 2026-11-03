// PRISM (Synth B) presets: each lists only the parameters that differ from the init patch (+ optional drawn LFO shapes).
#pragma once
#include "SB_Params.h"
#include <vector>
#include <string>
#include <utility>

namespace sb
{
struct Preset
{
    std::string name, category;
    std::vector<std::pair<int, float>> values;                         // (parameter index, real value)
    std::vector<std::pair<int, std::vector<float>>> shapes;            // (lfo 0..3, 16 points -1..1) for LFOs set to "Drawn"
};
const std::vector<Preset>& presets();                                  // [0] is always "Init"
std::vector<float> presetValues (int index);                           // full parameter set (defaults + overrides)
} // namespace sb
