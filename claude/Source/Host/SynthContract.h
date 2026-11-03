// The only thing the host knows about a synth. Both engines implement this and nothing else is shared.
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

namespace duo
{
struct TransportInfo
{
    double bpm = 138.0;
    double ppq = 0.0;       // quarter-note position of the start of the block (valid if playing)
    bool   playing = false;
};

class SynthContract
{
public:
    virtual ~SynthContract() = default;

    virtual void prepare (double sampleRate, int maxBlock) = 0;
    // reads the synth's own parameters from the snapshot the processor refreshed, then renders (adds nothing: overwrites)
    virtual void process (float* left, float* right, int numSamples, const TransportInfo& t) = 0;

    virtual void noteOn (int note, float velocity, int channel) = 0;
    virtual void noteOff (int note, int channel) = 0;
    virtual void pitchBend (float v) = 0;                 // -1..1
    virtual void controlChange (int cc, int value) = 0;   // 0..127
    virtual void aftertouch (int value) = 0;              // 0..127
    virtual void allNotesOff() = 0;                       // musical release
    virtual void panic() = 0;                             // instant silence

    virtual int  numParams() const = 0;
    virtual void setParamSnapshot (const float* values) = 0;   // numParams() real-unit values
};
} // namespace duo
