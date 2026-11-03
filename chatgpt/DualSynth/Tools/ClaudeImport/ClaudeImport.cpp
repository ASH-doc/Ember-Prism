// ClaudeImport — converts the Claude baseline's factory presets into ChatGPT-engine patches (version 1 map).
//
//   DualClaudeImport <claude-export.json> <output folder>
//
// Reads the export written by the Claude project's DuoPresetExport tool, maps every EMBER preset to a Synth A patch and
// every PRISM preset to a Synth B patch (parameter IDs, units, enum indexes, wavetables, modulation routes, FX slots),
// level-matches each result against the Claude render, measures how close it sounds, and writes:
//   claude-import-ember.json, claude-import-prism.json   patch banks in the ChatGPT patch format
//   claude-import-report.md                              status, distance and every unmapped setting per preset
// Mapping is by parameter meaning. The two engines are different DSP, so every result is an approximation; the measured
// distance only helps sort the results, it does not replace listening.
#include <JuceHeader.h>
#include "sa-processor.h"
#include "sb-plugin.h"
#include "sb-tables.h"
#include "MatchFeatures.h"
#include <cstdio>
#include <map>
#include <set>

using namespace juce;

namespace {

constexpr int kMapVersion=1;
constexpr double kExactThresholdDb=4.0;   // a Claude table whose nearest built-in table is further than this is embedded exactly
constexpr double kRate=48000.0,kHold=1.0,kTotal=2.0;

// ---------------------------------------------------------------------------------------------------------------------
String lower(String s){return s.toLowerCase().removeCharacters(" ");}
double num(const var& object,const String& key,double fallback=0){const var v=object[Identifier(key)];return v.isVoid()?fallback:double(v);}

struct Report{
 String engine,name,category,status;int index=0;match::Distance distance;float gainTrimDb=0;std::vector<String> notes;
};

// Render a processor with a held C3 and a 1 s hold, like the exporter did for the Claude engines.
match::Features renderFeatures(AudioProcessor& processor){
 const int block=256,total=int(kTotal*kRate);processor.setRateAndBufferSizeDetails(kRate,block);processor.prepareToPlay(kRate,block);
 std::vector<float> mono(size_t(total),0.f);AudioBuffer<float> audio(2,block);bool on=false,off=false;
 for(int at=0;at<total;at+=block){
  MidiBuffer midi;if(!on){midi.addEvent(MidiMessage::noteOn(1,60,.8f),0);on=true;}
  if(!off&&double(at)>=kHold*kRate){midi.addEvent(MidiMessage::noteOff(1,60),0);off=true;}
  processor.processBlock(audio,midi);
  for(int i=0;i<block&&at+i<total;++i)mono[size_t(at+i)]=.5f*(audio.getSample(0,i)+audio.getSample(1,i));
 }
 return match::analyse(mono,kRate,kHold);
}
match::Features featuresOf(const var& preset){
 const var f=preset["features"];match::Features out;out.rmsDb=float(double(f["rmsDb"]));out.peakDb=float(double(f["peakDb"]));out.attackMs=float(double(f["attackMs"]));out.releaseDb=float(double(f["releaseDb"]));
 if(auto* bands=f["band"].getArray())for(int i=0;i<match::kBands&&i<bands->size();++i)out.band[size_t(i)]=float(double((*bands)[i]));
 return out;
}
String statusFor(float score){return score<=4.f?"close":score<=9.f?"approximate":"different";}

// ---------------------------------------------------------------------------------------------------------------------
// Synth A (EMBER)
struct EmberMapper{
 const var& src;std::map<String,double> out;std::vector<std::pair<String,String>> routes;Array<var> mods;std::vector<String>& notes;
 explicit EmberMapper(const var& source,std::vector<String>& n):src(source),notes(n){}
 double claude(const String& id)const{return num(src["params"],"sa_"+id);}
 int claudeChoice(const String& id)const{return int(claude(id)+.5);}
 const SynthA::Param* param(const String& id)const{const int i=SynthA::indexOf(id.toStdString());return i<0?nullptr:&SynthA::parameters()[size_t(i)];}
 double base(const String& id)const{auto found=out.find(id);if(found!=out.end())return found->second;auto* p=param(id);return p?p->initial:0;}
 void set(const String& id,double value){auto* p=param(id);if(p==nullptr){notes.push_back("internal: unknown Synth A parameter "+id);return;}out[id]=p->limit(float(value));}
 void setChoice(const String& id,const String& label){auto* p=param(id);if(p==nullptr)return;for(size_t i=0;i<p->choices.size();++i)if(lower(String(p->choices[i]))==lower(label)){out[id]=double(i);return;}notes.push_back("no choice '"+label+"' in "+id);}
 double unitDepth(const String& id,double nativeDelta)const{auto* p=param(id);if(p==nullptr)return 0;const double b=base(id);return .5*(p->normalise(float(b+nativeDelta))-p->normalise(float(b-nativeDelta)));}
 double octaveDepth(const String& id,double octaves)const{auto* p=param(id);if(p==nullptr)return 0;const double b=base(id);return .5*(p->normalise(float(b*std::pow(2.0,octaves)))-p->normalise(float(b*std::pow(2.0,-octaves))));}
};

var convertEmber(const var& preset,std::vector<String>& notes){
 EmberMapper m(preset,notes);
 const String divisionNames[8]={"","1 bar","1/2","1/4","1/8","1/16","1/8 T","1/16 T"};
 // oscillators
 const double flavor=m.claude("flavor");
 for(int n=1;n<=3;++n){const String o="osc"+String(n)+".",c="o"+String(n)+"_";
  m.set(o+"wave",m.claude(c+"wave"));m.set(o+"octave",m.claude(c+"oct"));m.set(o+"coarse",m.claude(c+"semi"));m.set(o+"fine",m.claude(c+"fine"));
  if(n<3)m.set(o+"pw",m.claude(c+"pw"));else m.set(o+"pw",.5);
  m.set(o+"level",m.claude(c+"lvl"));m.set(o+"flavor",flavor);
  m.set(o+"uni.voices",m.claude("uni_voices"));m.set(o+"uni.detune",m.claude("uni_detune"));m.set(o+"uni.spread",m.claude("uni_spread"));m.set(o+"uni.blend",m.claude("uni_blend"));
 }
 notes.push_back("unison: the Claude preset has one global unison; applied to all three oscillators");
 m.set("osc2.sync",m.claude("o2_sync"));m.set("osc1.fm",m.claude("o2_fm"));m.set("mix.ring",m.claude("o2_ring"));
 m.set("mix.sub",m.claude("sub_lvl"));m.set("mix.sub.wave",m.claude("sub_wave"));m.set("mix.sub.octave",m.claude("sub_oct"));m.set("mix.noise",m.claude("noise_lvl"));m.set("mix.noise.type",m.claude("noise_col"));
 m.set("macro.warmth",m.claude("warmth"));m.set("analog.age",m.claude("age"));
 // filters. The Claude Moog ladder is always low-pass whatever the type says, so it maps to LP.
 for(int n=1;n<=2;++n){const String f="flt"+String(n)+".",c="f"+String(n)+"_";const int model=m.claudeChoice(c+"model");
  m.set(f+"model",model);m.set(f+"mode",model==0?0.0:m.claude(c+"type"));if(model==0&&m.claudeChoice(c+"type")!=0)notes.push_back("filter "+String(n)+": Claude ladder ignored its HP/BP/Notch type (always low-pass); mapped to low-pass");
  m.set(f+"cutoff",m.claude(c+"cut"));m.set(f+"res",m.claude(c+"res"));m.set(f+"drive",m.claude(c+"drive"));m.set(f+"key",m.claude(c+"key"));
  m.set(f+"env",m.claude(c+"env")*5.0/6.0);          // Claude: env x 5 octaves, ChatGPT: env x 6 octaves
  m.set(f+"velocity",m.claude(c+"vel")*2.0/3.0);     // Claude: (vel-.5) x 2 octaves, ChatGPT: (vel-.5) x 3 octaves
  const bool open=n==2&&m.claude("f2_cut")>19000&&m.claude("f2_drive")<.01&&std::abs(m.claude("f2_env"))<.01;m.set(f+"on",n==1?1.0:(open?0.0:1.0));}
 m.set("filter.routing",m.claude("f_route"));
 for(int n=1;n<=3;++n){const String e="env"+String(n)+".",c="e"+String(n)+"_";m.set(e+"attack",m.claude(c+"a"));m.set(e+"decay",m.claude(c+"d"));m.set(e+"sustain",m.claude(c+"s"));m.set(e+"release",m.claude(c+"r"));}
 for(int n=1;n<=3;++n){const String l="lfo"+String(n)+".",c="l"+String(n)+"_";m.set(l+"shape",m.claude(c+"shape"));m.set(l+"rate",m.claude(c+"rate"));
  const int sync=m.claudeChoice(c+"sync");m.set(l+"sync",sync>0?1.0:0.0);if(sync>0)m.setChoice(l+"division",divisionNames[sync]);}
 for(int n=1;n<=4;++n)m.set("macro."+String(n),m.claude("mac"+String(n)));
 m.set("voice.mode",m.claude("play_mode"));m.set("voice.glide",m.claude("glide"));m.set("voice.bend",m.claude("bend_range"));m.set("chord.on",m.claude("chord_on"));
 if(m.claudeChoice("chord_on")==1)notes.push_back("chord stab: the learned chord is part of a Claude session, not of the preset, so the default chord intervals are used");
 // arpeggiator and trance gate
 const String arpRates[5]={"1/4","1/8","1/16","1/16 T","1/32"},gateRates[3]={"1/8","1/16","1/32"};
 m.set("arp.on",m.claude("arp_on"));m.set("arp.mode",m.claude("arp_mode"));m.setChoice("arp.rate",arpRates[std::clamp(m.claudeChoice("arp_rate"),0,4)]);m.set("arp.octaves",m.claude("arp_oct"));m.set("arp.gate",m.claude("arp_gate"));m.set("arp.swing",m.claude("arp_swing"));m.set("arp.hold",m.claude("arp_hold"));
 m.set("seq.gate.on",m.claude("gate_on"));m.setChoice("seq.gate.rate",gateRates[std::clamp(m.claudeChoice("gate_rate"),0,2)]);m.set("seq.gate.depth",m.claude("gate_depth"));m.set("seq.gate.smooth",.001+m.claude("gate_smooth")*.099);
 for(int s=1;s<=16;++s){m.set("seq.gate.step"+String(s),m.claude("gs"+String(s)));m.set("seq.gate.tie"+String(s),0);}
 // effects
 for(auto* module:{"drive","chorus","flanger","phaser","delay","reverb","eq","pump"})m.set(String("fx.")+module+".on",0);
 if(m.claudeChoice("fx_drv_on")==1){m.set("fx.drive.on",1);m.set("fx.drive.mode",m.claude("fx_drv_type"));m.set("fx.drive.amount",m.claude("fx_drv_amt"));m.set("fx.drive.mix",1);}
 if(m.claudeChoice("fx_ch_on")==1){const String k=m.claudeChoice("fx_ch_mode")==1?"flanger":"chorus";m.set("fx."+k+".on",1);m.set("fx."+k+".rate",m.claude("fx_ch_rate"));m.set("fx."+k+".depth",m.claude("fx_ch_depth"));m.set("fx."+k+".mix",m.claude("fx_ch_mix"));}
 if(m.claudeChoice("fx_ph_on")==1){m.set("fx.phaser.on",1);m.set("fx.phaser.rate",m.claude("fx_ph_rate"));m.set("fx.phaser.depth",m.claude("fx_ph_depth"));m.set("fx.phaser.feedback",m.claude("fx_ph_fb"));m.set("fx.phaser.mix",m.claude("fx_ph_mix"));}
 if(m.claudeChoice("fx_dl_on")==1){const String divisions[6]={"1/16","1/8","1/8 D","1/4","","1/2"};const int t=m.claudeChoice("fx_dl_time");
  m.set("fx.delay.on",1);if(t==4){m.set("fx.delay.sync",0);m.set("fx.delay.time",60.0/138.0*1.5);notes.push_back("delay: dotted quarter has no tempo-sync division here; mapped to a fixed time of 652 ms (138 BPM)");}else{m.set("fx.delay.sync",1);m.setChoice("fx.delay.division",divisions[t]);}
  m.set("fx.delay.feedback",m.claude("fx_dl_fb"));m.set("fx.delay.tone",200.0*std::pow(80.0,m.claude("fx_dl_tone")));m.set("fx.delay.pingpong",m.claude("fx_dl_pp"));m.set("fx.delay.mix",m.claude("fx_dl_mix"));}
 if(m.claudeChoice("fx_rv_on")==1){m.set("fx.reverb.on",1);m.set("fx.reverb.size",m.claude("fx_rv_size"));m.set("fx.reverb.damp",m.claude("fx_rv_damp"));m.set("fx.reverb.mix",m.claude("fx_rv_mix"));}
 if(m.claudeChoice("fx_eq_on")==1){m.set("fx.eq.on",1);m.set("fx.eq.low",m.claude("fx_eq_lo"));m.set("fx.eq.high",m.claude("fx_eq_hi"));m.set("fx.eq.mix",1);}
 if(m.claudeChoice("fx_pump_on")==1){m.set("fx.pump.on",1);m.set("fx.pump.depth",m.claude("fx_pump_depth"));m.setChoice("fx.pump.rate",m.claudeChoice("fx_pump_rate")==1?"1/8":"1/4");m.set("fx.pump.mix",1);notes.push_back("pump: Claude release time has no equivalent control; curve left at the ChatGPT default");}
 m.set("master.volume",-9);   // replaced by the level match below
 // modulation matrix: 8 Claude slots become up to 12 routes
 const String sourceNames[17]={"","LFO 1","LFO 2","LFO 3","Env 1","Env 2","Env 3","Velocity","Key","Mod wheel","Pressure","Random","Macro 1","Macro 2","Macro 3","Macro 4",""};
 Array<var> routes;
 auto addRoute=[&](const String& source,const String& destination,double depth){if(std::abs(depth)<1.e-4)return;if(routes.size()>=12){notes.push_back("modulation: more than 12 routes needed, extra routes dropped");return;}auto* r=new DynamicObject;r->setProperty("src",source);r->setProperty("dst",destination);r->setProperty("amount",depth);routes.add(var(r));};
 for(int s=1;s<=8;++s){const int so=m.claudeChoice("m"+String(s)+"_src"),ds=m.claudeChoice("m"+String(s)+"_dst");double amount=m.claude("m"+String(s)+"_amt");if(so==0||ds==0||std::abs(amount)<1.e-6)continue;
  if(so==16){notes.push_back("modulation slot "+String(s)+" dropped: the Pitch Bend source does not exist in Synth A (Claude values: source 16, destination "+String(ds)+", amount "+String(m.claude("m"+String(s)+"_amt"),3)+")");continue;}
  const String name=sourceNames[so];const double keyScale=so==8?1.25:1.0;   // Claude Key = (note-60)/48, ChatGPT Key = (note-60)/60
  amount*=keyScale;
  auto oscActive=[&](int n){return m.base("osc"+String(n)+".level")>.001;};
  switch(ds){
   case 1:for(int n=1;n<=3;++n)if(oscActive(n))addRoute(name,"osc"+String(n)+".fine",m.unitDepth("osc"+String(n)+".fine",2400*amount));if(std::abs(amount)*24>1)notes.push_back("modulation slot "+String(s)+": pitch range is limited to +-100 cents here (Claude allowed +-24 semitones)");break;
   case 2:addRoute(name,"osc1.fine",m.unitDepth("osc1.fine",2400*amount));if(std::abs(amount)*24>1)notes.push_back("modulation slot "+String(s)+": pitch range limited to +-100 cents");break;
   case 3:addRoute(name,"osc2.fine",m.unitDepth("osc2.fine",2400*amount));if(std::abs(amount)*24>1)notes.push_back("modulation slot "+String(s)+": pitch range limited to +-100 cents");break;
   case 4:addRoute(name,"osc1.pw",m.unitDepth("osc1.pw",.45*amount));break;
   case 5:addRoute(name,"osc2.pw",m.unitDepth("osc2.pw",.45*amount));break;
   case 6:addRoute(name,"flt1.cutoff",m.octaveDepth("flt1.cutoff",4*amount));break;
   case 7:addRoute(name,"flt1.res",m.unitDepth("flt1.res",.5*amount));break;
   case 8:addRoute(name,"flt2.cutoff",m.octaveDepth("flt2.cutoff",4*amount));break;
   case 9:addRoute(name,"flt2.res",m.unitDepth("flt2.res",.5*amount));break;
   case 10:addRoute(name,"flt1.drive",m.unitDepth("flt1.drive",.5*amount));addRoute(name,"flt2.drive",m.unitDepth("flt2.drive",.5*amount));break;
   case 11:notes.push_back("modulation slot "+String(s)+" dropped: amplitude modulation has no equivalent destination in Synth A (Claude values: source "+String(so)+", destination Amp, amount "+String(amount,3)+")");break;
   case 12:addRoute(name,"master.pan",m.unitDepth("master.pan",amount));break;
   case 13:for(int n=1;n<=3;++n)addRoute(name,"osc"+String(n)+".uni.detune",m.unitDepth("osc"+String(n)+".uni.detune",.5*amount));break;
   case 14:addRoute(name,"osc1.level",m.unitDepth("osc1.level",amount));break;
   case 15:addRoute(name,"osc2.level",m.unitDepth("osc2.level",amount));break;
   case 16:addRoute(name,"osc3.level",m.unitDepth("osc3.level",amount));break;
   case 17:addRoute(name,"mix.sub",m.unitDepth("mix.sub",amount));break;
   case 18:addRoute(name,"mix.noise",m.unitDepth("mix.noise",amount));break;
   case 19:addRoute(name,"osc1.fm",m.unitDepth("osc1.fm",.5*amount));break;
   case 20:addRoute(name,"macro.warmth",m.unitDepth("macro.warmth",.5*amount));break;
   case 21:addRoute(name,"lfo1.rate",m.unitDepth("lfo1.rate",m.base("lfo1.rate")*(std::pow(2.0,3*amount)-1)));break;
   case 22:addRoute(name,"lfo2.rate",m.unitDepth("lfo2.rate",m.base("lfo2.rate")*(std::pow(2.0,3*amount)-1)));break;
   default:break;
  }
 }
 auto* patch=new DynamicObject;patch->setProperty("format","dualsynth-patch");patch->setProperty("version",1);patch->setProperty("engine","A");
 auto* params=new DynamicObject;for(auto& entry:m.out)params->setProperty(Identifier(entry.first),entry.second);patch->setProperty("params",var(params));patch->setProperty("mod",routes);
 return var(patch);
}

// ---------------------------------------------------------------------------------------------------------------------
template<class Processor,class Load>
void finishPatch(var& patch,const String& volumeKey,const std::vector<String>& cutoffKeys,const var& claudePreset,const String& engine,const String& name,const String& category,int index,Load load,std::vector<Report>& reports,std::vector<String>& notes){
 Report report;report.engine=engine;report.name=name;report.category=category;report.index=index;
 const match::Features reference=featuresOf(claudePreset);
 auto* params=patch.getDynamicObject()->getProperty("params").getDynamicObject();
 std::map<String,double> baseCutoff;for(auto& key:cutoffKeys)if(params->hasProperty(Identifier(key)))baseCutoff[key]=double(params->getProperty(Identifier(key)));
 // One render at a given filter-cutoff shift: loudness is matched to the Claude render first, then the sound is measured.
 auto evaluate=[&](double shiftOctaves,double& trimOut,match::Features& featuresOut,Result& status){
  for(auto& entry:baseCutoff)params->setProperty(Identifier(entry.first),jlimit(20.0,20000.0,entry.second*std::pow(2.0,shiftOctaves)));
  double trim=-9;match::Distance distance;
  for(int pass=0;pass<2;++pass){
   params->setProperty(Identifier(volumeKey),trim);auto processor=std::make_unique<Processor>();status=load(*processor,patch);if(status.failed())return distance;
   featuresOut=renderFeatures(*processor);distance=match::compare(reference,featuresOut);if(pass==0)trim=jlimit(-60.0,0.0,trim+double(reference.rmsDb-featuresOut.rmsDb));
  }
  trimOut=trim;return distance;
 };
 // The two engines filter differently (slopes, resonance, drive), so the same cutoff does not sound the same. Try cutoffs within
 // one octave either way and keep the one whose spectrum is closest to the Claude render (shift 0 wins ties).
 double bestShift=0,bestTrim=-9;match::Features bestFeatures;match::Distance best;Result status=Result::ok();
 {double trim=0;match::Features features;best=evaluate(0.0,trim,features,status);bestTrim=trim;bestFeatures=features;
  if(status.wasOk()&&!baseCutoff.empty())for(double shift:{-1.0,-.75,-.5,-.25,.25,.5,.75,1.0}){Result s=Result::ok();match::Features f;double t=0;const auto d=evaluate(shift,t,f,s);if(s.wasOk()&&d.score<best.score-.25f){best=d;bestShift=shift;bestTrim=t;bestFeatures=f;}}}
 if(status.failed()){notes.push_back("the converted patch was rejected: "+status.getErrorMessage());}
 evaluate(bestShift,bestTrim,bestFeatures,status);   // leave the patch holding the winning settings
 if(bestShift!=0)notes.push_back("filter cutoff moved "+String(bestShift,2)+" octaves (both filters) so the spectrum is closer to the Claude render; the engines' filters differ");
 if(std::getenv("CLAUDE_IMPORT_DEBUG")!=nullptr&&index<3){std::printf("DEBUG %s %s: rms %.1f vs %.1f dB, attack %.0f vs %.0f ms, release %.1f vs %.1f dB, cutoff shift %.2f, score %.1f\n  claude bands:",engine.toRawUTF8(),name.toRawUTF8(),reference.rmsDb,bestFeatures.rmsDb,reference.attackMs,bestFeatures.attackMs,reference.releaseDb,bestFeatures.releaseDb,bestShift,best.score);
  for(float b:reference.band)std::printf(" %.0f",b);std::printf("\n  chatgpt bands:");for(float b:bestFeatures.band)std::printf(" %.0f",b);std::printf("\n");}
 report.gainTrimDb=float(bestTrim);report.distance=best;report.notes=notes;report.status=statusFor(best.score);
 if(bestFeatures.rmsDb<-80.f)report.status="silent";
 const String mark=report.status=="close"?"[C]":(report.status=="approximate"?"[C~]":"[C?]");
 auto* object=patch.getDynamicObject();object->setProperty("name",name+" "+mark);object->setProperty("category","Claude "+category);
 auto* info=new DynamicObject;info->setProperty("mapVersion",kMapVersion);info->setProperty("source","Claude baseline factory preset "+String(index)+": "+name);info->setProperty("status",report.status);
 info->setProperty("spectrumDb",best.spectrumDb);info->setProperty("levelDb",best.levelDb);info->setProperty("attackMs",best.attackMs);info->setProperty("releaseDb",best.releaseDb);info->setProperty("score",best.score);
 Array<var> noteList;for(auto& n:notes)noteList.add(n);info->setProperty("notes",noteList);object->setProperty("claudeImport",var(info));
 reports.push_back(report);
}

}
int main(int argc,char** argv){
 ScopedJuceInitialiser_GUI gui;
 if(argc<3){std::fprintf(stderr,"usage: DualClaudeImport <claude-export.json> <output folder>\n");return 2;}
 const auto input=File::getCurrentWorkingDirectory().getChildFile(argv[1]);const auto outputFolder=File::getCurrentWorkingDirectory().getChildFile(argv[2]);outputFolder.createDirectory();
 var data;if(JSON::parse(input.loadFileAsString(),data).failed()||String(data["format"].toString())!="claude-export"){std::fprintf(stderr,"not a claude-export file\n");return 1;}
 std::vector<Report> reports;Array<var> emberBank;
 // ---- EMBER: convert, level-match, calibrate the filter cutoff, measure
 {auto* list=data["ember"]["presets"].getArray();
  for(int i=1;i<list->size();++i){const auto& preset=(*list)[i];std::vector<String> notes;var patch=convertEmber(preset,notes);
   finishPatch<SynthAProcessor>(patch,"master.volume",std::vector<String>{"flt1.cutoff","flt2.cutoff"},preset,"EMBER",preset["name"].toString(),preset["category"].toString(),i,[](SynthAProcessor& p,const var& v){return p.setPatch(v);},reports,notes);
   emberBank.add(patch);std::printf("EMBER %d/%d %s -> %s (%.1f)\n",i+1,list->size(),preset["name"].toString().toRawUTF8(),reports.back().status.toRawUTF8(),reports.back().distance.score);}}
 outputFolder.getChildFile("claude-import-ember.json").replaceWithText(JSON::toString(var(emberBank),false));
 // ---- PRISM: the bank is ChatGPT's own import (Engines/B/Presets/claude-prism.json). It is only measured here, never rewritten.
 {PrismProcessor probe;auto* list=data["prism"]["presets"].getArray();const int start=probe.factoryCount()-list->size();
  if(start<0)std::fprintf(stderr,"PRISM bank is smaller than the Claude export; skipping its evaluation\n");
  else for(int i=0;i<list->size();++i){const auto& preset=(*list)[i];var patch=probe.factoryPatch(start+i);std::vector<String> notes;
   finishPatch<PrismProcessor>(patch,"master.gain",std::vector<String>{},preset,"PRISM (ChatGPT bank)",preset["name"].toString(),preset["category"].toString(),i,[](PrismProcessor& p,const var& v){return p.restore(v);},reports,notes);
   std::printf("PRISM %d/%d %s -> %s (%.1f)\n",i+1,list->size(),preset["name"].toString().toRawUTF8(),reports.back().status.toRawUTF8(),reports.back().distance.score);}}
 // ---- report
 std::map<String,int> counts;for(auto& r:reports)counts[r.engine+" "+r.status]++;
 String md="# Claude preset import report (map version "+String(kMapVersion)+")\n\n"
  "Generated by `Tools/ClaudeImport`. **Nothing here was listened to.** Status is an objective measurement: each preset was rendered (C3, velocity 0.8, 1 s hold) by the Claude engine and by the ChatGPT engine from the converted patch, after matching loudness (master level only), and the two were compared on a 31-band spectrum, loudness, attack time and release. "
  "`close` = score <= 4, `approximate` <= 9, `different` > 9 (score = spectrum dB + 0.5 x level dB + 0.02 x attack ms + 0.2 x release dB). The two engines are different DSP, so even `close` means close, not identical; Arash's ear decides.\n\n"
  "- **EMBER** rows: presets converted by this tool (the Claude Init patch is not imported). The tool also tried the filter cutoff up to one octave either way and kept the closest spectrum; each such move is in the preset's notes.\n"
  "- **PRISM (ChatGPT bank)** rows: ChatGPT's own mapping of the Claude PRISM presets, measured here with the same method and only the master level matched. The tool did not change that bank.\n\n## Totals\n";
 for(auto& c:counts)md+="- "+c.first+": "+String(c.second)+"\n";
 md+="\n## Presets\n";
 for(const char* engine:{"EMBER","PRISM (ChatGPT bank)"}){md+="\n### "+String(engine)+"\n\n| # | preset | category | status | score | spectrum dB | level dB | attack ms | release dB | gain trim dB |\n|---|---|---|---|---|---|---|---|---|---|\n";
  for(auto& r:reports)if(r.engine==engine)md+="| "+String(r.index)+" | "+r.name+" | "+r.category+" | "+r.status+" | "+String(r.distance.score,1)+" | "+String(r.distance.spectrumDb,1)+" | "+String(r.distance.levelDb,1)+" | "+String(r.distance.attackMs,0)+" | "+String(r.distance.releaseDb,1)+" | "+String(r.gainTrimDb,1)+" |\n";}
 md+="\n## Per-preset notes for EMBER (settings that could not be carried over exactly)\n";
 for(auto& r:reports){if(r.engine!="EMBER")continue;StringArray unique;for(auto& n:r.notes)if(!unique.contains(n))unique.add(n);if(unique.isEmpty())continue;md+="\n**"+r.name+"**\n";for(auto& n:unique)md+="- "+n+"\n";}
 outputFolder.getChildFile("claude-import-report.md").replaceWithText(md);
 std::printf("done: %d presets measured\n",int(reports.size()));
 return 0;
}
