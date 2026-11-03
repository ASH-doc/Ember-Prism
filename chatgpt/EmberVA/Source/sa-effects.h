// sa-effects.h — Synth A's private, algorithmic stereo effects.
#pragma once
#include "sa-dsp.h"
namespace SynthA {
struct DelayLine {
    std::vector<float> data; size_t head=0;
    void prepare(size_t size) { data.assign(size,0); head=0; }
    float read(float samples) const { float at=float(head)-clamp(samples,1.f,float(data.size()-2)); if(at<0)at+=float(data.size()); auto a=size_t(at); auto b=(a+1)%data.size(); return data[a]+(data[b]-data[a])*(at-float(a)); }
    void push(float x) {data[head]=x;head=(head+1)%data.size();}
    void clear(){std::fill(data.begin(),data.end(),0);head=0;}
};
class Effects {
 public:
    Effects(); void prepare(float sampleRate); void reset();
    void update(const Values&); void process(float& left,float& right,double beat,float bpm);
 private:
    enum {DriveOn,DriveMix,DriveMode,DriveAmount,DriveTone,ChOn,ChMix,ChRate,ChDepth,ChFeedback,
      FlOn,FlMix,FlRate,FlDepth,FlFeedback,PhOn,PhMix,PhRate,PhDepth,PhFeedback,
      DeOn,DeMix,DeSync,DeDivision,DeTime,DeFeedback,DeTone,DePing,
      ReOn,ReMix,ReMode,ReSize,ReDamp,EqOn,EqMix,EqLow,EqHigh,PuOn,PuMix,PuRate,PuDepth,PuCurve,Count};
    std::array<int,Count> ids{}; std::array<float,Count> v{};
    float sr=96000; std::array<float,8> mix{};
    std::array<DelayLine,2> chorus,flanger,delay;
    std::array<DelayLine,8> reverb;
    std::array<float,8> reverbDamp{};
    std::array<std::array<float,6>,2> phaser{};
    std::array<float,2> driveLP{},delayLP{},low{},highLP{},phaseFeedback{},chFeedback{},flFeedback{};
    std::array<DCBlock,2> dc;
    float chPhase=0,flPhase=0,phPhase=0,delaySamples=24000,driveCoeff=.1f,delayCoeff=.1f;
    float lowGain=1,highGain=1,lowCoeff=.01f,highCoeff=.1f;
};
}
