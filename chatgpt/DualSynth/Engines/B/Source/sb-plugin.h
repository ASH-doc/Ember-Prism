// sb-plugin.h — Native host integration, self-contained patch assets and UI-safe commands.
#pragma once
#include <JuceHeader.h>
#include "sb-engine.h"
class PrismProcessor final:public juce::AudioProcessor,private juce::AsyncUpdater {
 public:
 PrismProcessor();~PrismProcessor()override;
 void prepareToPlay(double,int)override;void releaseResources()override{};void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&)override;
 bool isBusesLayoutSupported(const BusesLayout&)const override;
 const juce::String getName()const override{return "Prism Flux";}bool acceptsMidi()const override{return true;}bool producesMidi()const override{return false;}
 bool hasEditor()const override{return true;}juce::AudioProcessorEditor* createEditor()override;double getTailLengthSeconds()const override{return 30;}
 int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{};
 const juce::String getProgramName(int)override{return {};};void changeProgramName(int,const juce::String&)override{}
 void getStateInformation(juce::MemoryBlock&)override;void setStateInformation(const void*,int)override;
 float read(int)const;float read(const juce::String&)const;void write(int,float);void write(const juce::String&,float);
 juce::var patch()const;juce::Result restore(const juce::var&,bool commit=true, std::function<void()> beforePublish = {});juce::Result open(const juce::File&);juce::Result save(const juce::File&);
 int factoryCount()const{return factory.size();}juce::var factoryPatch(int i)const{return factory[i];}void selectFactory(int);
 juce::Result importWave(const juce::File&,int oscillator,int cycleLength=2048);const SynthB::WaveSet* importedTable()const;
 juce::String title()const;juce::String macroTitle(int)const;void nameMacro(int,const juce::String&);
 void randomize();void snapshot();void swapScene();void route(int source,int destination);void reorder(int from,int to);
 juce::UndoManager undo;juce::AudioProcessorValueTreeState parameters;SynthB::Instrument engine;juce::MidiKeyboardState keyboard;
 std::atomic<bool> silence{false};std::atomic<float> wheelRequest{-1},bendRequest{-2},cpuLoad{0};std::atomic<int> voices{0},learnTarget{-1};
 std::array<std::atomic<int>,128> ccAssignments{};
 private:
 struct Imported {std::vector<float> raw;int frames=1;juce::String name;std::unique_ptr<SynthB::WaveSet> table;};
 std::vector<std::unique_ptr<Imported>> waveAssets;std::atomic<const Imported*> currentWave{nullptr};
 std::array<std::atomic<float>*,SynthB::capacity> atomics{};std::array<std::atomic<float>,SynthB::capacity> ccPending{};
 std::array<SynthB::Event,2048> events{};juce::Array<juce::var> factory;
 mutable juce::CriticalSection metadata;juce::String patchTitle="Init / Crystal",category="Init";juce::StringArray tags{"crystal","modern"};
 juce::StringArray macroTitles{"Scan","Edge","Lift","Motion","Density","Width","Echo","Bloom"};juce::var scene;
 static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();void handleAsyncUpdate()override;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrismProcessor)
};
