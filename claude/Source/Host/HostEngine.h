// AETHER DUO host: MIDI router (layer / split / channel), two mixer strips, master chain.
// It only talks to the synths through SynthContract. It never changes how either synth sounds.
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>
#include "SynthContract.h"
#include "HostDsp.h"

namespace duo
{
enum RouteMode { Layer = 0, Split = 1, Channel = 2 };

struct HostParams
{
    // mixer
    float gainA = 1.0f, gainB = 1.0f, panA = 0.0f, panB = 0.0f;
    bool muteA = false, muteB = false, soloA = false, soloB = false;
    // routing
    int mode = Layer, splitNote = 60, channelA = 1, channelB = 2;   // channel 0 = omni
    bool splitSwap = false;
    // master
    float masterGain = 0.9f;
    GlueCompressor::Params comp;
    MasterReverb::Params reverb;
};

struct Meters
{
    std::atomic<float> aL { 0 }, aR { 0 }, bL { 0 }, bR { 0 }, mL { 0 }, mR { 0 }, gr { 0 }, limGr { 0 };
};

class HostEngine
{
public:
    void prepare (double sampleRate, int maxBlock, SynthContract* synthA, SynthContract* synthB);

    // renders one block: MIDI (already containing keyboard/UI notes) is routed sample-accurately
    void processBlock (float* outL, float* outR, int numSamples, const juce::MidiBuffer& midi, const TransportInfo& transport, const HostParams& p);

    void panic();                                    // instant silence on both synths + forget all held notes
    void allNotesOffBoth();
    const Meters& meters() const { return m; }
    int heldCount (int synth) const;                 // for tests / UI: how many notes this synth currently has down
    int latencySamples() const { return limiter.latency(); }

private:
    void handle (const juce::MidiMessage& msg, const HostParams& p);
    void noteOnTo (int mask, int note, float vel, int ch);
    void render (int start, int len, const TransportInfo& t, const HostParams& p);
    void applyAudibility (const HostParams& p);

    SynthContract* syn[2] = { nullptr, nullptr };
    double sr = 48000.0;
    // which synths received each key (bit 0 = A, bit 1 = B) so note-off always reaches the same synth
    std::array<std::array<uint8_t, 128>, 16> held {};
    int lastMode = -1;
    bool audible[2] = { true, true };
    bool sustainState[16] = {};

    std::vector<float> aL, aR, bL, bR, sumL, sumR;
    float gA = 1, gB = 1, panAs = 0, panBs = 0, mastS = 1, muteGA = 1, muteGB = 1;
    GlueCompressor comp; MasterReverb reverb; SafetyLimiter limiter;
    Meters m;
    float peakA[2] = {}, peakB[2] = {}, peakM[2] = {};
    double blockPpq = 0.0;
};
} // namespace duo
