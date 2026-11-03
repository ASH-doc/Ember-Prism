// sa-engine.cpp — 16-voice virtual analog engine, 2x processing, MIDI and modulation.
#include "sa-engine.h"
namespace SynthA {
Engine::Engine() {
 auto id=[](const std::string& s){return indexOf(s);};
 for(int n=0;n<3;++n){auto o="osc"+std::to_string(n+1)+".";oi[n]={id(o+"wave"),id(o+"flavor"),id(o+"octave"),id(o+"coarse"),id(o+"fine"),id(o+"pw"),id(o+"level"),id(o+"phase"),id(o+"free"),id(o+"uni.voices"),id(o+"uni.detune"),id(o+"uni.spread"),id(o+"uni.blend")};
 auto e="env"+std::to_string(n+1)+".";ei[n]={id(e+"attack"),id(e+"decay"),id(e+"sustain"),id(e+"release"),id(e+"curve")};
 auto l="lfo"+std::to_string(n+1)+".";li[n]={id(l+"shape"),id(l+"rate"),id(l+"sync"),id(l+"division"),id(l+"retrigger"),id(l+"fade"),id(l+"polarity")};}
 for(int n=0;n<2;++n){auto f="flt"+std::to_string(n+1)+".";fi[n]={id(f+"on"),id(f+"model"),id(f+"mode"),id(f+"cutoff"),id(f+"res"),id(f+"drive"),id(f+"key"),id(f+"env"),id(f+"velocity"),id(f+"fm")};}
 for(int n=0;n<12;++n){auto m="mod."+std::to_string(n+1)+".";mi[n]={id(m+"source"),id(m+"dest"),id(m+"amount")};}
 syncId=id("osc2.sync");fmId=id("osc1.fm");ringId=id("mix.ring");subId=id("mix.sub");subWaveId=id("mix.sub.wave");subOctId=id("mix.sub.octave");noiseId=id("mix.noise");noiseTypeId=id("mix.noise.type");ageId=id("analog.age");warmId=id("macro.warmth");satId=id("macro.saturation");routeId=id("filter.routing");balanceId=id("filter.balance");
 modeId=id("voice.mode");glideId=id("voice.glide");bendId=id("voice.bend");velocityId=id("voice.velocity");chordId=id("chord.on");bpmId=id("clock.bpm");hostId=id("clock.host");volumeId=id("master.volume");panId=id("master.pan");arpId=id("arp.on");
 for(int n=0;n<4;++n){macroIds[n]=id("macro."+std::to_string(n+1));chordIds[n]=id("chord."+std::to_string(n+1));}
 target=smooth=global=defaults();for(auto& s:scope)s.store(0);
}
void Engine::prepare(double sampleRate) {sr=float(clamp(sampleRate,8000.,192000.));osr=sr*2;effects.prepare(osr);reset();}
void Engine::reset() {
 for(int i=0;i<maxVoices;++i){voices[i]=Voice{};voices[i].random.state=0x82a7f32du+uint32_t(i)*198491317u;}
 for(auto& h:held)h.fill(0);for(auto& h:heldOrder)h.fill(0);
 sustain.fill(false);bend.fill(0);wheel.fill(0);pressure.fill(0);effects.reset();sequencer.reset();
 downL.reset();downR.reset();outputDC[0]={};outputDC[1]={};beat=0;counter=0;controlCounter=0;noteAge=0;
 smooth=global=target;lastMode=int(target[modeId]);lastArp=target[arpId]>.5f;peak.store(0);
}
void Engine::setParameters(const Values& v) {const auto& p=parameters();for(size_t i=0;i<p.size();++i)target[i]=p[i].limit(v[i]);}
void Engine::allNotesOff(){for(auto& v:voices)if(v.active){for(auto& e:v.env)e.off();v.down=v.sustained=false;}for(auto& h:held)h.fill(0);sustain.fill(false);sequencer.reset();}
void Engine::panic(){const double savedBeat=beat;reset();beat=savedBeat;}
int Engine::activeVoices()const{int n=0;for(auto& v:voices)n+=v.active?1:0;return n;}
void Engine::trigger(int note,float velocity,int channel,int parent,bool legato) {
 if(note<0||note>127)return;
 Voice* selected=nullptr;
 if(int(global[modeId])!=0)selected=&voices[0];
 else { for(auto& v:voices)if(!v.active){selected=&v;break;}
   if(!selected){selected=&voices[0];for(auto& v:voices)if((!v.down&&selected->down)||((v.down==selected->down)&&v.age<selected->age))selected=&v;}}
 auto& v=*selected;const bool continuing=v.active;
 if(!(legato&&continuing)) {
   const float tailL=v.lastL+v.tailL,tailR=v.lastR+v.tailR,oldPitch=v.pitch;const uint32_t seed=v.random.state;
   v=Voice{};v.tailL=tailL;v.tailR=tailR;v.random.state=seed?seed:12345;
   v.pitch=(continuing&&int(global[modeId])!=0&&smooth[glideId]>.001f)?oldPitch:float(note);
   v.tolerance=v.random.next();v.randomValue=v.random.next();
   for(int o=0;o<3;++o)for(int u=0;u<8;++u)v.phase[o][u]=smooth[oi[o].free]>.5f?(v.random.next()+1)*.5f:smooth[oi[o].phase];
   for(int l=0;l<3;++l){v.lfoPhase[l]=smooth[li[l].retrigger]>.5f?0:std::fmod(float(double(counter)/osr)*smooth[li[l].rate],1.f);v.lfoHold[l]=v.random.next();v.lfoPrev[l]=v.random.next();}
   for(auto& e:v.env)e.on();
 }
 v.active=v.down=true;v.sustained=false;v.note=note;v.parent=parent;v.channel=channel;v.targetPitch=float(note);
 v.velocity=std::pow(clamp(velocity,.001f,1.f),global[velocityId]);v.age=++noteAge;v.p=smooth;updateVoice(v,smooth[bpmId]);
}
void Engine::release(int note,int channel) {
 for(auto& v:voices)if(v.active&&v.parent==note&&v.channel==channel&&v.down){v.down=false;if(sustain[size_t(channel-1)])v.sustained=true;else for(auto& e:v.env)e.off();}
}
void Engine::directNote(int note,float velocity,int channel,bool on) {
 if(on) {
   if(global[chordId]>.5f && int(global[modeId])==0) {
     std::array<bool,128> used{};for(int n=0;n<4;++n){const int pitch=clamp(note+int(global[chordIds[n]]),0,127);if(!used[size_t(pitch)]){trigger(pitch,velocity,channel,note);used[size_t(pitch)]=true;}}
   }else trigger(note,velocity,channel,note,int(global[modeId])==2);
 } else release(note,channel);
}
void Engine::midi(const MidiEvent& e) {
 const int c=clamp(e.channel,1,16),n=clamp(e.note,0,127);const auto ci=size_t(c-1);
 switch(e.type) {
  case MidiEvent::NoteOn:
   if(e.value<=0){MidiEvent off=e;off.type=MidiEvent::NoteOff;midi(off);break;}
   held[ci][size_t(n)]=e.value;heldOrder[ci][size_t(n)]=++noteAge;sequencer.input(n,e.value,c,true);sequencer.restartGate(beat);
   if(!lastArp)directNote(n,e.value,c,true);break;
  case MidiEvent::NoteOff:
   held[ci][size_t(n)]=0;sequencer.input(n,0,c,false);
   if(!lastArp){
     bool resumed=false;
     if (int(global[modeId]) != 0 && voices[0].parent == n && voices[0].channel == c) {
       uint64_t newest = 0;
       int next = -1, nextChannel = 0;
       for (int ch = 0; ch < 16; ++ch)
         for (int key = 0; key < 128; ++key)
           if (held[ch][key] > 0 && heldOrder[ch][key] > newest) {
             newest = heldOrder[ch][key]; next = key; nextChannel = ch;
           }
       if (next >= 0) {
         directNote(next, held[nextChannel][next], nextChannel + 1, true);
         resumed = true;
       }
     }
     if(!resumed)directNote(n,0,c,false);
   }break;
  case MidiEvent::PitchBend:bend[ci]=clamp(e.value,-1.f,1.f);break;
  case MidiEvent::Aftertouch:pressure[ci]=clamp(e.value,0.f,1.f);break;
  case MidiEvent::ControlChange:
   sequencer.control(n,int(e.value),c);
   if(n==1)wheel[ci]=e.value/127;
   if(n==64){sustain[ci]=e.value>=64;if(!sustain[ci])for(auto& v:voices)if(v.channel==c&&v.sustained){v.sustained=false;for(auto& env:v.env)env.off();}}
   if (n == 120 || n == 123) {
     held[ci].fill(0);
     heldOrder[ci].fill(0);
     sustain[ci] = false;
     for (auto& voice : voices) {
       if (!voice.active || voice.channel != c) continue;
       voice.down = voice.sustained = false;
       if (n == 120) {
         voice.active = false;
         voice.lastL = voice.lastR = voice.tailL = voice.tailR = 0;
       } else {
         for (auto& envelope : voice.env) envelope.off();
       }
     }
   }
   break;
  case MidiEvent::Panic:panic();break;
 }
}
void Engine::applyMods(Values& dest,const Voice& v) {
 std::array<float,16> source {0,v.velocity,(float(v.note)-60)/60,wheel[size_t(v.channel-1)],pressure[size_t(v.channel-1)],v.envOut[0],v.envOut[1],v.envOut[2],v.lfoOut[0],v.lfoOut[1],v.lfoOut[2],v.randomValue,v.p[macroIds[0]],v.p[macroIds[1]],v.p[macroIds[2]],v.p[macroIds[3]]};
 const auto& defs=parameters();
 for(const auto& row:mi){const int src=clamp(int(v.p[row[0]]),0,15),dst=int(v.p[row[1]])-1;if(src==0||dst<0||dst>=int(defs.size()))continue;const auto& d=defs[size_t(dst)];dest[size_t(dst)]=d.limit(d.denormalise(d.normalise(dest[size_t(dst)])+source[size_t(src)]*v.p[row[2]]));}
}
void Engine::updateVoice(Voice& v,float bpm) {
 Values p=smooth;applyMods(p,v);v.p=p;
 const float glide=p[glideId];v.pitch+=glide>.001f?(v.targetPitch-v.pitch)*(1-std::exp(-32/(osr*glide))):(v.targetPitch-v.pitch);
 v.drift+=(v.driftTarget-v.drift)*.0015f;
 if(--v.driftCounter<=0){v.driftTarget=v.random.next();v.driftCounter=int(osr/32*.5f);}
 for(int o=0;o<3;++o){const auto& i=oi[o];const int count=p[i.wave]>3.5f?7:clamp(int(p[i.voices]),1,8);v.count[o]=count;
   const float pitch=v.pitch+p[i.octave]*12+p[i.coarse]+p[i.fine]/100+bend[size_t(v.channel-1)]*p[bendId]+p[ageId]*(v.tolerance*.035f+v.drift*.12f);
   for(int u=0;u<count;++u){const float pos=count==1?0:float(u)*2/float(count-1)-1;const float cents=pos*pos*pos*p[i.detune]*p[i.detune]*65;
     v.delta[o][u]=clamp(440.f*std::pow(2.f,(pitch-69+cents/100)/12)/osr,.000001f,.44f);
     const float pan=pos*p[i.spread];const float weight=(count==1?1.f:(1-std::abs(pos))*(1-p[i.blend])+p[i.blend]);
     v.gainL[o][u]=std::sqrt((1-pan)*.5f)*weight/std::sqrt(float(count));v.gainR[o][u]=std::sqrt((1+pan)*.5f)*weight/std::sqrt(float(count));
   }
 }
 for(int f=0;f<2;++f){auto& i=fi[f];const float octaves=(v.pitch-60)/12*p[i.key]+p[i.env]*v.envOut[1]*6+p[i.velocity]*(v.velocity-.5f)*3+p[ageId]*v.tolerance*.03f;
   float hz=p[i.cutoff]*std::pow(2.f,octaves);for(auto& filter:v.filter[f])filter.tune(hz,osr);}
 (void)bpm;
}
void Engine::renderVoice(Voice& v,float& l,float& r,float bpm) {
 const auto& p=v.p;
 for(int e=0;e<3;++e){const auto& i=ei[e];v.envOut[e]=v.env[e].tick(p[i.a],p[i.d],p[i.s],p[i.r],p[i.curve],osr);}
 for(int n=0;n<3;++n){const auto& i=li[n];const float rate=p[i.sync]>.5f?bpm/60/divisions[size_t(clamp(int(p[i.division]),0,9))]:p[i.rate];
   v.lfoPhase[n]+=rate/osr;if(v.lfoPhase[n]>=1){v.lfoPhase[n]-=1;v.lfoPrev[n]=v.lfoHold[n];v.lfoHold[n]=v.random.next();}
   const float phase=v.lfoPhase[n];float a=0;switch(int(p[i.shape])){case 0:a=std::sin(float(2*pi)*phase);break;case 1:a=1-4*std::abs(phase-.5f);break;case 2:a=phase*2-1;break;case 3:a=phase<.5f?1.f:-1.f;break;case 4:a=v.lfoHold[n];break;default:a=v.lfoPrev[n]+(v.lfoHold[n]-v.lfoPrev[n])*(phase*phase*(3-2*phase));}
   if(p[i.polarity]>.5f)a=(a+1)*.5f*(p[i.polarity]>1.5f?-1:1);
   v.lfoOut[n]=a*(p[i.fade]>.001f?std::min(1.f,v.time/p[i.fade]):1);
 }
 float oscL[3]{},oscR[3]{};bool syncWrap=false;
 const float fm=std::sin(float(2*pi)*v.phase[1][0])*p[fmId]*.12f;
 for(int o=0;o<3;++o){const auto& i=oi[o];
   for(int u=0;u<v.count[o];++u){float dt=v.delta[o][u]*(o==0?1+fm:1);float phase=v.phase[o][u];
     // Hard sync runs at 2x; extreme resets still need higher-order BLEP refinement.
     if(o==1&&p[syncId]>.5f&&syncWrap)phase=std::fmod(v.phase[0][0]*dt/std::max(v.delta[0][0],.000001f),1.f);
     float a=wave(phase,dt,int(p[i.wave]),p[i.pw]);
     const int flavor=int(p[i.flavor]);if(flavor==1)a=soft(a*1.3f+.12f)-soft(.12f);else if(flavor==2)a=.8f*a+.2f*std::sin(float(2*pi)*phase);else if(flavor==3)a=soft(a*2+.22f)-soft(.22f);
     oscL[o]+=a*v.gainL[o][u]*p[i.level];oscR[o]+=a*v.gainR[o][u]*p[i.level];
     phase+=dt;if(phase>=1){phase-=1;if(o==0&&u==0)syncWrap=true;}v.phase[o][u]=phase;
   }
 }
 const float subDt=440.f*std::pow(2.f,(v.pitch-69+bend[size_t(v.channel-1)]*p[bendId])/12)/osr/(p[subOctId]>.5f?4:2);
 float sub=wave(v.subPhase,subDt,p[subWaveId]>.5f?1:3,.5f)*p[subId];v.subPhase+=subDt;v.subPhase-=std::floor(v.subPhase);
 float noise=v.random.next();v.pink=.985f*v.pink+.015f*noise;noise=(p[noiseTypeId]>.5f?v.pink*5:noise)*(p[noiseId]+p[ageId]*.0005f);
 float a[2]{oscL[0]+oscL[1],oscR[0]+oscR[1]},b[2]{oscL[2]+sub*.707f,oscR[2]+sub*.707f};
 for(int c=0;c<2;++c){const float ring=(c==0?oscL[0]*oscL[1]:oscR[0]*oscR[1]);a[c]=a[c]*(1-p[ringId])+ring*p[ringId];a[c]+=noise*.5f;}
 float out[2]{};
 for(int c=0;c<2;++c){
   auto filter=[&](int n,float x){const auto& i=fi[n];if(p[i.on]<.5f)return x;
     if(p[i.fm]>.001f){const float base=v.filter[n][c].targetG;v.filter[n][c].g=clamp(base*(1+p[i.fm]*.8f*oscL[1]),.0001f,1.f);}
     return v.filter[n][c].tick(x,int(p[i.model]),int(p[i.mode]),p[i.res],p[i.drive]);};
   const float pre=1+p[satId]*6+p[ageId]*.25f;
   const float warmth=p[warmId],bias=.18f*warmth;
   auto color=[&](float x){return (soft(x*pre*.45f+bias)-soft(bias))/.45f/(1+p[satId]*1.7f);};
   if(p[routeId]<.5f)out[c]=filter(1,filter(0,color(a[c]+b[c])));
   else if(p[routeId]<1.5f){float x=color(a[c]+b[c]);out[c]=filter(0,x)*(1-p[balanceId])+filter(1,x)*p[balanceId];}
   else out[c]=filter(0,color(a[c]))*(1-p[balanceId])*2+filter(1,color(b[c]))*p[balanceId]*2;
   v.warmLP[c]+=.22f*(out[c]-v.warmLP[c]);out[c]+=warmth*.55f*(v.warmLP[c]-out[c]);out[c]=v.dc[c].tick(out[c]);
 }
 v.fade=std::min(1.f,v.fade+1/(osr*.003f));const float amp=v.envOut[0]*v.velocity*.5f*v.fade;
 v.lastL=out[0]*amp;v.lastR=out[1]*amp;v.tailL*=.992f;v.tailR*=.992f;
 l+=v.lastL+v.tailL;r+=v.lastR+v.tailR;v.time+=1/osr;
 if(v.env[0].stage==Envelope::idle&&std::abs(v.tailL)+std::abs(v.tailR)<1.e-6f)v.active=false;
}
void Engine::controlTick(float bpm) {
 const auto& defs=parameters();const float slew=1-std::exp(-32/(osr*.012f));
 for(size_t i=0;i<defs.size();++i)smooth[i]=defs[i].discrete?target[i]:smooth[i]+slew*(target[i]-smooth[i]);
 const Voice* newest=nullptr;for(auto& v:voices)if(v.active){updateVoice(v,bpm);if(!newest||v.age>newest->age)newest=&v;}
 global=smooth;if(newest)applyMods(global,*newest);
 const bool arp=global[arpId]>.5f;const int mode=int(global[modeId]);
 if(arp!=lastArp||mode!=lastMode){for(auto& v:voices){v.down=v.sustained=false;for(auto& e:v.env)e.off();}lastArp=arp;lastMode=mode;}
 effects.update(global);sequencer.update(global);
}
void Engine::render(float* left,float* right,int samples,const MidiEvent* events,int eventCount,double hostBpm,double ppq) {
 const float bpm=global[hostId]>.5f&&hostBpm>=20&&hostBpm<=400?float(hostBpm):global[bpmId];
 if(ppq>=0&&target[hostId]>.5f)beat=ppq;
 int event=0;float blockPeak=0;
 for(int s=0;s<samples;++s){
   if(controlCounter==0)controlTick(bpm);
   while(event<eventCount&&events[event].sample<=s)midi(events[event++]);
   for(int os=0;os<2;++os){
     if(controlCounter==0&&os!=0)controlTick(bpm);
     const int count=sequencer.tick(beat,bpm,osr,seqEvents);for(int i=0;i<count;++i){const auto& e=seqEvents[size_t(i)];directNote(e.note,e.velocity,e.channel,e.on);}
     float l=0,r=0;for(auto& v:voices)if(v.active)renderVoice(v,l,r,bpm);
     const float gate=sequencer.gate(beat,osr);l*=gate;r*=gate;effects.process(l,r,beat,bpm);
     const float gain=std::pow(10.f,global[volumeId]/20),pan=global[panId];
     l=outputDC[0].tick(l)*gain*std::sqrt(1-pan);r=outputDC[1].tick(r)*gain*std::sqrt(1+pan);
     // Gentle emergency ceiling at -6 dBFS; finite guard prevents poisoned host buffers.
     l=std::isfinite(l)?.5f*soft(l*2):0;r=std::isfinite(r)?.5f*soft(r*2):0;
     downL.push(l);downR.push(r);beat+=double(bpm)/(60*osr);++counter;controlCounter=(controlCounter+1)%32;
   }
   left[s]=downL.read();right[s]=downR.read();blockPeak=std::max(blockPeak,std::max(std::abs(left[s]),std::abs(right[s])));
   if((counter&15)==0){const unsigned w=scopeWrite.load(std::memory_order_relaxed);scope[w%512].store((left[s]+right[s])*.5f,std::memory_order_relaxed);scopeWrite.store(w+1,std::memory_order_release);}
 }
 peak.store(std::max(blockPeak,peak.load()*.9f));
}
}
