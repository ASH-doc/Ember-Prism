// sb-engine.cpp — Stereo mipmapped playback, voice lifecycle, modulation and host-synchronized rendering.
#include "sb-engine.h"
namespace SynthB {
Instrument::Instrument():schema(Schema::get()),layout(schema.loc),tables(Tables::get()){
 desired=slewed=global=schema.initial();for(auto& v:displayed)v.store(0);for(auto& v:sources)v.store(0);for(auto& v:spectrum)v.store(0);for(auto& v:voiceLights)v.store(false);
 for(int i=0;i<16;++i)pool[size_t(i)].rng.bits+=uint64_t(i+1)*0x65aa10c7ULL;
}
void Instrument::prepare(double sr){sampleRate=float(bounded(sr,8000.,192000.));internalRate=2*sampleRate;for(auto& v:pool)for(auto& pair:v.filters)for(auto& f:pair)f.setup(internalRate);rack.prepare(internalRate);reset();}
void Instrument::parameters(const State& values){for(size_t i=0;i<schema.specs.size();++i)desired[i]=schema.specs[i].valid(values[i]);}
void Instrument::reset(){for(auto& v:pool){v.running=false;v.pressed=false;v.pedalled=false;v.envelopes={};v.lfos={};v.tailL=v.tailR=v.previousL=v.previousR=0;for(auto& f:v.filters)for(auto& channel:f)channel.clear();v.dc[0]={};v.dc[1]={};}for(auto& k:keys)k.fill(0);for(auto& p:priorities)p.fill(0);wheels.fill(0);bend.fill(0);bendSmooth.fill(0);pressure.fill(0);pedals.fill(false);performance.clear();rack.clear();leftDown.clear();rightDown.clear();outputDC[0]={};outputDC[1]={};slewed=global=desired;performance.settings(global);modulationClock=0;lastMode=int(desired[size_t(layout.mode)]);lastPerformance=performance.running();meter.store(0);}
void Instrument::releaseAll(){for(auto& v:pool){v.pressed=v.pedalled=false;for(auto& env:v.envelopes)env.lift();}for(auto& k:keys)k.fill(0);pedals.fill(false);performance.clear();}
int Instrument::active()const{int count=0;for(const auto& v:pool)if(v.running)++count;return count;}
void Instrument::start(int key,int channel,float velocity,int origin){
 if(key<0||key>127)return;const int mode=int(global[size_t(layout.mode)]);Voice* voice=nullptr;
 if(mode!=0)voice=&pool[0];else {for(auto& v:pool)if(!v.running){voice=&v;break;}if(!voice){voice=&pool[0];for(auto& v:pool)if((!v.pressed&&voice->pressed)||(v.pressed==voice->pressed&&v.serial<voice->serial))voice=&v;}}
 auto& v=*voice;bool legato=mode==2&&v.running&&v.pressed;bool continuing=v.running;float previousNote=v.note;
 if(!legato){v.tailL=v.previousL;v.tailR=v.previousR;v.previousL=v.previousR=0;v.envelopes={};v.lfos={};v.source.fill(0);v.filterWet.fill(0);v.subPhase=0;v.fadeIn=0;v.random=v.rng.next();v.noiseLow=0;v.dc[0]={};v.dc[1]={};
  for(auto& pair:v.filters)for(auto& f:pair)f.clear();for(auto& env:v.envelopes)env.strike();
  for(int o=0;o<2;++o){int base=layout.osc[o];for(int lane=0;lane<16;++lane){float phase=global[size_t(base+SynthB::phase)]+(v.rng.next()+1)*.5f*global[size_t(base+randomPhase)];v.phase[size_t(o)][size_t(lane)]=phase-std::floor(phase);}}
  for(int l=0;l<4;++l){int base=layout.lfo[l];v.lfos[size_t(l)].phase=global[size_t(base+restart)]>.5f?0:float(std::fmod(double(frameClock)/internalRate*global[size_t(base+rate)],1.));v.lfos[size_t(l)].sampleHold=v.rng.next();}
  v.note=continuing&&mode!=0&&global[size_t(layout.glide)]>.001f?previousNote:float(key);
 }
 v.running=v.pressed=true;v.pedalled=false;v.key=key;v.channel=channel;v.origin=origin;v.target=float(key);v.serial=++serial;v.velocity=std::pow(bounded(velocity,.001f,1.f),global[size_t(layout.velocityCurve)]);v.local=slewed;refresh(v,global[size_t(layout.tempo)]);
}
void Instrument::stop(int key,int channel,int origin){for(auto& v:pool)if(v.running&&v.key==key&&v.channel==channel&&v.origin==origin&&v.pressed){v.pressed=false;if(origin==0&&pedals[size_t(channel-1)])v.pedalled=true;else for(auto& env:v.envelopes)env.lift();}}
void Instrument::message(const Event& event){
 const int channel=bounded(event.channel,1,16),key=bounded(event.key,0,127);const size_t c=size_t(channel-1);
 switch(event.kind){
  case Event::Down:if(event.value<=0){Event off=event;off.kind=Event::Up;message(off);return;}keys[c][size_t(key)]=event.value;priorities[c][size_t(key)]=++serial;performance.key(key,channel,event.value,true);if(!performance.running())start(key,channel,event.value,0);break;
  case Event::Up:{keys[c][size_t(key)]=0;performance.key(key,channel,0,false);bool replaced=false;if (!performance.running() && int(global[size_t(layout.mode)]) != 0 &&
      pool[0].key == key && pool[0].channel == channel) {
    uint64_t newest = 0;
    int note = -1, nextChannel = 0;
    for (int ch = 0; ch < 16; ++ch)
     for (int n = 0; n < 128; ++n)
      if (keys[ch][n] > 0 && priorities[ch][n] > newest) {
       newest = priorities[ch][n]; note = n; nextChannel = ch;
      }
    if (note >= 0) {
     start(note, nextChannel + 1, keys[nextChannel][note], 0);
     replaced = true;
    }
   }if(!replaced)stop(key,channel,0);break;}
  case Event::Bend:bend[c]=bounded(event.value,-1.f,1.f);break;
  case Event::Pressure:pressure[c]=bounded(event.value,0.f,1.f);break;
  case Event::Controller:
   if(key==1)wheels[c]=event.value/127;
   if(key==64){pedals[c]=event.value>=64;performance.pedal(channel,pedals[c]);if(!pedals[c])for(auto& v:pool)if(v.channel==channel&&v.pedalled){v.pedalled=false;for(auto& env:v.envelopes)env.lift();}}
   if (key == 120 || key == 123) {
    keys[c].fill(0);
    priorities[c].fill(0);
    pedals[c] = false;
    performance.clearChannel(channel);
    for (auto& voice : pool) {
     if (!voice.running || voice.channel != channel) continue;
     voice.pressed = voice.pedalled = false;
     if (key == 120) {
      voice.running = false;
      voice.tailL = voice.tailR = voice.previousL = voice.previousR = 0;
     } else {
      for (auto& envelope : voice.envelopes) envelope.lift();
     }
    }
   }
   break;
  case Event::Silence:reset();break;
 }
}
void Instrument::modulate(State& state,const Voice& voice){
 for(int r=0;r<routes;++r){int base=layout.route[r];int src=bounded(int(voice.local[size_t(base+source)]),0,modulationSourceCount-1),dest=int(voice.local[size_t(base+destination)])-1;if(src==0||dest<0||dest>=int(schema.specs.size()))continue;
  float amount=voice.source[size_t(src)];if(voice.local[size_t(base+bipolar)]<.5f&&(src>=4&&src<=7||src==12||src==13||src==pitchBendSource))amount=(amount+1)*.5f;
  amount=std::copysign(std::pow(std::abs(amount),voice.local[size_t(base+shape)]),amount);const auto& spec=schema.specs[size_t(dest)];state[size_t(dest)]=spec.valid(spec.realValue(spec.unitValue(state[size_t(dest)])+amount*voice.local[size_t(base+depth)]));
 }
}
void Instrument::refresh(Voice& v,float){
 State next=slewed;modulate(next,v);v.local=next;const auto& p=v.local;
 const float glide=p[size_t(layout.glide)];v.note=glide>.001f?v.note+(v.target-v.note)*(1-std::exp(-32/(internalRate*glide))):v.target;
 const float eup=p[size_t(layout.euphoria)];
 for(int o=0;o<2;++o){int base=layout.osc[o];int voices=bounded(int(p[size_t(base+unison)]),1,16);v.lanes[size_t(o)]=voices;
  const float pitch=v.note+p[size_t(base+octave)]*12+p[size_t(base+tune)]+p[size_t(base+fine)]*.01f+bendSmooth[size_t(v.channel-1)]*p[size_t(layout.bend)];
  float detune=p[size_t(base+SynthB::detune)]*(1+eup*.25f),width=bounded(p[size_t(base+spread)]+eup*.1f,0.f,1.f);
  float bandwidth=warpBandwidth(int(p[size_t(base+warp1)]),p[size_t(base+amount1)])*warpBandwidth(int(p[size_t(base+warp2)]),p[size_t(base+amount2)]);
  for(int u=0;u<voices;++u){const float location=voices==1?0:2.f*float(u)/float(voices-1)-1;float offset=location;int curve=int(p[size_t(base+detuneCurve)]);if(curve==1)offset=std::copysign(location*location,location);if(curve==2)offset=location*(.25f+.75f*location*location);
   const float dt=bounded(440.f*std::exp2((pitch-69+offset*detune*detune*.8f)/12)/internalRate,.000001f,.42f);v.increment[size_t(o)][size_t(u)]=dt;
   float panorama=bounded(p[size_t(base+pan)]+location*width,-1.f,1.f);float weight=(1-std::abs(location))*(1-p[size_t(base+blend)])+p[size_t(base+blend)];if(voices==1)weight=1;
   v.stereoL[size_t(o)][size_t(u)]=std::sqrt((1-panorama)*.5f)*weight/std::sqrt(float(voices));v.stereoR[size_t(o)][size_t(u)]=std::sqrt((1+panorama)*.5f)*weight/std::sqrt(float(voices));
   v.position[size_t(o)][size_t(u)]=bounded(p[size_t(base+pos)]+location*p[size_t(base+wtSpread)]*.5f+eup*.035f*v.source[4],0.f,1.f);
   v.mip[size_t(o)][size_t(u)]=bounded(std::log2(std::max(1.f,dt*2048*bandwidth/ .82f)),0.f,9.f);
  }
 }
 for(int f=0;f<2;++f){int b=layout.filter[f];float oct=(v.note-60)/12*p[size_t(b+keyTrack)]+v.source[2]*p[size_t(b+envAmount)]*5+(v.velocity-.5f)*p[size_t(b+velocity)]*2;
  float hz=p[size_t(b+cutoff)]*std::exp2(oct);for(auto& filter:v.filters[size_t(f)])filter.coefficients(hz,p[size_t(b+resonance)],p[size_t(b+vowel)]);
 }
}
Stereo Instrument::sample(Voice& v,float bpm){
 const State& p=v.local;
 v.source[0]=0;for(int e=0;e<3;++e)v.source[size_t(e+1)]=v.envelopes[size_t(e)].advance(p.data()+layout.env[e],internalRate);
 for(int l=0;l<4;++l)v.source[size_t(l+4)]=v.lfos[size_t(l)].advance(p.data()+layout.lfo[l],internalRate,bpm,v.rng);
 v.source[8]=v.velocity;v.source[9]=(float(v.key)-60)/60;v.source[10]=wheels[size_t(v.channel-1)];v.source[11]=pressure[size_t(v.channel-1)];v.source[12]=v.random;v.source[13]=v.lfos[0].sampleHold;
 for(int m=0;m<8;++m)v.source[size_t(14+m)]=p[size_t(layout.macro[m])];v.source[22]=p[size_t(layout.euphoria)];v.source[23]=p[size_t(layout.space)];
 v.source[pitchBendSource] = bendSmooth[size_t(v.channel - 1)];
 const float sub=std::sin(float(tau)*v.subPhase);float other[2]{std::sin(float(tau)*v.phase[1][0]),std::sin(float(tau)*v.phase[0][0])};Stereo oscillators[2];
 for(int o=0;o<2;++o){const int b=layout.osc[o];const WaveSet& wt=tables.at(int(p[size_t(b+table)]),imported);float mod=p[size_t(b+fmSource)]>.5f?sub:other[o];
  for(int u=0;u<v.lanes[size_t(o)];++u){float phase=v.phase[size_t(o)][size_t(u)];phase=warpedPhase(phase,int(p[size_t(b+warp1)]),p[size_t(b+amount1)],mod);phase=warpedPhase(phase,int(p[size_t(b+warp2)]),p[size_t(b+amount2)],mod);
   float y=wt.sample(phase,v.position[size_t(o)][size_t(u)],v.mip[size_t(o)][size_t(u)]);y=warpedAmplitude(y,int(p[size_t(b+warp1)]),p[size_t(b+amount1)],mod);y=warpedAmplitude(y,int(p[size_t(b+warp2)]),p[size_t(b+amount2)],mod);
   oscillators[o].l+=y*v.stereoL[size_t(o)][size_t(u)]*p[size_t(b+level)];oscillators[o].r+=y*v.stereoR[size_t(o)][size_t(u)]*p[size_t(b+level)];
   float next=v.phase[size_t(o)][size_t(u)]+v.increment[size_t(o)][size_t(u)];v.phase[size_t(o)][size_t(u)]=next-std::floor(next);
  }
 }
 const float fundamental=440.f*std::exp2((v.note-69+bendSmooth[size_t(v.channel-1)]*p[size_t(layout.bend)])/12);v.subPhase+=fundamental/internalRate/(p[size_t(layout.subOct)]>.5f?4:2);v.subPhase-=std::floor(v.subPhase);
 float subWave=p[size_t(layout.subWave)]>.5f?std::asin(sub)*.63661977f:sub;float noise=v.rng.next();v.noiseLow+=.025f*(noise-v.noiseLow);noise=(v.noiseLow*4*(1-p[size_t(layout.noiseColor)])+noise*p[size_t(layout.noiseColor)])*p[size_t(layout.noise)];
 const float extras=(subWave*p[size_t(layout.sub)]+noise)*.7071068f;oscillators[0].l+=extras*.5f;oscillators[0].r+=extras*.5f;oscillators[1].l+=extras*.5f;oscillators[1].r+=extras*.5f;
 float output[2]{};for(int f=0;f<2;++f){int b=layout.filter[f];v.filterWet[size_t(f)]+=(p[size_t(b+enabled)]*p[size_t(b+mix)]-v.filterWet[size_t(f)])*.002f;}
 for(int c=0;c<2;++c){float a=c==0?oscillators[0].l:oscillators[0].r,b=c==0?oscillators[1].l:oscillators[1].r;
  auto filter=[&](int index,float input){int base=layout.filter[index];float y=v.filters[size_t(index)][size_t(c)].process(input,int(p[size_t(base+type)]),p[size_t(base+drive)]);return input+(y-input)*v.filterWet[size_t(index)];};
  const int routing=int(p[size_t(layout.routing)]);const float balance=p[size_t(layout.filterBalance)];
  if(routing==0)output[c]=filter(1,filter(0,a+b));else if(routing==1)output[c]=filter(0,a+b)*(1-balance)+filter(1,a+b)*balance;else if(routing==2)output[c]=filter(0,a)*(1-balance)*2+filter(1,b)*balance*2;else output[c]=(filter(0,a+b)+filter(1,a+b))*.5f;
  output[c]=v.dc[c].step(output[c]);
 }
 v.fadeIn=std::min(1.f,v.fadeIn+1/(internalRate*.002f));float amplitude=.35f*v.velocity*v.source[1]*v.fadeIn*(1+p[size_t(layout.euphoria)]*.15f);
 v.previousL=output[0]*amplitude;v.previousR=output[1]*amplitude;v.tailL*=.99f;v.tailR*=.99f;
 if(v.envelopes[0].stage==0&&std::abs(v.tailL)+std::abs(v.tailR)<1.e-7f)v.running=false;
 return {v.previousL+v.tailL,v.previousR+v.tailR};
}
void Instrument::control(float bpm){
 const float coefficient=1-std::exp(-32/(internalRate*.012f));for(size_t i=0;i<schema.specs.size();++i)slewed[i]=schema.specs[i].integer?desired[i]:slewed[i]+(desired[i]-slewed[i])*coefficient;
 for(int c=0;c<16;++c)bendSmooth[size_t(c)]+=(bend[size_t(c)]-bendSmooth[size_t(c)])*coefficient;
 const Voice* newest=nullptr;for(auto& v:pool)if(v.running){refresh(v,bpm);if(!newest||v.serial>newest->serial)newest=&v;}
 global=slewed;if(newest)modulate(global,*newest);performance.settings(global);rack.update(global);
 int mode=int(global[size_t(layout.mode)]);bool performing=performance.running();if(mode!=lastMode||performing!=lastPerformance){for(auto& v:pool){v.pressed=v.pedalled=false;for(auto& e:v.envelopes)e.lift();}lastMode=mode;lastPerformance=performing;}
}
void Instrument::render(float* left,float* right,int frames,const Event* events,int count,double hostBpm,double hostBeat,const WaveSet* custom){
 imported=custom;float bpm=global[size_t(layout.host)]>.5f&&hostBpm>=20&&hostBpm<=400?float(hostBpm):global[size_t(layout.tempo)];if(global[size_t(layout.host)]>.5f&&hostBeat>=0)beat=hostBeat;
 int nextEvent=0;float peak=0;
 for(int frame=0;frame<frames;++frame){if(modulationClock==0)control(bpm);while(nextEvent<count&&events[nextEvent].offset<=frame)message(events[nextEvent++]);
  for(int os=0;os<2;++os){if(modulationClock==0&&os!=0)control(bpm);const int generatedCount=performance.advance(beat,generated);for(int n=0;n<generatedCount;++n){const auto& e=generated[size_t(n)];if(e.start)start(e.note,e.channel,e.velocity,e.origin);else stop(e.note,e.channel,e.origin);}
   Stereo mixed;for(auto& v:pool)if(v.running)mixed=mixed+sample(v,bpm);mixed=rack.process(mixed,beat,bpm);
   const float gain=std::pow(10.f,global[size_t(layout.master)]/20),pan=global[size_t(layout.pan)];mixed.l=outputDC[0].step(mixed.l)*gain*std::sqrt(1-pan);mixed.r=outputDC[1].step(mixed.r)*gain*std::sqrt(1+pan);
   auto ceiling=[](float x){if(!std::isfinite(x))return 0.f;float a=std::abs(x);if(a>.42f)a=.42f+.08f*std::tanh((a-.42f)/.08f);return std::copysign(a,x);};leftDown.feed(ceiling(mixed.l));rightDown.feed(ceiling(mixed.r));beat+=double(bpm)/(60*internalRate);++frameClock;modulationClock=(modulationClock+1)&31;
  }
  left[frame]=leftDown.output();right[frame]=rightDown.output();peak=std::max(peak,std::max(std::abs(left[frame]),std::abs(right[frame])));
  unsigned index=spectrumIndex.load(std::memory_order_relaxed);spectrum[size_t(index%2048)].store((left[frame]+right[frame])*.5f,std::memory_order_relaxed);spectrumIndex.store(index+1,std::memory_order_release);
 }
 const Voice* newest=nullptr;for(size_t i=0;i<pool.size();++i){const auto& v=pool[i];voiceLights[i].store(v.running);if(v.running&&(!newest||v.serial>newest->serial))newest=&v;}
 for(size_t i=0;i<schema.specs.size();++i)displayed[i].store(newest?newest->local[i]:global[i],std::memory_order_relaxed);for(size_t i=0;i<sources.size();++i)sources[i].store(newest?newest->source[i]:0,std::memory_order_relaxed);step.store(performance.playhead());meter.store(std::max(peak,meter.load()*.9f));
}
}
