// sb-motion.h — Independent curved AHDSRs, drawable LFOs and tempo event generation.
#pragma once
#include "sb-schema.h"
namespace SynthB {
struct EnvelopeState {
 int stage=0;float value=0,position=0,start=0;
 void strike(){stage=1;position=0;start=value;}void lift(){if(stage!=0&&stage!=5){stage=5;position=0;start=value;}}
 float advance(const float* controls,float sampleRate);
};
struct LFOState {
 float phase=0,lastRandom=0,sampleHold=0,seconds=0;
 float advance(const float* controls,float sampleRate,float tempo,Entropy& random);
};
struct PulseNote {int note=60,channel=1,origin=0;float velocity=.8f;bool start=true;};
class Performance {
 public:Performance();void clear();void settings(const State&);void key(int note,int channel,float velocity,bool down);void pedal(int channel,bool down);
 int advance(double beat,std::array<PulseNote,512>& result);int playhead()const{return shownStep;}bool running()const;
 private:
 struct Key {int note=0,channel=1;float velocity=0;bool down=false;uint64_t order=0;};
 struct Lane {double next=-1,off=-1;int counter=0;bool enabled=false;std::array<PulseNote,128> sounding{};int count=0;};
 std::array<Key,128> held{};int count=0;uint64_t stamp=0;std::array<bool,16> pedals{};std::array<Lane,2> lanes{};
 State values{};const Layout& layout;Entropy random;double lastBeat=-1;int shownStep=0;bool latched=false;
};
}
