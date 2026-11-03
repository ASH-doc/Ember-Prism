// sb-schema.cpp — All automation, UI, patch and modulation IDs are defined here once.
#include "sb-schema.h"
#include <stdexcept>
namespace SynthB {
const std::vector<std::string>& sourceNames(){static const std::vector<std::string> names{"None","Env 1","Env 2","Env 3","LFO 1","LFO 2","LFO 3","LFO 4","Velocity","Key","Wheel","Pressure","Random","Sample hold","Macro 1","Macro 2","Macro 3","Macro 4","Macro 5","Macro 6","Macro 7","Macro 8","Euphoria","Space","Pitch Bend"};return names;}
const std::vector<std::string>& tableNames(){static const std::vector<std::string> names{"Vector Saw","Pulse Atlas","Harmonic Fold","Prism Ramp","Triangle Orbit","Glass Mesh","Chrome Grid","Crystal Teeth","Ion Shards","Digital Silk","Vowel Arc","Formant Lane","Choir Vector","Whisper Mouth","Vocal Prism","Fluid Cell","Coral Thread","Grain Current","Plasma Ribbon","Organic Lattice","Bell Spectrum","FM Crown","Chime Matrix","Metal Rain","Orbital Gong","Air Spectrum","Frozen Hiss","Cloud Comb","White Halo","Spectral Dust","Imported"};return names;}
const std::vector<std::string>& effectNames(){static const std::vector<std::string> n{"Chorus","Flanger","Phaser","Delay","Reverb","Distortion","EQ","Compressor","Filter","Widener"};return n;}
const Schema& Schema::get(){static Schema s;return s;}
int Schema::find(const std::string& key)const{for(size_t i=0;i<specs.size();++i)if(specs[i].key==key)return int(i);return -1;}
State Schema::initial()const{State s{};for(size_t i=0;i<specs.size();++i)s[i]=specs[i].initial;return s;}
Schema::Schema(){
 auto number=[&](std::string key,std::string title,float low,float high,float value,std::string section,std::string unit="",float power=1,bool integer=false){int at=int(specs.size());specs.push_back({key,title,section,unit,low,high,value,power,integer,{}});return at;};
 auto menu=[&](std::string key,std::string title,const std::vector<std::string>& values,int value,std::string section){int at=number(key,title,0,float(values.size()-1),float(value),section,"",1,true);specs.back().options=values;return at;};
 auto flag=[&](std::string key,std::string title,bool value,std::string section){return menu(key,title,{"Off","On"},value?1:0,section);};
 const std::vector<std::string> lengths{"1/32","1/16 T","1/16","1/8 T","1/8","1/8 D","1/4","1/2","1 bar","2 bars"};
 const std::vector<std::string> warps{"None","Sync","Bend +","Bend -","Mirror","Asymmetry","FM","AM","Ring","PWM","Quantize","Flip"};
 for(int n=0;n<2;++n){auto k="osc"+std::to_string(n+1)+".",s="Oscillator "+std::to_string(n+1);loc.osc[n]=int(specs.size());
  menu(k+"table","Table",tableNames(),n==0?5:0,s);number(k+"pos","Position",0,1,.25f,s);
  number(k+"octave","Octave",-3,3,0,s,"oct",1,true);number(k+"tune","Semitone",-24,24,0,s,"st",1,true);number(k+"fine","Fine",-100,100,n==0?0:7,s,"ct");
  number(k+"level","Level",0,1,n==0?.75f:.3f,s);number(k+"pan","Pan",-1,1,0,s);number(k+"phase","Phase",0,1,0,s);number(k+"random","Phase random",0,1,1,s);
  number(k+"uni.voices","Unison",1,16,n==0?3:1,s,"",1,true);number(k+"uni.detune","Detune",0,1,.18f,s);number(k+"uni.spread","Width",0,1,.8f,s);number(k+"uni.blend","Blend",0,1,.7f,s);number(k+"uni.wtspread","WT spread",0,1,.15f,s);menu(k+"uni.curve","Detune curve",{"Linear","Exponential","Prism"},2,s);
  menu(k+"warp1.type","Warp A",warps,0,s);number(k+"warp1.amount","Warp A amount",0,1,0,s);menu(k+"warp2.type","Warp B",warps,0,s);number(k+"warp2.amount","Warp B amount",0,1,0,s);menu(k+"fm.source","FM source",{"Other oscillator","Sub"},0,s);
 }
 loc.sub=number("mix.sub","Sub level",0,1,.1f,"Sources");loc.subOct=menu("mix.sub.octave","Sub octave",{"-1","-2"},0,"Sources");loc.subWave=menu("mix.sub.wave","Sub wave",{"Sine","Triangle"},0,"Sources");loc.noise=number("mix.noise","Noise",0,1,0,"Sources");loc.noiseColor=number("mix.noise.color","Noise color",0,1,.8f,"Sources");
 for(int n=0;n<2;++n){auto k="flt"+std::to_string(n+1)+".",s="Filter "+std::to_string(n+1);loc.filter[n]=int(specs.size());flag(k+"on","Power",n==0,s);menu(k+"type","Type",{"LP 12","LP 24","High pass","Band pass","Notch","Comb","Vowel","Air shelf","Low shelf"},0,s);number(k+"cutoff","Cutoff",20,20000,14000,s,"Hz",.23f);number(k+"res","Resonance",0,1,.12f,s);number(k+"drive","Drive",0,1,0,s);number(k+"mix","Mix",0,1,1,s);number(k+"key","Key tracking",0,1,.25f,s);number(k+"env","Env 2",-1,1,.1f,s);number(k+"velocity","Velocity",-1,1,.1f,s);number(k+"vowel","Vowel",0,1,.35f,s);}
 loc.routing=menu("filter.route","Routing",{"Serial","Parallel","Per oscillator","Both to both"},0,"Routing");loc.filterBalance=number("filter.balance","Balance",0,1,.5f,"Routing");
 for(int n=0;n<3;++n){auto k="env"+std::to_string(n+1)+".",s="Envelope "+std::to_string(n+1);loc.env[n]=int(specs.size());number(k+"attack","Attack",.001f,12,.006f,s,"s",.3f);number(k+"hold","Hold",0,5,0,s,"s",.35f);number(k+"decay","Decay",.005f,12,.45f,s,"s",.3f);number(k+"sustain","Sustain",0,1,n==0?.8f:.3f,s);number(k+"release","Release",.005f,20,.45f,s,"s",.3f);number(k+"curve","Curve",.25f,4,1.3f,s);}
 for(int n=0;n<4;++n){auto k="lfo"+std::to_string(n+1)+".",s="LFO "+std::to_string(n+1);loc.lfo[n]=int(specs.size());number(k+"rate","Rate",.01f,40,n==0?.5f:1.f,s,"Hz",.3f);flag(k+"sync","Sync",true,s);menu(k+"division","Division",lengths,7+n%3,s);flag(k+"restart","Key restart",true,s);number(k+"fade","Fade",0,12,0,s,"s",.35f);menu(k+"interpolation","Curve",{"Smooth","Linear","Steps"},0,s);for(int j=0;j<16;++j)number(k+"point"+std::to_string(j+1),"Point "+std::to_string(j+1),-1,1,float(std::sin(tau*j/16)),s);}
 for(int n=0;n<8;++n)loc.macro[n]=number("macro"+std::to_string(n+1)+".value","Macro "+std::to_string(n+1),0,1,0,"Macros");
 loc.euphoria=number("macro.euphoria","Euphoria",0,1,.12f,"Macros");loc.space=number("macro.space","Space",0,1,.15f,"Macros");
 loc.mode=menu("voice.mode","Voice mode",{"Poly","Mono","Legato"},0,"Performance");loc.glide=number("voice.glide","Glide",0,2,0,"Performance","s",.4f);loc.bend=number("voice.bend","Bend range",1,24,2,"Performance","st",1,true);loc.velocityCurve=number("voice.velocity","Velocity curve",.3f,3,1,"Performance");loc.tempo=number("clock.bpm","Tempo",30,300,138,"Clock","BPM");loc.host=flag("clock.host","Host sync",true,"Clock");
 // Arp and pitched step sequencer each have their own independent lanes.
 for(int n=0;n<2;++n){const std::string k=n==0?"arp.":"seq.",s=n==0?"Arpeggiator":"Step sequencer";int start=int(specs.size());if(n==0)loc.arp=start;else loc.seq=start;
  flag(k+"on","Enabled",false,s);menu(k+"mode","Mode",{"Up","Down","Up down","Random","As played","Chord"},0,s);menu(k+"rate","Rate",lengths,2,s);number(k+"swing","Swing",0,.65f,0,s);number(k+"length","Gate length",.05f,.98f,.65f,s);flag(k+"hold","Latch",false,s);number(k+"octaves","Octaves",1,4,1,s,"",1,true);flag(k+"restart","Key restart",true,s);
  for(int j=0;j<16;++j)number(k+"step"+std::to_string(j+1)+".pitch","Pitch "+std::to_string(j+1),-24,24,0,s,"st",1,true);
  for(int j=0;j<16;++j)number(k+"step"+std::to_string(j+1)+".gate","Gate "+std::to_string(j+1),0,1,j%4==3?.6f:1.f,s);
  for(int j=0;j<16;++j)number(k+"step"+std::to_string(j+1)+".velocity","Velocity "+std::to_string(j+1),.05f,1,j%4==0?1.f:.8f,s);
 }
 for(int n=0;n<10;++n){auto k="fx"+std::to_string(n+1)+".",s=effectNames()[n];loc.fx[n]=int(specs.size());flag(k+"on","Power",n==3||n==4||n==6||n==9,s);number(k+"mix","Mix",0,1,n==6||n==7||n==8?1:.3f,s);
  const char* titles[10][3]={{"Rate","Depth","Feedback"},{"Rate","Depth","Feedback"},{"Rate","Depth","Feedback"},{"Time","Feedback","Tone"},{"Size","Damping","Shimmer"},{"Drive","Tone","Resolution"},{"Low","Mid","Air"},{"Threshold","Ratio","Release"},{"Cutoff","Resonance","Drive"},{"Width","Delay","Balance"}};
  const float ranges[10][3][3]={{{.01f,8,.35f},{0,1,.5f},{0,.85f,.15f}},{{.01f,10,.2f},{0,1,.5f},{0,.88f,.3f}},{{.01f,12,.25f},{0,1,.65f},{0,.85f,.25f}},{{.01f,3,.375f},{0,.92f,.3f},{200,20000,12000}},{{0,1,.7f},{0,1,.25f},{0,1,.3f}},{{0,1,.25f},{200,20000,16000},{2,16,10}},{{-12,12,0},{-12,12,0},{-12,12,2}},{{-48,0,-18},{1,20,4},{.01f,1,.12f}},{{20,20000,14000},{0,1,.15f},{0,1,0}},{{0,2,1.2f},{0,30,7},{-1,1,0}}};
  for(int j=0;j<3;++j){auto& r=ranges[n][j];number(k+std::string(1,char('a'+j)),titles[n][j],r[0],r[1],r[2],s,"",r[1]>100?.25f:1.f);}
  menu(k+"mode","Mode",n==4?std::vector<std::string>{"Hall","Plate","Shimmer"}:n==5?std::vector<std::string>{"Soft clip","Wavefold","Bitcrush"}:n==3?std::vector<std::string>{"Stereo","Ping pong","Cross"}:n==8?std::vector<std::string>{"Low pass","High pass","Band pass"}:std::vector<std::string>{"Normal","Wide"},0,s);
  flag(k+"sync",n>=4?"Tempo motion":"Sync",n==3,s);menu(k+"division","Division",lengths,5,s);
 }
 for(int n=0;n<10;++n)loc.order[n]=menu("rack.slot"+std::to_string(n+1),"Slot "+std::to_string(n+1),effectNames(),n,"Rack order");
 for(int n=0;n<6;++n)loc.locks[n]=flag("random.lock"+std::to_string(n+1),std::vector<std::string>{"Oscillators","Filters","Envelopes","Modulation","Sequence","Effects"}[n],n==4,"Randomizer locks");
 loc.master=number("master.gain","Output",-60,0,-9,"Master","dB");loc.pan=number("master.pan","Pan",-1,1,0,"Master");
 std::vector<std::string> dest{"None"};for(const auto& p:specs)dest.push_back(p.key);for(int n=0;n<routes;++n)for(auto suffix:{"source","destination","depth","bipolar","curve"})dest.push_back("mod"+std::to_string(n+1)+"."+suffix);
 for(int n=0;n<routes;++n){auto k="mod"+std::to_string(n+1)+".";loc.route[n]=int(specs.size());menu(k+"source","Source",sourceNames(),0,"Matrix");menu(k+"destination","Destination",dest,0,"Matrix");number(k+"depth","Depth",-1,1,0,"Matrix");flag(k+"bipolar","Bipolar",true,"Matrix");number(k+"curve","Curve",.25f,4,1,"Matrix");}
 if(specs.size()>capacity)throw std::runtime_error("Synth B parameter capacity exceeded");
}
}
