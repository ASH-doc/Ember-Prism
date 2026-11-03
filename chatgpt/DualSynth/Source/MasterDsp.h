#pragma once
// Master-section DSP owned by the host. Plain C++ (no JUCE types) so it can be unit-tested and reused.
// Nothing here is shared with either synth engine.
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace masterdsp {

inline float dbToGain(float db){return std::pow(10.f,db*.05f);}
inline float gainToDb(float gain){return 20.f*std::log10(std::max(gain,1.e-9f));}

// ---- FET-style peak compressor ("FET 76") -------------------------------------------------------------------------
// A software take on the classic fast FET limiting amplifier: a fixed threshold with the INPUT control driving the
// signal into it, fixed ratio buttons (4, 8, 12, 20) and an "all buttons in" mode, very fast attack, program-style
// release, and a little FET grit that grows with the gain reduction. It is an approximation built from the published
// behaviour of that style of unit, not a circuit model and not a copy of any product. Zero latency.
struct FetParams {
 float inputDb=6.f;      // 0 .. 40   drive into the fixed threshold (more input = more compression)
 float outputDb=0.f;     // -24 .. 12 makeup / trim after the compressor
 float attackUs=100.f;   // 20 .. 800 microseconds
 float releaseMs=250.f;  // 50 .. 1100 milliseconds
 float mix=1.f;          // 0 .. 1    parallel blend with the untouched input (1 = fully compressed)
 int ratio=0;            // 0..4 = 4:1, 8:1, 12:1, 20:1, all buttons in
 bool sidechainHighpass=true; // fixed 90 Hz high-pass in the detector so bass does not pump the whole mix
};

class FetCompressor {
public:
 static constexpr float thresholdDb=-10.f;       // measured after the input gain
 static constexpr float allButtonsRatio=28.f;
 static float ratioValue(int index){static const float table[4]={4.f,8.f,12.f,20.f};return index>=0&&index<4?table[index]:allButtonsRatio;}

 void prepare(double sampleRate){sr=float(sampleRate);detectorCoefficient=1.f-std::exp(-6.2831853f*90.f/sr);reset();}
 void reset(){gr=0;lp[0]=lp[1]=0;maxGr=0;}
 // Compresses in place. Returns the largest gain reduction (dB, >= 0) seen in this call.
 float process(float* left,float* right,int count,const FetParams& p){
  const bool all=p.ratio>=4;
  const float ratio=ratioValue(p.ratio),slope=1.f-1.f/ratio;
  const float threshold=all?thresholdDb-3.f:thresholdDb,knee=all?0.f:2.f;
  const float attackSeconds=std::max(1.f/sr,p.attackUs*1.e-6f*(all?.5f:1.f)),releaseSeconds=std::max(.005f,p.releaseMs*.001f*(all?.6f:1.f));
  const float attack=std::exp(-1.f/(attackSeconds*sr)),release=std::exp(-1.f/(releaseSeconds*sr));
  const float inputGain=dbToGain(p.inputDb),outputGain=dbToGain(p.outputDb),gritScale=all?1.f:.6f;
  float largest=0;
  for(int i=0;i<count;++i){
   const float dryL=left[i],dryR=right[i],xl=dryL*inputGain,xr=dryR*inputGain;
   float dl=xl,dr=xr;
   if(p.sidechainHighpass){lp[0]+=detectorCoefficient*(xl-lp[0]);lp[1]+=detectorCoefficient*(xr-lp[1]);dl=xl-lp[0];dr=xr-lp[1];}
   const float over=gainToDb(std::max(std::abs(dl),std::abs(dr)))-threshold;
   float target=0;
   if(over>knee*.5f)target=over*slope;
   else if(knee>0&&over>-knee*.5f){const float x=over+knee*.5f;target=slope*x*x/(2.f*knee);}
   const float coefficient=target>gr?attack:release;
   gr=coefficient*gr+(1.f-coefficient)*target;
   if(gr<1.e-6f)gr=0;
   const float gain=dbToGain(-gr);
   float yl=xl*gain,yr=xr*gain;
   // FET grit: zero when nothing is being compressed, growing with gain reduction.
   const float w=std::min(1.f,gr/12.f)*gritScale;
   if(w>0){const float k=1.f+1.4f*w;yl+=w*(std::tanh(yl*k)/k-yl);yr+=w*(std::tanh(yr*k)/k-yr);}
   yl*=outputGain;yr*=outputGain;
   if(p.mix>=1.f){left[i]=yl;right[i]=yr;}
   else{left[i]=dryL+p.mix*(yl-dryL);right[i]=dryR+p.mix*(yr-dryR);}
   largest=std::max(largest,gr);
  }
  maxGr=largest;return largest;
 }
 float currentGr()const{return gr;}
private:
 float sr=48000.f,detectorCoefficient=.0116f,gr=0,maxGr=0,lp[2]{};
};

// ---- Reverb input stage: pre-delay and low cut -----------------------------------------------------------------------
// With pre-delay 0 and low cut at or below 20 Hz the signal passes through bit-for-bit unchanged, so saved states from
// before these controls existed sound exactly as they did.
class ReverbInputStage {
public:
 static constexpr float maxPreDelayMs=200.f;
 void prepare(double sampleRate){sr=float(sampleRate);size=size_t(maxPreDelayMs*.001f*sr)+2;for(auto& line:buffer)line.assign(size,0.f);reset();}
 void reset(){for(auto& line:buffer)std::fill(line.begin(),line.end(),0.f);head=0;lp[0]=lp[1]=0;}
 void process(float& left,float& right,float preDelayMs,float lowCutHz){
  const size_t delay=std::min(size-1,size_t(std::max(0.f,preDelayMs)*.001f*sr+.5f));
  float in[2]={left,right};
  if(buffer[0].empty()){return;}
  float out[2]={in[0],in[1]};
  if(delay>0){for(int c=0;c<2;++c){buffer[size_t(c)][head]=in[c];out[c]=buffer[size_t(c)][(head+size-delay)%size];}}
  else{for(int c=0;c<2;++c)buffer[size_t(c)][head]=in[c];}
  head=(head+1)%size;
  if(lowCutHz>20.5f){const float a=1.f-std::exp(-6.2831853f*lowCutHz/sr);for(int c=0;c<2;++c){lp[c]+=a*(out[c]-lp[c]);out[c]-=lp[c];}}
  left=out[0];right=out[1];
 }
private:
 float sr=48000.f;size_t size=0,head=0;std::array<std::vector<float>,2> buffer;float lp[2]{};
};

}
