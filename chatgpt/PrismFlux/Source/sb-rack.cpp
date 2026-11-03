// sb-rack.cpp — Clean ensemble, multitap modulation, spectral color and pitched reverb feedback.
#include "sb-rack.h"
namespace SynthB {
EffectRack::EffectRack():layout(Schema::get().loc){}
void EffectRack::prepare(float sampleRate){sr=sampleRate;for(auto& x:chorus)x.allocate(int(sr*.08f)+8);for(auto& x:flanger)x.allocate(int(sr*.035f)+8);for(auto& x:delay)x.allocate(int(sr*16)+8);for(auto& x:shifter)x.allocate(int(sr*.12f)+8);for(auto& x:widen)x.allocate(int(sr*.05f)+8);for(auto& x:ambience)x.allocate(int(sr*.2f)+8);for(auto& f:filter)f.setup(sr);clear();}
void EffectRack::clear(){for(auto* bank:{&chorus,&flanger,&delay,&shifter,&widen})for(auto& x:*bank)x.clear();for(auto& x:ambience)x.clear();for(auto& f:filter)f.clear();damp.fill(0);for(auto& p:phaseState)p.fill(0);flFeedback.fill(0);phaseFeedback.fill(0);deFeedback.fill(0);bass.fill(0);air.fill(0);distTone.fill(0);compress.fill(0);widthLP.fill(0);wet.fill(0);chorusPhase=flangerPhase=phaserPhase=pitchPhase=0;}
void EffectRack::update(const State& state){
 for(int f=0;f<10;++f)for(int p=0;p<fxSize;++p)parameters[size_t(f)][size_t(p)]=state[size_t(layout.fx[f]+p)];
 for(int i=0;i<10;++i)order[size_t(i)]=int(state[size_t(layout.order[i])]);order=resolvedRack(order);
 spaceAmount=state[size_t(layout.space)];euphoriaAmount=state[size_t(layout.euphoria)];
 for(auto& f:filter)f.coefficients(parameters[8][fxA],parameters[8][fxB],.5f);
}
Stereo EffectRack::module(int effect,Stereo in,double beat,float bpm){
 const auto& p=parameters[size_t(effect)];float x[2]{in.l,in.r},result[2]{in.l,in.r};
 auto rate=[&]{return p[fxSync]>.5f?bpm/(60*beatLengths[size_t(bounded(int(p[fxDivision]),0,9))]):p[fxA];};
 if(effect==0){chorusPhase+=rate()/sr;chorusPhase-=std::floor(chorusPhase);for(int c=0;c<2;++c){float y=0;for(int tap=0;tap<3;++tap){float phase=chorusPhase+float(tap)/3+float(c)*.13f;y+=chorus[c].tap(sr*(.014f+.01f*p[fxB]*(.5f+.5f*std::sin(float(tau)*phase))))/3;}chorus[c].write(x[c]+bounded(y*p[fxC],-1.f,1.f));result[c]=y;} }
 else if(effect==1){flangerPhase+=rate()/sr;flangerPhase-=std::floor(flangerPhase);for(int c=0;c<2;++c){float y=flanger[c].tap(sr*(.0005f+.008f*p[fxB]*(.5f+.5f*std::sin(float(tau)*(flangerPhase+float(c)*.25f)))));flanger[c].write(x[c]+flFeedback[c]*p[fxC]);flFeedback[c]=std::tanh(y);result[c]=y;}}
 else if(effect==2){phaserPhase+=rate()/sr;phaserPhase-=std::floor(phaserPhase);for(int c=0;c<2;++c){float coefficient=.05f+.88f*p[fxB]*(.5f+.5f*std::sin(float(tau)*(phaserPhase+float(c)*.17f)));float y=x[c]+phaseFeedback[c]*p[fxC];for(auto& memory:phaseState[c]){float next=memory-coefficient*y;memory=y+coefficient*next;y=next;}phaseFeedback[c]=std::tanh(y);result[c]=y;}}
 else if(effect==3){float target=p[fxSync]>.5f?60/bpm*beatLengths[size_t(bounded(int(p[fxDivision]),0,9))]:p[fxA];delayTime+=(target-delayTime)*.00015f;float taps[2]{};float tone=1-std::exp(-float(tau)*p[fxC]/sr);for(int c=0;c<2;++c){float y=delay[c].tap(delayTime*sr*(p[fxMode]>1.5f&&c==1?1.3333f:1));deFeedback[c]+=tone*(y-deFeedback[c]);taps[c]=deFeedback[c];}for(int c=0;c<2;++c){delay[c].write(x[c]+std::tanh(taps[p[fxMode]>.5f?1-c:c])*(p[fxB]+spaceAmount*.05f));result[c]=x[c]+taps[c];}}
 else if(effect==4){
  constexpr float times[12]{.0217f,.0271f,.0329f,.0377f,.0431f,.0499f,.0571f,.0637f,.0713f,.0799f,.0893f,.0977f};
  float t[12]{},sum=0;for(int i=0;i<12;++i){float delaySamples=times[i]*sr*(p[fxMode]>.5f&&p[fxMode]<1.5f?.72f:1.3f);float y=ambience[size_t(i)].tap(delaySamples);damp[size_t(i)]+=(.08f+(1-p[fxB])*.65f)*(y-damp[size_t(i)]);t[i]=damp[size_t(i)];sum+=t[i];}
  float reverbL=(t[0]-t[2]+t[4]+t[6]-t[8]+t[10])*.3f,reverbR=(t[1]+t[3]-t[5]+t[7]+t[9]-t[11])*.3f;
  // Two overlapping variable-delay grains transpose the feedback by one octave.
  pitchPhase+=1/(sr*.065f);pitchPhase-=std::floor(pitchPhase);float shifted[2]{};
  for(int c=0;c<2;++c){shifter[c].write(c==0?reverbL:reverbR);for(int grain=0;grain<2;++grain){float phase=pitchPhase+float(grain)*.5f;phase-=std::floor(phase);float window=.5f-.5f*std::cos(float(tau)*phase);shifted[c]+=shifter[c].tap((1-phase)*sr*.065f+2)*window;}}
  const float shimmer=bounded((p[fxMode]>1.5f?p[fxC]:0)+spaceAmount*.25f,0.f,.8f),feedback=(.55f+p[fxA]*.38f)*(1-.5f*shimmer);
  for(int i=0;i<12;++i){float excitation=(x[i&1]+x[1-(i&1)]*.3f)*.16f+shifted[i&1]*shimmer*.25f;ambience[size_t(i)].write(std::tanh(excitation+(t[i]-sum/6)*feedback));}
  result[0]=x[0]+reverbL;result[1]=x[1]+reverbR;
 }
 else if(effect==5){float gain=1+p[fxA]*24;for(int c=0;c<2;++c){float y=x[c]*gain;if(p[fxMode]<.5f)y=std::tanh(y);else if(p[fxMode]<1.5f)y=2/std::acos(-1.f)*std::asin(std::sin(y*1.5707963f));else {float steps=std::pow(2.f,p[fxC]);y=std::round(bounded(y,-1.f,1.f)*steps)/steps;}y/=1+p[fxA]*3;distTone[c]+=(1-std::exp(-float(tau)*p[fxB]/sr))*(y-distTone[c]);result[c]=distTone[c];}}
 else if(effect==6){float lowGain=std::pow(10.f,p[fxA]/20),midGain=std::pow(10.f,p[fxB]/20),airGain=std::pow(10.f,(p[fxC]+euphoriaAmount*3)/20);for(int c=0;c<2;++c){bass[c]+=(1-std::exp(-float(tau)*200/sr))*(x[c]-bass[c]);air[c]+=(1-std::exp(-float(tau)*6500/sr))*(x[c]-air[c]);result[c]=bass[c]*lowGain+(air[c]-bass[c])*midGain+(x[c]-air[c])*airGain;}}
 else if(effect==7){float amplitude=std::max(std::abs(x[0]),std::abs(x[1]));float coefficient=1-std::exp(-1/(sr*(amplitude>compress[0]?.003f:p[fxC])));compress[0]+=coefficient*(amplitude-compress[0]);float threshold=std::pow(10.f,p[fxA]/20);float gain=compress[0]>threshold?std::pow(compress[0]/threshold,1/p[fxB]-1):1;result[0]=x[0]*gain;result[1]=x[1]*gain;}
 else if(effect==8){for(int c=0;c<2;++c)result[c]=filter[c].process(x[c],p[fxMode]<.5f?0:p[fxMode]<1.5f?2:3,p[fxC]);}
 else {float mid=(x[0]+x[1])*.5f,side=(x[0]-x[1])*.5f;float delayed=widen[0].tap(sr*p[fxB]/1000+1);widen[0].write(mid);float width=bounded(p[fxA]+spaceAmount*.4f,0.f,2.f);side=side*width+(delayed-mid)*std::max(0.f,width-1)*.3f;result[0]=(mid+side)*std::sqrt(1-p[fxC]);result[1]=(mid-side)*std::sqrt(1+p[fxC]);}
 if(p[fxMode]>.5f&&(effect==0||effect==1||effect==2||effect==6||effect==7||effect==9)){float mid=(result[0]+result[1])*.5f,side=(result[0]-result[1])*.625f;result[0]=mid+side;result[1]=mid-side;}
 (void)beat;return {result[0],result[1]};
}
Stereo EffectRack::process(Stereo input,double beat,float bpm){
 for(int slot=0;slot<10;++slot){int index=order[size_t(slot)];const auto& p=parameters[size_t(index)];float target=p[fxOn]*p[fxMix];if(index==3)target=p[fxOn]*bounded(p[fxMix]+spaceAmount*.22f,0.f,1.f);if(index==4)target=p[fxOn]*bounded(p[fxMix]+spaceAmount*.4f,0.f,1.f);if(index==9)target=p[fxOn]*bounded(p[fxMix]+spaceAmount*.5f,0.f,1.f);if(index==6)target=p[fxOn]*p[fxMix];wet[size_t(index)]+=(target-wet[size_t(index)])*.001f;
  // Always clock delay-based modules so tails and bypass transitions remain continuous.
  Stereo effected=module(index,input,beat,bpm);float w=wet[size_t(index)];if(index>=4&&p[fxSync]>.5f)w*=.75f+.25f*std::cos(float(tau*beat/beatLengths[size_t(bounded(int(p[fxDivision]),0,9))]));input={input.l+(effected.l-input.l)*w,input.r+(effected.r-input.r)*w};
 }return input;
}
}
