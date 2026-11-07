#pragma once
// SpireToEmber.h — Translates a decoded Spire 1.5 preset into an EMBER (Synth A) patch and an ImportReport.
// First version (contract v1, checkpoint 1A): the lists and ids come from Spire's own tables and saved fixtures; the unit curves
// (cutoff in Hz, times in seconds, LFO rate ...) are NOT calibrated, so every number that depends on one is reported as
// Approximated, and the caller is expected to correct level, filter cutoff and envelope times against a render of the original.
#include "ImportTypes.h"
#include <juce_core/juce_core.h>

namespace aether::import {

// Knobs the caller may turn after translation to bring the sound closer to a Spire render of the same preset.
struct EmberCalibration {
    double cutoffShiftOctaves = 0;    // added to every filter cutoff (default curve: Hz = 20 * 1000^stored)
    double timeScaleOctaves = 0;      // envelope attack, decay and release are multiplied by 2^this
    double masterVolumeDb = -9;       // EMBER master volume
};

struct EmberTranslation {
    ImportReport report;
    juce::var patch;                  // "dualsynth-patch" JSON for SynthAProcessor::setPatch; void when blocked
};

EmberTranslation translateSpireToEmber(const DecodedPatch& decoded, const EmberCalibration& calibration = {});

} // namespace aether::import
