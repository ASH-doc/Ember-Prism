// sa-parameters.cpp — Single source of truth for every sound parameter.
#include "sa-parameters.h"
#include <stdexcept>
namespace SynthA {
const std::vector<std::string>& modSources() {
    static const std::vector<std::string> s {"Off","Velocity","Key","Mod wheel","Pressure","Env 1","Env 2","Env 3","LFO 1","LFO 2","LFO 3","Random","Macro 1","Macro 2","Macro 3","Macro 4"}; return s;
}
const std::vector<Param>& parameters() {
 static const auto table=[] {
    std::vector<Param> p;
    auto add=[&](std::string id,std::string label,float lo,float hi,float def,std::string group,std::string unit="",float skew=1.f,bool discrete=false) {
      p.push_back({id,label,unit,group,lo,hi,def,skew,{},discrete});
    };
    auto choice=[&](std::string id,std::string label,std::vector<std::string> items,int def,std::string group) {
      add(id,label,0,float(items.size()-1),float(def),group,"",1,true); p.back().choices=std::move(items);
    };
    auto toggle=[&](std::string id,std::string label,bool on,std::string g) { choice(id,label,{"Off","On"},on?1:0,g); };
    const std::vector<std::string> div {"1/32","1/16 T","1/16","1/8 T","1/8","1/8 D","1/4","1/2","1 bar","2 bars"};
    for(int n=1;n<=3;++n) {
      auto id="osc"+std::to_string(n)+".", g="Oscillator "+std::to_string(n);
      choice(id+"wave","Wave",{"Saw","Pulse","Triangle","Sine","Hypersaw"},0,g);
      choice(id+"flavor","Character",{"Velvet","Copper","Reed","Cinder"},0,g);
      add(id+"octave","Octave",-3,3,0,g,"oct",1,true);
      add(id+"coarse","Tune",-24,24,0,g,"st",1,true);
      add(id+"fine","Fine",-100,100,n==2?7:0,g,"ct");
      add(id+"pw","Width",.05f,.95f,.5f,g);
      add(id+"level","Level",0,1,n==1?.8f:n==2?.35f:0,g);
      add(id+"phase","Phase",0,1,0,g); toggle(id+"free","Free run",true,g);
      add(id+"uni.voices","Voices",1,8,1,g,"",1,true);
      add(id+"uni.detune","Detune",0,1,.2f,g);
      add(id+"uni.spread","Spread",0,1,.75f,g);
      add(id+"uni.blend","Blend",0,1,.7f,g);
    }
    toggle("osc2.sync","Hard sync",false,"Mixer");
    add("osc1.fm","FM",0,1,0,"Mixer"); add("mix.ring","Ring",0,1,0,"Mixer");
    add("mix.sub","Sub",0,1,.18f,"Mixer"); choice("mix.sub.wave","Sub wave",{"Sine","Square"},0,"Mixer");
    choice("mix.sub.octave","Sub octave",{"-1","-2"},0,"Mixer");
    add("mix.noise","Noise",0,1,0,"Mixer"); choice("mix.noise.type","Noise type",{"White","Pink"},1,"Mixer");
    add("analog.age","Age",0,1,.2f,"Character"); add("macro.warmth","Warmth",0,1,.35f,"Macros");
    add("macro.saturation","Saturation",0,1,.15f,"Macros");
    for(int n=1;n<=2;++n) {
      auto id="flt"+std::to_string(n)+".",g="Filter "+std::to_string(n);
      toggle(id+"on","Enabled",n==1,g);
      choice(id+"model","Model",{"Ember 24","Silk 12","Snarl 12","Forge SVF"},0,g);
      choice(id+"mode","Response",{"Low pass","High pass","Band pass","Notch"},0,g);
      add(id+"cutoff","Cutoff",20,20000,n==1?4500.f:9000.f,g,"Hz",.23f);
      add(id+"res","Resonance",0,1,.18f,g); add(id+"drive","Drive",0,1,.12f,g);
      add(id+"key","Key track",0,1,.35f,g); add(id+"env","Env 2",-1,1,.3f,g);
      add(id+"velocity","Velocity",-1,1,.15f,g); add(id+"fm","Filter FM",0,1,0,g);
    }
    choice("filter.routing","Routing",{"Serial","Parallel","Split"},0,"Filter routing");
    add("filter.balance","Balance",0,1,.5f,"Filter routing");
    for(int n=1;n<=3;++n) {
      auto id="env"+std::to_string(n)+".",g="Envelope "+std::to_string(n);
      add(id+"attack","Attack",.001f,10,.008f,g,"s",.3f);
      add(id+"decay","Decay",.005f,10,.4f,g,"s",.3f);
      add(id+"sustain","Sustain",0,1,n==1?.8f:.25f,g);
      add(id+"release","Release",.005f,15,.3f,g,"s",.3f);
      add(id+"curve","Curve",.3f,3,1.4f,g);
    }
    for(int n=1;n<=3;++n) {
      auto id="lfo"+std::to_string(n)+".",g="LFO "+std::to_string(n);
      choice(id+"shape","Shape",{"Sine","Triangle","Saw","Square","Hold","Smooth random"},0,g);
      add(id+"rate","Rate",.01f,30,n==1?1.f:.25f,g,"Hz",.3f);
      toggle(id+"sync","Tempo sync",false,g); choice(id+"division","Division",div,6,g);
      toggle(id+"retrigger","Key sync",true,g); add(id+"fade","Fade in",0,10,0,g,"s",.4f);
      choice(id+"polarity","Polarity",{"Bipolar","Positive","Negative"},0,g);
    }
    for(int n=1;n<=4;++n) add("macro."+std::to_string(n),"Macro "+std::to_string(n),0,1,0,"Macros");
    choice("voice.mode","Voice mode",{"Poly","Mono","Legato"},0,"Performance");
    add("voice.glide","Glide",0,2,0,"Performance","s",.4f);
    add("voice.bend","Bend range",1,24,2,"Performance","st",1,true);
    add("voice.velocity","Velocity curve",.3f,3,1,"Performance");
    toggle("chord.on","Chord memory",false,"Performance");
    for(int n=1;n<=4;++n) add("chord."+std::to_string(n),"Interval "+std::to_string(n),-24,24,n==1?0:n==2?7:n==3?12:16,"Performance","st",1,true);
    add("clock.bpm","Tempo",30,300,138,"Clock","BPM"); toggle("clock.host","Follow host",true,"Clock");
    toggle("arp.on","Arpeggiator",false,"Arpeggiator");
    choice("arp.mode","Pattern",{"Up","Down","Up-down","Random","As played","Chord"},0,"Arpeggiator");
    add("arp.octaves","Octaves",1,4,1,"Arpeggiator","",1,true);
    choice("arp.rate","Rate",div,2,"Arpeggiator");
    add("arp.swing","Swing",0,.65f,0,"Arpeggiator"); add("arp.gate","Length",.05f,.95f,.6f,"Arpeggiator");
    toggle("arp.hold","Hold",false,"Arpeggiator");
    for(int n=0;n<16;++n) add("arp.velocity"+std::to_string(n+1),"Velocity "+std::to_string(n+1),.05f,1,n%4==0?1:.8f,"Arp velocity");
    toggle("seq.gate.on","Trance gate",false,"Trance gate"); choice("seq.gate.rate","Rate",div,2,"Trance gate");
    add("seq.gate.depth","Depth",0,1,1,"Trance gate"); add("seq.gate.smooth","Smooth",.001f,.1f,.004f,"Trance gate","s",.4f);
    add("seq.gate.shape","Shape",0,1,.2f,"Trance gate"); toggle("seq.gate.restart","Key restart",true,"Trance gate");
    for(int n=0;n<16;++n) { add("seq.gate.step"+std::to_string(n+1),"Step "+std::to_string(n+1),0,1,n%4==3?0:1,"Gate steps"); toggle("seq.gate.tie"+std::to_string(n+1),"Tie "+std::to_string(n+1),false,"Gate ties"); }
    for(auto name:{"drive","chorus","flanger","phaser","delay","reverb","eq","pump"}) {
      auto id=std::string("fx.")+name+".",g=std::string("FX ")+name;
      toggle(id+"on","Enabled",false,g); add(id+"mix","Mix",0,1,std::string(name)=="eq"?1.f:.25f,g);
    }
    choice("fx.drive.mode","Circuit",{"Tube","Tape","Hard"},0,"FX drive"); add("fx.drive.amount","Drive",0,1,.3f,"FX drive"); add("fx.drive.tone","Tone",200,18000,5500,"FX drive","Hz",.3f);
    for(auto name:{"chorus","flanger","phaser"}) {
      auto id=std::string("fx.")+name+".",g=std::string("FX ")+name;
      add(id+"rate","Rate",.02f,8,.3f,g,"Hz",.4f); add(id+"depth","Depth",0,1,.5f,g); add(id+"feedback","Feedback",0,.85f,.25f,g);
    }
    toggle("fx.delay.sync","Sync",true,"FX delay"); choice("fx.delay.division","Division",div,5,"FX delay");
    add("fx.delay.time","Time",.01f,2,.375f,"FX delay","s",.5f); add("fx.delay.feedback","Feedback",0,.88f,.35f,"FX delay");
    add("fx.delay.tone","Tone",200,16000,3500,"FX delay","Hz",.3f); toggle("fx.delay.pingpong","Ping pong",true,"FX delay");
    choice("fx.reverb.mode","Space",{"Dark plate","Dark hall"},0,"FX reverb"); add("fx.reverb.size","Size",0,1,.65f,"FX reverb"); add("fx.reverb.damp","Damping",0,1,.65f,"FX reverb");
    add("fx.eq.low","Low shelf",-12,12,0,"FX eq","dB"); add("fx.eq.high","High shelf",-12,12,-1.5f,"FX eq","dB");
    choice("fx.pump.rate","Rate",div,6,"FX pump"); add("fx.pump.depth","Depth",0,1,.65f,"FX pump"); add("fx.pump.curve","Curve",.3f,4,1.5f,"FX pump");
    add("master.volume","Volume",-60,0,-9,"Master","dB"); add("master.pan","Pan",-1,1,0,"Master");
    // Destinations include all entries, including matrix metadata. Matrix recursion uses the previous control tick.
    const int total=int(p.size())+36;
    std::vector<std::string> destinations {"None"}; for(auto& v:p) destinations.push_back(v.id);
    for(int n=1;n<=12;++n) for(auto s:{"source","dest","amount"}) destinations.push_back("mod."+std::to_string(n)+"."+s);
    for(int n=1;n<=12;++n) {
      auto id="mod."+std::to_string(n)+"."; std::string g="Modulation matrix";
      choice(id+"source","Source",modSources(),0,g); choice(id+"dest","Destination",destinations,0,g);
      add(id+"amount","Amount",-1,1,0,g);
    }
    if(int(p.size())!=total || p.size()>maxParams) throw std::runtime_error("Parameter capacity");
    return p;
 }(); return table;
}
int indexOf(const std::string& id) { const auto& p=parameters(); for(size_t i=0;i<p.size();++i) if(p[i].id==id) return int(i); return -1; }
Values defaults() { Values v{}; const auto& p=parameters(); for(size_t i=0;i<p.size();++i) v[i]=p[i].initial; return v; }
}
