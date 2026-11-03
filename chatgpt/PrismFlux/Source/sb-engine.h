// sb-engine.h — Independent stereo wavetable instrument; no JUCE/UI dependency.
#pragma once
#include "sb-tables.h"
#include "sb-motion.h"
#include "sb-rack.h"
#include <atomic>
namespace SynthB {
struct Event {enum Kind{Down,Up,Bend,Controller,Pressure,Silence} kind=Down;int offset=0,key=60,channel=1;float value=0;};
class Instrument {
 public:
 Instrument();void prepare(double sampleRate);void parameters(const State&);void reset();void releaseAll();void message(const Event&);
 void render(float* left,float* right,int frames,const Event* events,int count,double hostBpm=0,double hostBeat=-1,const WaveSet* custom=nullptr);
 int active()const;std::array<std::atomic<float>,capacity> displayed{};std::array<std::atomic<float>,24> sources{};
 std::array<std::atomic<float>,2048> spectrum{};std::atomic<unsigned> spectrumIndex{0};std::array<std::atomic<bool>,16> voiceLights{};
 std::atomic<int> step{0};std::atomic<float> meter{0};
 private:
 struct Voice {
  bool running=false,pressed=false,pedalled=false;int key=60,channel=1,origin=0;uint64_t serial=0;
  float velocity=0,note=60,target=60,subPhase=0,fadeIn=0,random=0,tailL=0,tailR=0,previousL=0,previousR=0,noiseLow=0;
  std::array<EnvelopeState,3> envelopes{};std::array<LFOState,4> lfos{};std::array<float,24> source{};
  std::array<std::array<float,16>,2> phase{},increment{},stereoL{},stereoR{},position{},mip{};
  std::array<int,2> lanes{};std::array<std::array<DigitalFilter,2>,2> filters;
  std::array<float,2> filterWet{};DCGuard dc[2];State local{};Entropy rng;
 };
 const Schema& schema;const Layout& layout;const Tables& tables;State desired{},slewed{},global{};
 std::array<Voice,16> pool;EffectRack rack;Performance performance;std::array<PulseNote,512> generated{};
 std::array<std::array<float,128>,16> keys{};std::array<std::array<uint64_t,128>,16> priorities{};
 std::array<float,16> wheels{},bend{},bendSmooth{},pressure{};std::array<bool,16> pedals{};
 float sampleRate=48000,internalRate=96000;double beat=0;uint64_t frameClock=0,serial=0;int modulationClock=0,lastMode=0;bool lastPerformance=false;
 Decimator leftDown,rightDown;DCGuard outputDC[2];const WaveSet* imported=nullptr;
 void start(int key,int channel,float velocity,int origin);void stop(int key,int channel,int origin);
 void refresh(Voice&,float bpm);void modulate(State&,const Voice&);void control(float bpm);Stereo sample(Voice&,float bpm);
};
}
