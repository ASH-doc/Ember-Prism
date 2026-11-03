// sa-sequencer.cpp — Bounded note storage; no audio-thread allocation.
#include "sa-sequencer.h"
namespace SynthA {
Sequencer::Sequencer() {
 onId=indexOf("arp.on");modeId=indexOf("arp.mode");octId=indexOf("arp.octaves");rateId=indexOf("arp.rate");
 swingId=indexOf("arp.swing");lengthId=indexOf("arp.gate");holdId=indexOf("arp.hold");
 gateOn=indexOf("seq.gate.on");gateRate=indexOf("seq.gate.rate");gateDepth=indexOf("seq.gate.depth");
 gateSmooth=indexOf("seq.gate.smooth");gateShape=indexOf("seq.gate.shape");gateRestart=indexOf("seq.gate.restart");
 for(int i=0;i<16;++i){velocities[i]=indexOf("arp.velocity"+std::to_string(i+1));steps[i]=indexOf("seq.gate.step"+std::to_string(i+1));ties[i]=indexOf("seq.gate.tie"+std::to_string(i+1));}
 p=defaults();
}
void Sequencer::reset(){count=0;soundingCount=0;order=0;step=0;nextBeat=offBeat=lastBeat=-1;gateOrigin=0;gateValue=1;sustain.fill(false);}
void Sequencer::update(const Values& v) {
 p=v;
 if(wasHold && p[holdId]<.5f) for(int i=count-1;i>=0;--i)if(!notes[i].down&&!sustain[size_t(notes[i].channel-1)])notes[i]=notes[--count];
 wasHold=p[holdId]>.5f;
}
void Sequencer::input(int note,float velocity,int channel,bool on) {
 if(on) {
   bool anyDown=false;for(int i=0;i<count;++i)anyDown|=notes[i].down;
   if(p[holdId]>.5f&&!anyDown)count=0;
   for(int i=0;i<count;++i)if(notes[i].note==note&&notes[i].channel==channel){notes[i].down=true;notes[i].velocity=velocity;return;}
   if(count<128)notes[count++]={note,channel,velocity,true,order++};
   if(count==1)nextBeat=-1;
 } else for(int i=count-1;i>=0;--i)if(notes[i].note==note&&notes[i].channel==channel){notes[i].down=false;if(p[holdId]<.5f&&!sustain[size_t(channel-1)])notes[i]=notes[--count];}
}
void Sequencer::control(int cc,int value,int channel) {
 if(cc==64){sustain[size_t(channel-1)]=value>=64;if(value<64&&p[holdId]<.5f)for(int i=count-1;i>=0;--i)if(notes[i].channel==channel&&!notes[i].down)notes[i]=notes[--count];}
 if (cc == 120 || cc == 123) {
   sustain[size_t(channel - 1)] = false;
   for (int i = count - 1; i >= 0; --i)
     if (notes[i].channel == channel) notes[i] = notes[--count];
   for (int i = soundingCount - 1; i >= 0; --i)
     if (sounding[i].channel == channel) sounding[i] = sounding[--soundingCount];
 }
}
int Sequencer::tick(double beat,float,float,std::array<NoteEvent,256>& out) {
 int n=0;const bool enabled=p[onId]>.5f;
 auto stop=[&]{for(int i=0;i<soundingCount;++i){out[n]=sounding[i];out[n++].on=false;}soundingCount=0;offBeat=-1;};
 if(enabled!=wasOn){stop();nextBeat=-1;step=0;wasOn=enabled;}
 if(!enabled)return n;
 if(lastBeat>=0 && (beat<lastBeat-.001 || beat-lastBeat>.25)){stop();nextBeat=-1;step=0;}lastBeat=beat;
 if(offBeat>=0&&beat>=offBeat)stop();
 if(count==0){if(soundingCount)stop();nextBeat=-1;step=0;return n;}
 if(nextBeat<0)nextBeat=beat;
 if(beat+1.e-9<nextBeat)return n;
 stop();
 const float base=divisions[size_t(clamp(int(p[rateId]),0,9))];
 const float duration=base*(1+(step%2?-1:1)*p[swingId]);
 nextBeat=beat+duration;offBeat=beat+duration*p[lengthId];
 std::array<Held,128> sorted=notes;
 const int mode=clamp(int(p[modeId]),0,5),oct=clamp(int(p[octId]),1,4);
 std::sort(sorted.begin(),sorted.begin()+count,[mode](const Held& a,const Held& b){return mode==4?a.order<b.order:a.note<b.note;});
 auto emit=[&](const Held& h,int octave) {
   const int note=h.note+octave*12;if(note>127||n>=256||soundingCount>=128)return;
   NoteEvent e {note,h.channel,h.velocity*p[velocities[size_t(step%16)]],true};out[n++]=e;sounding[soundingCount++]=e;
 };
 if(mode==5)for(int i=0;i<count;++i)emit(sorted[i],(step/count)%oct);
 else {
   const int total=count*oct;int at=step%total;
   if(mode==1)at=total-1-at;
   if(mode==2&&total>1){at=step%(2*total-2);if(at>=total)at=2*total-2-at;}
   if(mode==3)at=clamp(int((random.next()+1)*.5f*total),0,total-1);
   emit(sorted[size_t(at%count)],at/count);
 }
 step=(step+1)%1000000;return n;
}
void Sequencer::restartGate(double beat){if(p[gateRestart]>.5f)gateOrigin=beat;}
float Sequencer::gate(double beat,float sr) {
 double pos=(beat-gateOrigin)/divisions[size_t(clamp(int(p[gateRate]),0,9))];if(pos<0)pos=0;
 const int stepIndex=int(std::fmod(std::floor(pos),16.));const float phase=float(pos-std::floor(pos));
 float level=p[steps[size_t(stepIndex)]];
 if(p[ties[size_t(stepIndex)]]<.5f)level*=1-p[gateShape]*phase;
 float target=p[gateOn]>.5f?1-p[gateDepth]*(1-level):1;
 gateValue+=(target-gateValue)*std::min(1.f,1/(sr*std::max(.001f,p[gateSmooth])));return gateValue;
}
}
