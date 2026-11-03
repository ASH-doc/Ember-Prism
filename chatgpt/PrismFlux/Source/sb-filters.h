// sb-filters.h — Independent clean digital filtering and reusable delay primitives.
#pragma once
#include "sb-schema.h"
namespace SynthB {
class Circular {
 public:void allocate(int count){buffer.assign(size_t(count),0);cursor=0;}void clear(){std::fill(buffer.begin(),buffer.end(),0);cursor=0;}
 float tap(float distance)const{distance=bounded(distance,1.f,float(buffer.size()-2));float at=float(cursor)-distance;if(at<0)at+=float(buffer.size());int i=int(at);float frac=at-float(i);return buffer[size_t(i)]+(buffer[size_t((i+1)%int(buffer.size()))]-buffer[size_t(i)])*frac;}
 void write(float v){buffer[size_t(cursor)]=v;if(++cursor==int(buffer.size()))cursor=0;}
 private:std::vector<float> buffer;int cursor=0;
};
struct IntegratorPair {
 float first=0,second=0;
 std::array<float,3> run(float input,float tangent,float damping){const float a=1/(1+tangent*(tangent+damping));float band=(first+tangent*(input-second))*a;float low=second+tangent*band;first=2*band-first;second=2*low-second;return {low,input-damping*band-low,band};}
};
class DigitalFilter {
 public:void setup(float sampleRate);void clear();void coefficients(float hz,float resonance,float vowel);
 float process(float input,int kind,float drive);
 private:float sr=96000,g=.1f,target=.1f,q=1,combLength=120,combFeedback=0,vowelPosition=.3f,shelf=0;
 IntegratorPair first,second;std::array<IntegratorPair,3> formants;std::array<float,3> formantG{};Circular comb;
};
struct DCGuard {float previous=0,state=0;float step(float v){float out=v-previous+.9995f*state;previous=v;state=out;return out;}};
class Decimator {
 public:Decimator();void clear(){history.fill(0);write=0;}void feed(float x){history[size_t(write)]=x;write=(write+1)%47;}float output()const;
 private:std::array<float,47> weights{},history{};int write=0;
};
}
