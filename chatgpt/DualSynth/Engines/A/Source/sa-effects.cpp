// sa-effects.cpp — Oversampled tube/tape drive, modulation, dark FDN reverb, delay, shelves and pump.
#include "sa-effects.h"
namespace SynthA {
Effects::Effects() {
 const char* names[]={"fx.drive.on","fx.drive.mix","fx.drive.mode","fx.drive.amount","fx.drive.tone",
 "fx.chorus.on","fx.chorus.mix","fx.chorus.rate","fx.chorus.depth","fx.chorus.feedback",
 "fx.flanger.on","fx.flanger.mix","fx.flanger.rate","fx.flanger.depth","fx.flanger.feedback",
 "fx.phaser.on","fx.phaser.mix","fx.phaser.rate","fx.phaser.depth","fx.phaser.feedback",
 "fx.delay.on","fx.delay.mix","fx.delay.sync","fx.delay.division","fx.delay.time","fx.delay.feedback","fx.delay.tone","fx.delay.pingpong",
 "fx.reverb.on","fx.reverb.mix","fx.reverb.mode","fx.reverb.size","fx.reverb.damp",
 "fx.eq.on","fx.eq.mix","fx.eq.low","fx.eq.high","fx.pump.on","fx.pump.mix","fx.pump.rate","fx.pump.depth","fx.pump.curve"};
 for(int i=0;i<Count;++i) ids[i]=indexOf(names[i]);
}
void Effects::prepare(float sampleRate) {
 sr=sampleRate; for(auto& d:chorus)d.prepare(size_t(sr*.06f)+4);
 for(auto& d:flanger)d.prepare(size_t(sr*.025f)+4); for(auto& d:delay)d.prepare(size_t(sr*16)+4);
 for(auto& d:reverb)d.prepare(size_t(sr*.16f)+4);
 lowCoeff=1-std::exp(float(-2*pi)*200/sr); highCoeff=1-std::exp(float(-2*pi)*4500/sr); reset();
}
void Effects::reset() {
 for(auto* bank:{&chorus,&flanger,&delay})for(auto& d:*bank)d.clear(); for(auto& d:reverb)d.clear();
 mix.fill(0);reverbDamp.fill(0);driveLP.fill(0);delayLP.fill(0);low.fill(0);highLP.fill(0);
 phaseFeedback.fill(0);chFeedback.fill(0);flFeedback.fill(0);for(auto& p:phaser)p.fill(0);dc={};chPhase=flPhase=phPhase=0;
}
void Effects::update(const Values& p) {
 for(int i=0;i<Count;++i)v[i]=p[ids[i]];
 driveCoeff=1-std::exp(float(-2*pi)*v[DriveTone]/sr);delayCoeff=1-std::exp(float(-2*pi)*v[DeTone]/sr);
 lowGain=std::pow(10.f,v[EqLow]/20);highGain=std::pow(10.f,v[EqHigh]/20);
}
void Effects::process(float& left,float& right,double beat,float bpm) {
 const int on[]={DriveOn,ChOn,FlOn,PhOn,DeOn,ReOn,EqOn,PuOn};
 for(int i=0;i<8;++i)mix[i]+=(v[on[i]]*v[on[i]+1]-mix[i])*.001f;
 chPhase+=v[ChRate]/sr;flPhase+=v[FlRate]/sr;phPhase+=v[PhRate]/sr;
 chPhase-=std::floor(chPhase);flPhase-=std::floor(flPhase);phPhase-=std::floor(phPhase);
 std::array<float,2> x {left,right};
 for(int c=0;c<2;++c) {
   float wet=x[c];
   const float gain=1+v[DriveAmount]*14;
   if(v[DriveMode]<.5f)wet=(soft(wet*gain+.18f)-soft(.18f))/(1+v[DriveAmount]*2);
   else if(v[DriveMode]<1.5f)wet=soft(wet*gain)/(1+v[DriveAmount]*2);
   else wet=clamp(wet*gain,-.85f,.85f)/(1+v[DriveAmount]*2);
   driveLP[c]+=driveCoeff*(wet-driveLP[c]); x[c]+=(dc[c].tick(driveLP[c])-x[c])*mix[0];
   const float ch=chorus[c].read(sr*(.014f+.007f*v[ChDepth]*std::sin(float(2*pi)*(chPhase+c*.25f))));
   chorus[c].push(x[c]+chFeedback[c]*v[ChFeedback]);chFeedback[c]=soft(ch);
   x[c]+=(ch-x[c])*(mix[1]*.65f);
   const float fl=flanger[c].read(sr*(.001f+.004f*v[FlDepth]*(.5f+.5f*std::sin(float(2*pi)*(flPhase+c*.25f)))));
   flanger[c].push(x[c]+flFeedback[c]*v[FlFeedback]);flFeedback[c]=soft(fl);
   x[c]+=(fl-x[c])*(mix[2]*.65f);
   const float a=.15f+.7f*v[PhDepth]*(.5f+.5f*std::sin(float(2*pi)*(phPhase+c*.13f)));
   float y=x[c]+phaseFeedback[c]*v[PhFeedback];
   for(auto& state:phaser[c]) {const float out=-a*y+state;state=y+a*out;y=out;}
   phaseFeedback[c]=soft(y);x[c]+=(y-x[c])*(mix[3]*.65f);
 }
 const float time=v[DeSync]>.5f?divisions[size_t(clamp(int(v[DeDivision]),0,9))]*60/bpm:v[DeTime];
 delaySamples+=.0002f*(clamp(time*sr,1.f,sr*15)-delaySamples);
 std::array<float,2> delayed{};
 for(int c=0;c<2;++c) { const float d=delay[c].read(delaySamples);delayLP[c]+=delayCoeff*(d-delayLP[c]);delayed[c]=delayLP[c]; }
 for(int c=0;c<2;++c) {delay[c].push(x[c]*std::min(1.f,mix[4]*4)+soft(delayed[v[DePing]>.5f?1-c:c])*v[DeFeedback]);x[c]+=delayed[c]*mix[4];}
 // Eight-line orthogonal feedback network (Householder), incommensurate delay lengths.
 constexpr float times[]={.0297f,.0371f,.0411f,.0437f,.0531f,.0617f,.0719f,.0793f};
 std::array<float,8> taps{};float sum=0;
 for(int i=0;i<8;++i) {
   taps[i]=reverb[i].read(times[i]*sr*(v[ReMode]>.5f?1.7f:1.f));
   reverbDamp[i]+=(.04f+(1-v[ReDamp])*.65f)*(taps[i]-reverbDamp[i]);taps[i]=reverbDamp[i];sum+=taps[i];
 }
 const float feedback=.55f+.4f*v[ReSize];
 for(int i=0;i<8;++i)reverb[i].push((x[i%2]+x[1-i%2]*.35f)*.18f*std::min(1.f,mix[5]*4)+soft((taps[i]-.25f*sum)*feedback));
 x[0]+=(taps[0]+taps[2]-taps[4]+taps[6])*.5f*mix[5];x[1]+=(taps[1]-taps[3]+taps[5]+taps[7])*.5f*mix[5];
 const double cyc=beat/divisions[size_t(clamp(int(v[PuRate]),0,9))];
 const float pump=1-v[PuDepth]*std::pow(1-float(cyc-std::floor(cyc)),v[PuCurve]);
 for(int c=0;c<2;++c) {
   low[c]+=lowCoeff*(x[c]-low[c]);highLP[c]+=highCoeff*(x[c]-highLP[c]);
   x[c]+=mix[6]*((lowGain-1)*low[c]+(highGain-1)*(x[c]-highLP[c]));
   x[c]*=1+mix[7]*(pump-1);
 }
 left=x[0];right=x[1];
}
}
