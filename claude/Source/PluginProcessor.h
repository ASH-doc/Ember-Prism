// AETHER DUO - JUCE processor: owns both engines, the host (mixer / routing / master) and every parameter.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include "Host/HostEngine.h"
#include "SynthA/SA_Engine.h"
#include "SynthA/SA_Presets.h"
#include "SynthB/SB_Engine.h"
#include "SynthB/SB_Presets.h"

class DuoProcessor : public juce::AudioProcessor
{
public:
    DuoProcessor();
    ~DuoProcessor() override;

    // ---- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "AETHER DUO"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& dest) override;
    void setStateInformation (const void* data, int size) override;

    // ---- access for the editor / tests
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    sa::Engine& engineA() { return *synA; }
    sb::Engine& engineB() { return *synB; }
    const duo::Meters& meters() const { return host.meters(); }
    const duo::GlueCompressor& compressorForUi() const { return lastCompUi; }

    juce::RangedAudioParameter* paramA (int idx) const { return pA[(size_t) idx]; }
    juce::RangedAudioParameter* paramB (int idx) const { return pB[(size_t) idx]; }
    juce::RangedAudioParameter* hostParam (const juce::String& id) const { return apvts.getParameter (id); }
    float rawHost (const juce::String& id) const { return *apvts.getRawParameterValue (id); }

    void loadPresetA (int index);
    void loadPresetB (int index);
    int  presetIndexA() const { return (int) apvts.state.getProperty ("presetA", 0); }
    int  presetIndexB() const { return (int) apvts.state.getProperty ("presetB", 0); }
    void setCustomShapeB (int lfo, int point, float v);                 // keeps engine + saved state in sync
    void learnChordA();                                                // stores the keys currently held as Ember's chord stab
    juce::String chordTextA() const;

    int  activePanel() const { return (int) apvts.state.getProperty ("panel", 0); }
    void setActivePanel (int p) { apvts.state.setProperty ("panel", p, nullptr); }

    void requestPanic() { panicFlag = true; }
    void tapTempo();

    // session files (JSON): { format:"dualsynth-session", version:1, mixer, routing, master, synthA, synthB }
    juce::String sessionToJson();
    bool sessionFromJson (const juce::String& text, juce::String& error);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void pushExtrasToEngines();
    juce::var patchToVar (int engine);
    void patchFromVar (int engine, const juce::var& v);

    std::unique_ptr<sa::Engine> synA;
    std::unique_ptr<sb::Engine> synB;
    duo::HostEngine host;
    duo::GlueCompressor lastCompUi;

    std::vector<juce::RangedAudioParameter*> pA, pB;
    std::vector<std::atomic<float>*> rawA, rawB;
    std::vector<float> snapA, snapB;
    struct HostRaw
    {
        std::atomic<float> *volA, *volB, *panA, *panB, *muteA, *muteB, *soloA, *soloB, *mode, *split, *swap, *chA, *chB, *master,
            *cOn, *cThr, *cRatio, *cAtt, *cRel, *cKnee, *cMake, *cMix, *rOn, *rSize, *rDecay, *rDamp, *rPre, *rLow, *rWet, *bpm, *sync;
    } hr {};

    std::atomic<bool> panicFlag { false };
    double lastTapTime = 0.0;
    std::vector<double> tapTimes;
    juce::MidiBuffer midiScratch;
    double currentSampleRate = 48000.0;
    juce::AudioBuffer<float> mixBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DuoProcessor)
};
