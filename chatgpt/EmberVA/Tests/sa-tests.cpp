// sa-tests.cpp — Offline engine regression and realtime-cost measurements.
#include "sa-engine.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdlib>
using namespace SynthA;
static void require(bool v,const char* text){if(!v){std::cerr<<"FAIL: "<<text<<'\n';std::exit(1);}}
static void set(Values& p,const char* id,float v){auto i=indexOf(id);require(i>=0,id);p[size_t(i)]=v;}
static double render(Engine& e,int blocks,int notes=0,bool stress=false){
 std::array<float,256> l{},r{};double energy=0;std::array<MidiEvent,32> events{};
 for(int n=0;n<notes;++n)events[n]={MidiEvent::NoteOn,0,48+n,1,.8f};
 for(int b=0;b<blocks;++b){e.render(l.data(),r.data(),256,b==0?events.data():nullptr,b==0?notes:0);
 for(int n=0;n<256;++n){require(std::isfinite(l[n])&&std::isfinite(r[n]),"finite output");require(std::abs(l[n])<.65f&&std::abs(r[n])<.65f,"bounded output");energy+=l[n]*l[n]+r[n]*r[n];}}
 if(stress)require(energy>0,"stress produces audio");return energy;
}
int main(int argc,char** argv){
 if(argc>1&&std::string(argv[1])=="--schema") {std::cout<<"[\n";bool first=true;for(auto& p:parameters()){if(!first)std::cout<<",\n";first=false;std::cout<<"{\"id\":\""<<p.id<<"\",\"min\":"<<p.min<<",\"max\":"<<p.max<<",\"default\":"<<p.initial<<",\"group\":\""<<p.group<<"\"}";}std::cout<<"\n]\n";return 0;}
 Engine e; e.prepare(48000);auto p=defaults();
 require(parameters().size()<maxParams,"parameter capacity");
 for(size_t i=0;i<parameters().size();++i)require(indexOf(parameters()[i].id)==int(i),"unique IDs");
 require(render(e,8)==0,"silence before first note");
 require(render(e,32,1)>1.e-5,"MIDI produces audio");
 e.midi({MidiEvent::NoteOff,0,48,1,0});render(e,300);require(e.activeVoices()==0,"release clears voices");
 e.reset();render(e,2,24);require(e.activeVoices()==16,"voice stealing stays at 16");e.panic();require(render(e,4)==0,"panic clears effects and voices");
 e.midi({MidiEvent::ControlChange,0,64,1,127});render(e,2,1);e.midi({MidiEvent::NoteOff,0,48,1,0});render(e,100);require(e.activeVoices()==1,"sustain retains voice");e.midi({MidiEvent::ControlChange,0,64,1,0});render(e,300);require(e.activeVoices()==0,"pedal release clears voice");
 for(int model=0;model<4;++model)for(int route=0;route<3;++route)for(int response=0;response<4;++response){p=defaults();set(p,"flt1.model",float(model));set(p,"flt1.mode",float(response));set(p,"filter.routing",float(route));set(p,"flt1.res",1);set(p,"flt1.drive",1);set(p,"macro.saturation",1);set(p,"flt2.on",1);set(p,"flt2.res",1);e.setParameters(p);e.reset();render(e,35,8,true);}
 p=defaults();set(p,"arp.on",1);set(p,"arp.hold",0);set(p,"arp.mode",5);e.setParameters(p);e.reset();render(e,50,8,true);for(int n=0;n<8;++n)e.midi({MidiEvent::NoteOff,0,48+n,1,0});render(e,400);require(e.activeVoices()==0,"arp releases all chord notes");
 p=defaults();set(p,"voice.mode",2);e.setParameters(p);e.reset();e.midi({MidiEvent::NoteOn,0,60,1,.8f});e.midi({MidiEvent::NoteOn,0,64,1,.8f});e.midi({MidiEvent::NoteOff,0,64,1,0});render(e,10);require(e.activeVoices()==1,"legato returns to held key");e.midi({MidiEvent::NoteOff,0,60,1,0});render(e,350);require(e.activeVoices()==0,"legato release");
 // Known oscillator frequency from zero crossings after the attack has settled.
 p=defaults();set(p,"osc1.wave",3);set(p,"osc2.level",0);set(p,"mix.sub",0);set(p,"flt1.on",0);set(p,"analog.age",0);set(p,"macro.warmth",0);set(p,"macro.saturation",0);set(p,"env1.sustain",1);e.setParameters(p);e.reset();e.midi({MidiEvent::NoteOn,0,69,1,1});render(e,20);
 std::array<float,48000> l{},r{};e.render(l.data(),r.data(),48000,nullptr,0);int crossings=0;for(int i=1;i<48000;++i)if(l[i-1]<=0&&l[i]>0)++crossings;require(std::abs(crossings-440)<=1,"A4 tuning");
 // All modulation destinations must remain bounded, even when matrix settings modulate themselves.
 p=defaults();set(p,"mod.1.source",8);set(p,"mod.1.amount",1);for(size_t d=0;d<parameters().size();++d){p[size_t(indexOf("mod.1.dest"))]=float(d+1);e.setParameters(p);e.reset();render(e,2,1);}
 std::cout<<"PASS: "<<parameters().size()<<" parameters; silence, voices, pedal, mono, arp, tuning, filters and matrix bounds.\n";
 for(int unison:{1,4,7}){p=defaults();for(int osc=1;osc<=3;++osc){set(p,("osc"+std::to_string(osc)+".uni.voices").c_str(),float(unison));set(p,("osc"+std::to_string(osc)+".level").c_str(),.4f);}e.setParameters(p);e.reset();auto start=std::chrono::steady_clock::now();render(e,188,16,true);double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::cout<<"16 voices x 3 oscillators x "<<unison<<" unison, 48k / 256: "<<seconds<<" s per 1.003 s audio ("<<seconds/1.003*100<<"% of one core offline)\n";}
}
