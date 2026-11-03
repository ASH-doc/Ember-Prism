// sb-motion.cpp — Sample-driven envelopes and independent arp/sequence lanes.
#include "sb-motion.h"
namespace SynthB {
float EnvelopeState::advance(const float* c,float sr){
 switch(stage){
  case 1:position+=1/(std::max(.001f,c[attack])*sr);value=start+(1-start)*std::pow(std::min(1.f,position),1/c[curve]);if(position>=1){value=1;position=0;stage=c[hold]>.0001f?2:3;}break;
  case 2:position+=1/sr;if(position>=c[hold]){position=0;stage=3;}break;
  case 3:position+=1/(std::max(.005f,c[decay])*sr);value=c[sustain]+(1-c[sustain])*std::pow(std::max(0.f,1-position),c[curve]);if(position>=1){value=c[sustain];stage=4;}break;
  case 4:value+=(c[sustain]-value)*.003f;break;
  case 5:position+=1/(std::max(.005f,c[release])*sr);value=start*std::pow(std::max(0.f,1-position),c[curve]);if(position>=1||value<1.e-8f){value=0;stage=0;}break;
  default:value=0;
 }return value;
}
float LFOState::advance(const float* c,float sr,float tempo,Entropy& random){
 const float hz=c[sync]>.5f?tempo/(60*beatLengths[size_t(bounded(int(c[division]),0,9))]):c[rate];phase+=hz/sr;seconds+=1/sr;
 if(phase>=1){phase-=1;lastRandom=sampleHold;sampleHold=random.next();}
 float p=phase*16;int a=int(p)&15,b=(a+1)&15;float f=p-std::floor(p);
 if(c[interpolation]<.5f)f=f*f*(3-2*f);else if(c[interpolation]>1.5f)f=0;
 float output=c[point0+a]+(c[point0+b]-c[point0+a])*f;return output*(c[fade]>.001f?std::min(1.f,seconds/c[fade]):1);
}
Performance::Performance():layout(Schema::get().loc){values=Schema::get().initial();}
void Performance::clear(){held={};count=0;stamp=0;pedals.fill(false);lanes={};lastBeat=-1;shownStep=0;latched=false;}
bool Performance::running()const{return values[layout.arp+seqOn]>.5f||values[layout.seq+seqOn]>.5f;}
void Performance::settings(const State& state){values=state;bool latch=(values[layout.arp+seqOn]>.5f&&values[layout.arp+seqHold]>.5f)||(values[layout.seq+seqOn]>.5f&&values[layout.seq+seqHold]>.5f);if(latched&&!latch)for(int n=count-1;n>=0;--n)if(!held[size_t(n)].down&&!pedals[size_t(held[size_t(n)].channel-1)])held[size_t(n)]=held[size_t(--count)];latched=latch;}
void Performance::key(int note,int channel,float velocity,bool down){
 if(down){bool any=false;for(int i=0;i<count;++i)any|=held[size_t(i)].down;if(latched&&!any)count=0;
  for(int i=0;i<count;++i)if(held[size_t(i)].note==note&&held[size_t(i)].channel==channel){held[size_t(i)].down=true;held[size_t(i)].velocity=velocity;return;}
  if(count<128)held[size_t(count++)]={note,channel,velocity,true,++stamp};
  if(count==1)for(int i=0;i<2;++i)if(values[(i==0?layout.arp:layout.seq)+seqRestart]>.5f){lanes[size_t(i)].next=-1;lanes[size_t(i)].counter=0;}
 }else for(int i=count-1;i>=0;--i)if(held[size_t(i)].note==note&&held[size_t(i)].channel==channel){held[size_t(i)].down=false;if(!latched&&!pedals[size_t(channel-1)])held[size_t(i)]=held[size_t(--count)];}
}
void Performance::pedal(int channel,bool down){pedals[size_t(channel-1)]=down;if(!down&&!latched)for(int i=count-1;i>=0;--i)if(held[size_t(i)].channel==channel&&!held[size_t(i)].down)held[size_t(i)]=held[size_t(--count)];}
int Performance::advance(double beat,std::array<PulseNote,512>& out){
 int events=0;const bool jump=lastBeat>=0&&(beat<lastBeat-.002||beat-lastBeat>.5);lastBeat=beat;
 for(int laneIndex=0;laneIndex<2;++laneIndex){auto& lane=lanes[size_t(laneIndex)];int base=laneIndex==0?layout.arp:layout.seq;const float* p=values.data()+base;bool enabled=p[seqOn]>.5f;
  auto off=[&]{for(int i=0;i<lane.count&&events<512;++i){auto e=lane.sounding[size_t(i)];e.start=false;out[size_t(events++)]=e;}lane.count=0;lane.off=-1;};
  if(enabled!=lane.enabled||jump){off();lane.next=-1;lane.counter=0;lane.enabled=enabled;}
  if(!enabled)continue;if(lane.off>=0&&beat>=lane.off)off();
  if(count==0){off();lane.next=-1;lane.counter=0;continue;}
  if(lane.next<0)lane.next=beat;if(beat+1.e-10<lane.next)continue;off();
  int step=lane.counter%16;const int mode=bounded(int(p[seqMode]),0,5);if(laneIndex==1){if(mode==1)step=15-step;else if(mode==2){step=lane.counter%30;if(step>15)step=30-step;}else if(mode==3)step=bounded(int((random.next()+1)*8),0,15);}
  shownStep=step;const float duration=beatLengths[size_t(bounded(int(p[seqRate]),0,9))]*(1+(lane.counter%2?-1:1)*p[seqSwing]);lane.next=beat+duration;lane.off=beat+duration*p[seqLength]*p[stepGate0+step];
  std::array<Key,128> sorted=held;std::sort(sorted.begin(),sorted.begin()+count,[mode](const Key& a,const Key& b){return mode==4?a.order<b.order:a.note<b.note;});
  auto on=[&](const Key& k,int octaveShift){int note=k.note+octaveShift*12+int(p[stepPitch0+step]);if(note<0||note>127||events>=512||lane.count>=128||p[stepGate0+step]<.02f)return;PulseNote event{note,k.channel,laneIndex+1,k.velocity*p[stepVelocity0+step],true};out[size_t(events++)]=event;lane.sounding[size_t(lane.count++)]=event;};
  if(mode==5){for(int i=0;i<count;++i)on(sorted[size_t(i)],(lane.counter/16)%bounded(int(p[seqOctaves]),1,4));}
  else if(laneIndex==1){const Key* latest=&held[0];for(int i=1;i<count;++i)if(held[size_t(i)].order>latest->order)latest=&held[size_t(i)];on(*latest,(lane.counter/16)%bounded(int(p[seqOctaves]),1,4));}
  else {const int total=count*bounded(int(p[seqOctaves]),1,4);int at=lane.counter%total;if(mode==1)at=total-1-at;if(mode==2&&total>1){at=lane.counter%(2*total-2);if(at>=total)at=2*total-2-at;}if(mode==3)at=bounded(int((random.next()+1)*.5f*total),0,total-1);on(sorted[size_t(at%count)],at/count);}
  lane.counter=(lane.counter+1)%1000000;
 }return events;
}
}
