#pragma once
#include <JuceHeader.h>
#include "sa-processor.h"
#include "sb-plugin.h"
// A shell owns two unchanged instruments. Host controls act only on MIDI and their outputs.
class DualProcessor final: public juce::AudioProcessor {
public:
 DualProcessor();~DualProcessor()override;
 void prepareToPlay(double,int)override;void releaseResources()override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&)override;
 bool isBusesLayoutSupported(const BusesLayout&)const override;
 juce::AudioProcessorEditor* createEditor()override;bool hasEditor()const override{return true;}
 const juce::String getName()const override{return "Dual Synth";}
 bool acceptsMidi()const override{return true;}bool producesMidi()const override{return false;}
 double getTailLengthSeconds()const override{return 30;}
 int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}
 void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return {};}
 void changeProgramName(int,const juce::String&)override{}
 void getStateInformation(juce::MemoryBlock&)override;void setStateInformation(const void*,int)override;
 float value(const juce::String&)const;void write(const juce::String&,float);
 SynthAProcessor a;PrismProcessor b;
 juce::AudioProcessorValueTreeState state;juce::MidiKeyboardState keyboard;
 std::atomic<bool> panic{false};std::atomic<int> panel{0};
 std::array<std::atomic<float>,3> peaks{};
 std::atomic<float> bend{-2},wheel{-1};
 // Ownership is retained until key-up, even when the user changes the split while playing.
 void routeMidi(const juce::MidiBuffer&,juce::MidiBuffer&,juce::MidiBuffer&);
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 juce::AudioBuffer<float> bufferA,bufferB;juce::MidiBuffer midiA,midiB;
 std::array<std::array<unsigned char,128>,16> owners{};
 juce::SmoothedValue<float> gainA,gainB,master,width,fxBlend,verbWet;
 juce::dsp::Compressor<float> compressor;juce::Reverb reverb;
 std::array<std::array<float,4>,2> alignB{};int alignHead=0;
 float toneLow[2]{},toneHigh[2]{};double sampleRate=48000;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DualProcessor)
};
class DualEditor final:public juce::AudioProcessorEditor,private juce::Timer {
public:explicit DualEditor(DualProcessor&);~DualEditor()override;void resized()override;void paint(juce::Graphics&)override;void select(int);
private:
 DualProcessor& p;juce::LookAndFeel_V4 look;juce::Component surface,clip;
 std::unique_ptr<juce::AudioProcessorEditor> editorA,editorB;
 juce::MidiKeyboardComponent keys;
 juce::TextButton showA{"A  /  EMBER VA"},showB{"B  /  PRISM FLUX"},panicButton{"PANIC"},save{"Save performance"},load{"Load"};
 std::vector<std::unique_ptr<juce::Component>> controls;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliders;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> toggles;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> choices;
 juce::Slider pitch,mod;juce::Label meter,status;
 std::unique_ptr<juce::FileChooser> chooser;
 void dial(juce::String,int,int,int,int);void toggle(juce::String,juce::String,int,int,int);void menu(juce::String,juce::StringArray,int,int,int);
 void file(bool);void timerCallback()override;
};
