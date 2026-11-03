#pragma once
#include <JuceHeader.h>
#include "sa-processor.h"
#include "sb-plugin.h"
#include "MasterDsp.h"

// Hands a child the host transport shifted forward by `samples`, so a child that renders one slice of an
// oversized host block sees tempo and PPQ for the start of that slice rather than the start of the block.
class SliceTransport final: public juce::AudioPlayHead {
public:
 void set(juce::AudioPlayHead* hostPlayHead,juce::int64 samplesIntoBlock,double rate){source=hostPlayHead;offset=samplesIntoBlock;sampleRate=rate;}
 juce::Optional<PositionInfo> getPosition()const override;
private:
 juce::AudioPlayHead* source=nullptr;juce::int64 offset=0;double sampleRate=48000;
};

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
 // Validates the whole performance (host tree and both engine patches) before changing anything.
 // On failure nothing has been changed and the message says why.
 juce::Result loadPerformance(const void*,int);
 static constexpr int maxPerformanceBytes=8*1024*1024;
 // Message of the most recent failed load, empty if the last load worked (a DAW recall has no other way to report it).
 juce::String lastLoadError()const;
 // By-name access for the editor and tests. The audio thread uses the cached get() below instead.
 float value(const juce::String&)const;void write(const juce::String&,float);
 // Host parameters read on the audio thread. Everything up to Air is the ChatGPT baseline set; the rest was added in
 // Phase 2 and only ever appended (no existing ID, range, default or choice index changed).
 enum Id{Routing,Split,ALow,AHigh,AChannel,AGain,AMute,BLow,BHigh,BChannel,BGain,BMute,Master,Glue,Comp,Verb,Eq,Threshold,Ratio,Attack,Release,Room,Damp,Wet,Width,Low,Air,
  APan,BPan,ASolo,BSolo,Swap,TapTarget,CompMode,FetInput,FetOutput,FetAttack,FetRelease,FetRatio,FetMix,FetSidechain,PreDelay,LowCut,IdCount};
 enum CompressorMode{GlueCompressor=0,Fet76=1};
 float get(Id id)const{return cached[size_t(id)]->load(std::memory_order_relaxed);}
 SynthAProcessor a;PrismProcessor b;
 juce::AudioProcessorValueTreeState state;juce::MidiKeyboardState keyboard;
 std::atomic<bool> panic{false};std::atomic<int> panel{0};
 std::array<std::atomic<float>,3> peaks{};
 // Stereo level meters (peak, linear): A left/right, B left/right, output left/right. Written by the audio thread.
 std::array<std::atomic<float>,6> levels{};
 // Largest compressor gain reduction in the last block, dB (>= 0), for the gain-reduction meter.
 std::atomic<float> gainReduction{0};
 // Tap tempo is a UI gesture: each call adds a tap. With two or more taps the average tempo is written to the engine(s)
 // chosen by the "tap.target" parameter and their "follow host" switch is turned off. Returns the tempo or 0 for the first tap.
 float tapTempo();
 float lastTapBpm()const{return tapBpm.load();}
 std::atomic<float> bend{-2},wheel{-1};
 // Ownership is retained until key-up, even when the user changes the split while playing.
 void routeMidi(const juce::MidiBuffer&,juce::MidiBuffer&,juce::MidiBuffer&);
 // Largest block rendered in one piece. Longer host blocks are rendered in slices of at most this size.
 int sliceCapacity()const{return bufferA.getNumSamples();}
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 struct Peaks{float a=0,b=0,master=0,level[6]{};float reduction=0;};
 float panGain(float pan,int side)const;float stripGain(int strip)const;
 void renderSlice(juce::AudioBuffer<float>&,int start,int length,const juce::MidiBuffer&,Peaks&);
 std::array<std::atomic<float>*,IdCount> cached{};
 juce::AudioBuffer<float> bufferA,bufferB;juce::MidiBuffer midiA,midiB,sliceMidi;
 SliceTransport transport;
 std::array<std::array<unsigned char,128>,16> owners{};
 juce::SmoothedValue<float> gainA,gainB,master,width,fxBlend,verbWet;
 juce::SmoothedValue<float> panLeftA,panRightA,panLeftB,panRightB;
 juce::dsp::Compressor<float> compressor;juce::Reverb reverb;
 masterdsp::FetCompressor fet;masterdsp::ReverbInputStage reverbInput;
 // Per-slice scratch for the master section (allocated up front, never on the audio thread).
 std::vector<float> dryL,dryR,wetL,wetR;
 std::array<double,8> tapTimes{};int tapCount=0;std::atomic<float> tapBpm{0};
 std::array<std::array<float,4>,2> alignB{};int alignHead=0;
 float toneLow[2]{},toneHigh[2]{};double sampleRate=48000;
 // Recall publication is exclusive with a complete host audio block.
 juce::CriticalSection publicationLock;
 std::atomic<bool> publicationPending{false};
 mutable juce::CriticalSection recallLock;
 mutable juce::CriticalSection loadErrorLock;juce::String loadError;
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
