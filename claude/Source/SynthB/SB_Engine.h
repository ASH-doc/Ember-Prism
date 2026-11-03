// PRISM (Synth B) - crystal-clear, modern wavetable engine.
#pragma once
#include "../Host/SynthContract.h"
#include "SB_Params.h"
#include "SB_Dsp.h"
#include "SB_Fx.h"
#include "SB_Wavetables.h"
#include <atomic>

namespace sb
{
constexpr int kVoices = 16;
constexpr int kCR = 16;
constexpr int kMaxUni = 16;

struct Voice
{
    bool active = false, gate = false, sustained = false, stealing = false;
    int key = 0, pKey = 0;
    float note = 60.0f, target = 60.0f, pNote = 60.0f, vel = 0.0f, pVel = 0.0f, rnd = 0.0f, stealGain = 1.0f, lastAmp = 0.0f;
    uint32_t order = 0;
    Adsr env[3];
    float ph1[kMaxUni] = {}, ph2[kMaxUni] = {}, y1prev[kMaxUni] = {}, y2prev[kMaxUni] = {};
    float phSub = 0.0f, airLp = 0.0f;
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

    // drawable LFO shapes (4 LFOs x 16 points, -1..1) - written by the UI, read by the audio thread
    void  setCustomShape (int lfo, int point, float v) { custom[(size_t) (lfo * 16 + point)].store (v); }
    float getCustomShape (int lfo, int point) const { return custom[(size_t) (lfo * 16 + point)].load(); }

    // read by the UI (message thread)
    void  readScope (float* dest, int count) const;
    int   activeVoices() const { int c = 0; for (auto& v : voices) c += v.active ? 1 : 0; return c; }
    int   seqStep() const { return seqStepIdx.load(); }
    int   arpStep() const { return arpStepIdx.load(); }
    float modShownDest (int d) const { return modShown[(size_t) d].load(); }
    float srcShownValue (int s) const { return srcShown[(size_t) s].load(); }
    float lfoValue (int i) const { return srcShown[(size_t) (S_LFO1 + i)].load(); }

private:
    void chunk (float* L, float* R, int n);
    void renderVoice (Voice& v, float* oL, float* oR, int n);
    void startVoice (Voice& v, int key, float note, float vel, bool retrig);
    void voiceOn (int key, float note, float vel);
    void trigger (int key, float vel);
    void releaseKey (int key);
    void arpUpdate();
    void seqUpdate();
    void rebuildArp();
    int  ch (int p) const { return (int) (pr[p] + 0.5f); }

    float pr[COUNT] = {}, sm[COUNT] = {};
    bool isFloat[COUNT] = {};
    float sr = 48000.0f, bpm = 138.0f;
    double beatPos = 0.0;
    float bend = 0.0f, modWheel = 0.0f, at = 0.0f;
    bool sustain = false;
    uint32_t orderCounter = 0;
    Voice voices[kVoices];
    Lfo lfos[4]; float lfoAcc[4] = {}, lfoV[4] = {};
    RackSlot rack[8];
    Rng rng;

    std::vector<std::pair<int, float>> monoStack;
    std::vector<int> heldKeys;
    std::vector<int> arpHeld, arpSeq; bool arpDirty = false; long arpLast = -1; int arpIdx = 0, arpKeyOn = -1; std::vector<int> arpChordOn;
    long seqLast = -1; int seqKeyOn = -1;
    float bufL[kCR], bufR[kCR];

    static constexpr int kScope = 2048;
    std::array<std::atomic<float>, kScope> scope;
    std::array<std::atomic<float>, 64> custom;
    std::array<std::atomic<float>, D_COUNT> modShown;
    std::array<std::atomic<float>, S_COUNT> srcShown;
    std::atomic<int> scopePos { 0 }, seqStepIdx { 0 }, arpStepIdx { 0 };
};
} // namespace sb
