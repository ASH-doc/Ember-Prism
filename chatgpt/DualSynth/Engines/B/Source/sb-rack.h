// sb-rack.h — Ten private, reorderable stereo effects and musical macro processing.
#pragma once
#include "sb-filters.h"
namespace SynthB {
class EffectRack {
 public:EffectRack();void prepare(float sampleRate);void clear();void update(const State&);Stereo process(Stereo input,double beat,float bpm);
 private:
 const Layout& layout;float sr=96000;std::array<std::array<float,fxSize>,10> parameters{};
 std::array<int,10> order{};std::array<float,10> wet{};float spaceAmount=0,euphoriaAmount=0;
 std::array<Circular,2> chorus,flanger,delay,shifter,widen;
 std::array<Circular,12> ambience;std::array<float,12> damp{};
 std::array<std::array<float,8>,2> phaseState{};std::array<float,2> flFeedback{},phaseFeedback{},deFeedback{},bass{},air{},distTone{},compress{},widthLP{};
 std::array<DigitalFilter,2> filter;float chorusPhase=0,flangerPhase=0,phaserPhase=0,pitchPhase=0,delayTime=.375f;
 Stereo module(int type,Stereo in,double beat,float bpm);
};
}
