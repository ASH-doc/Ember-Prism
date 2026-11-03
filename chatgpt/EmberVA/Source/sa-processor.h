// sa-processor.h — JUCE VST3/standalone wrapper, automation, JSON and MIDI learn.
#pragma once
#include <JuceHeader.h>
#include "sa-engine.h"
class SynthAProcessor final : public juce::AudioProcessor, private juce::AsyncUpdater {
 public:
    SynthAProcessor(); ~SynthAProcessor() override;
    void prepareToPlay(double,int) override;void releaseResources() override{}
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&)const override;
    juce::AudioProcessorEditor* createEditor()override;bool hasEditor()const override{return true;}
    const juce::String getName()const override{return "Ember VA";}
    bool acceptsMidi()const override{return true;}bool producesMidi()const override{return false;}
    double getTailLengthSeconds()const override{return 20;}
    int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}
    void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return {};}
    void changeProgramName(int,const juce::String&)override{}
    void getStateInformation(juce::MemoryBlock&)override;void setStateInformation(const void*,int)override;
    float getParam(const juce::String&)const;void setParam(const juce::String&,float);
    juce::var getPatch()const;juce::Result setPatch(const juce::var&);
    juce::Result loadPatchFile(const juce::File&);juce::Result savePatchFile(const juce::File&);
    int presetCount()const{return bank.size();}juce::var preset(int n)const{return bank[n];}
    void loadPreset(int);void mutate();void compare();void learnChord();
    juce::String patchName()const;juce::String macroName(int)const;void renameMacro(int,const juce::String&);
    juce::UndoManager undo;juce::AudioProcessorValueTreeState state;
    SynthA::Engine engine;juce::MidiKeyboardState keyboard;
    std::atomic<bool> panicRequested{false};std::atomic<int> voicesPlaying{0},learnParameter{-1},lastLearnCC{-1};
    std::atomic<float> cpu{0};std::array<std::atomic<float>,128> heldNotes{};
    std::atomic<float> uiPitch{-2},uiMod{-1};
    std::array<std::atomic<int>,128> ccMap{};
 private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    std::array<std::atomic<float>*,SynthA::maxParams> raw{};
    std::array<std::atomic<float>,SynthA::maxParams> pendingCC{};
    std::array<SynthA::MidiEvent,2048> midiEvents{};
    juce::Array<juce::var> bank;mutable juce::CriticalSection metaLock;
    juce::String name="Init - Warm Saw",category="Init";juce::StringArray macroNames{"Open","Width","Echo","Bite"};
    juce::var comparePatch;void handleAsyncUpdate()override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthAProcessor)
};
