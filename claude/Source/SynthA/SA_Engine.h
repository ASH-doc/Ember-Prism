// EMBER (Synth A) - warm, gritty subtractive virtual-analog engine.
#pragma once
#include "../Host/SynthContract.h"
#include "SA_Params.h"
#include "SA_Dsp.h"
#include "SA_Fx.h"
#include <atomic>
#include <array>

namespace sa
{
constexpr int kVoices = 16;
constexpr int kCR = 16;
constexpr int kMaxUni = 8;

struct Voice
{
    bool active = false, gate = false, sustained = false, stealing = false;
    int key = 0, pKey = 0;
    float note = 60.0f, target = 60.0f, pNote = 60.0f, vel = 0.0f, pVel = 0.0f, rnd = 0.0f, stealGain = 1.0f, lastAmp = 0.0f;
    uint32_t order = 0;
    Adsr env[3];
    float ph1[kMaxUni] = {}, ph2[kMaxUni] = {}, tri1[kMaxUni] = {}, tri2[kMaxUni] = {}, y2prev[kMaxUni] = {};
    float ph3 = 0, tri3 = 0, phSub = 0, pink[3] = {};
    float drift[3] = {}, tolP[3] = {}, tolCut = 0.0f, shapeLp = 0.0f;
    FiltState f1[2], f2[2];
    Rng rng;
};

class Engine : public duo::SynthContract
{
public:
    Engine();
    void prepare (double sampleRate, int maxBlock) override;
    void process (float* left, float* right, int numSamples, const duo::TransportInfo& t) override;
    void noteOn (int note, float velocity, int channel) override;
    void noteOff (int note, int channel) override;
    void pitchBend (float v) override { bend = v; }
    void controlChange (int cc, int value) override;
    void aftertouch (int value) override { at = (float) value / 127.0f; }
    void allNotesOff() override;
    void panic() override;
    int  numParams() const override { return COUNT; }
    void setParamSnapshot (const float* values) override { for (int i = 0; i < COUNT; ++i) pr[i] = values[i]; }

    // chord stab memory
    void learnChord();                                   // stores intervals of currently held keys
    std::vector<int> getChord() const { return chord; }
    void setChord (const std::vector<int>& c) { chord = c; }

    // read by the UI (message thread)
    void readScope (float* dest, int count) const;
    int  activeVoices() const { int c = 0; for (auto& v : voices) c += v.active ? 1 : 0; return c; }
    int  gateStep() const { return gateStepIdx.load(); }
    int  arpStep() const { return arpStepIdx.load(); }
    float lfoValue (int i) const { return lfoShown[i].load(); }

private:
    void chunk (float* L, float* R, int n);
    void renderVoice (Voice& v, float* oL, float* oR, int n);
    void startVoice (Voice& v, int key, float note, float vel, bool retrig);
    void voiceOn (int key, float note, float vel);
    void trigger (int key, float vel);
    void releaseKey (int key);
    void arpUpdate (int n);
    void rebuildArp();
    int  ch (int p) const { return (int) (pr[p] + 0.5f); }

    float pr[COUNT] = {}, sm[COUNT] = {};
    bool isFloat[COUNT] = {};
    float sr = 48000.0f, bpm = 138.0f;
    double beatPos = 0.0;
    float bend = 0.0f, modWheel = 0.0f, at = 0.0f, vibPhase = 0.0f;
    bool sustain = false;
    uint32_t orderCounter = 0;
    Voice voices[kVoices];
    Lfo lfos[3]; float lfoAcc[3] = {}, lfoV[3] = {};
    FxChain fx;
    Rng rng;

    std::vector<std::pair<int, float>> monoStack;      // key, velocity
    std::vector<int> heldKeys;                         // physically down (order pressed)
    std::vector<int> chord;
    // arp
    std::vector<int> arpHeld, arpSeq; bool arpDirty = false; long arpLast = -1; int arpIdx = 0, arpDir = 1, arpKeyOn = -1; std::vector<int> arpChordOn;
    // gate
    float gateGain = 1.0f;
    std::array<float, 2 * kCR> scratch {};
    float bufL[kCR], bufR[kCR];
    static constexpr int kScope = 2048;
    std::array<std::atomic<float>, kScope> scope;
    std::atomic<int> scopePos { 0 }, gateStepIdx { 0 }, arpStepIdx { 0 };
    std::atomic<float> lfoShown[3];
};
} // namespace sa
