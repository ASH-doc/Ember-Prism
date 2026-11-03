// EMBER (Synth A) presets: each one lists only the parameters that differ from the init patch.
#pragma once
#include "SA_Params.h"
#include <vector>
#include <string>
#include <utility>

namespace sa
{
struct Preset
{
    std::string name, category;
    std::vector<std::pair<int, float>> values;    // (parameter index, real value)
};
const std::vector<Preset>& presets();             // [0] is always "Init"
std::vector<float> presetValues (int index);      // full parameter set (defaults + overrides)
} // namespace sa
