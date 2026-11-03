// sb-schema.h — Synth B's independent native parameter and signal contract.
#pragma once
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace SynthB {
constexpr int capacity=768, voiceLimit=16, routes=24;
// Append sources so legacy patch indices keep their meaning.
constexpr int pitchBendSource = 24, modulationSourceCount = pitchBendSource + 1;
constexpr double tau=6.283185307179586476925;
template<typename T> T bounded(T x,T lo,T hi){return std::min(hi,std::max(lo,x));}
using State=std::array<float,capacity>;
struct Spec {
 std::string key,title,section,unit;float low=0,high=1,initial=0,power=1;bool integer=false;
 std::vector<std::string> options;
 float unitValue(float x)const{return std::pow(bounded((x-low)/(high-low),0.f,1.f),power);}
 float realValue(float x)const{return low+(high-low)*std::pow(bounded(x,0.f,1.f),1/power);}
 float valid(float x)const{x=std::isfinite(x)?bounded(x,low,high):initial;return integer?std::round(x):x;}
};
enum Osc {table,pos,octave,tune,fine,level,pan,phase,randomPhase,unison,detune,spread,blend,wtSpread,detuneCurve,warp1,amount1,warp2,amount2,fmSource,oscSize};
enum Fil {enabled,type,cutoff,resonance,drive,mix,keyTrack,envAmount,velocity,vowel,filterSize};
enum Env {attack,hold,decay,sustain,release,curve,envSize};
enum Lfo {rate,sync,division,restart,fade,interpolation,point0,lfoSize=point0+16};
enum Fx {fxOn,fxMix,fxA,fxB,fxC,fxMode,fxSync,fxDivision,fxSize};
enum Route {source,destination,depth,bipolar,shape,routeSize};
// Motion lanes (appended after every earlier parameter): three looping bipolar step lanes that offset one destination each.
enum MotionLane {laneDest,laneDivision,laneLength,laneInterp,laneDepth,laneValue0,laneSize=laneValue0+16};
enum Seq {seqOn,seqMode,seqRate,seqSwing,seqLength,seqHold,seqOctaves,seqRestart,stepPitch0,stepGate0=stepPitch0+16,stepVelocity0=stepGate0+16,seqSize=stepVelocity0+16};
struct Layout {
 std::array<int,2> claudeTable{};
 std::array<int,2> osc{},filter{};std::array<int,3> env{};std::array<int,4> lfo{};
 std::array<int,8> macro{};std::array<int,10> fx{},order{};std::array<int,routes> route{};
 int sub,subOct,subWave,noise,noiseColor,routing,filterBalance,mode,glide,bend,velocityCurve,tempo,host,master,pan,arp,seq,euphoria,space;
 std::array<int,6> locks{};
 int motionAmount=-1;std::array<int,3> motion{};
};
class Schema {
 public:Schema();std::vector<Spec> specs;Layout loc;int find(const std::string&)const;State initial()const;
 static const Schema& get();
};
const std::vector<std::string>& sourceNames();
const std::vector<std::string>& tableNames();
const std::vector<std::string>& claudeTableNames();
const std::vector<std::string>& effectNames();
// Destinations a motion lane can steer: index 0 is "Off", the others map to oscillator positions and filter cutoff/resonance.
const std::vector<std::string>& motionDestinations();
inline constexpr std::array<float,10> beatLengths{.125f,1.f/6,.25f,1.f/3,.5f,.75f,1,2,4,8};
inline std::array<int,10> resolvedRack(std::array<int,10> order){std::array<bool,10> used{};for(auto& index:order){index=bounded(index,0,9);if(used[size_t(index)])for(int candidate=0;candidate<10;++candidate)if(!used[size_t(candidate)]){index=candidate;break;}used[size_t(index)]=true;}return order;}
struct Stereo {float l=0,r=0;Stereo operator+(Stereo b)const{return {l+b.l,r+b.r};}Stereo operator*(float f)const{return {l*f,r*f};}};
struct Entropy {uint64_t bits=0x5f3b2c1d98765431ULL;float next(){bits^=bits>>12;bits^=bits<<25;bits^=bits>>27;return float((bits*2685821657736338717ULL)>>40)/8388608.f-1.f;}};
}
