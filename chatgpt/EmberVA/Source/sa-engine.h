// sa-engine.h — Synth A's portable C++ instrument API. No GUI or host dependencies.
#pragma once
#include "sa-effects.h"
#include "sa-sequencer.h"
#include <atomic>
namespace SynthA {
struct MidiEvent {
 enum Type { NoteOn,NoteOff,PitchBend,ControlChange,Aftertouch,Panic } type=NoteOn;
 int sample=0,note=60,channel=1;float value=0;
};
class Engine {
 public:
    Engine(); void prepare(double sampleRate); void reset();
    void setParameters(const Values&); const Values& getParameters() const {return target;}
    void render(float* left,float* right,int samples,const MidiEvent* events,int eventCount,double hostBpm=0,double ppq=-1);
    void midi(const MidiEvent&); void allNotesOff(); void panic();
    int activeVoices() const; double getBeat() const {return beat;}
    std::array<std::atomic<float>,512> scope{};std::atomic<unsigned> scopeWrite{0};std::atomic<float> peak{0};
 private:
    struct OscIds {int wave,flavor,octave,coarse,fine,pw,level,phase,free,voices,detune,spread,blend;};
    struct FilterIds {int on,model,mode,cutoff,res,drive,key,env,velocity,fm;};
    struct EnvIds {int a,d,s,r,curve;};struct LfoIds{int shape,rate,sync,division,retrigger,fade,polarity;};
    std::array<OscIds,3> oi{};std::array<FilterIds,2> fi{};std::array<EnvIds,3> ei{};std::array<LfoIds,3> li{};
    std::array<std::array<int,3>,12> mi{};
    int syncId,fmId,ringId,subId,subWaveId,subOctId,noiseId,noiseTypeId,ageId,warmId,satId,routeId,balanceId;
    int modeId,glideId,bendId,velocityId,chordId,bpmId,hostId,volumeId,panId,arpId;
    std::array<int,4> macroIds{},chordIds{};
    struct Voice {
      bool active=false,down=false,sustained=false;int note=60,parent=60,channel=1;uint64_t age=0;
      float velocity=0,pitch=60,targetPitch=60,tolerance=0,drift=0,driftTarget=0,subPhase=0,pink=0;
      float lastL=0,lastR=0,tailL=0,tailR=0,fade=0,time=0,randomValue=0,warmLP[2]{};
      std::array<std::array<float,8>,3> phase{},delta{},gainL{},gainR{};
      std::array<int,3> count{};std::array<float,3> lfoPhase{},lfoHold{},lfoPrev{},lfoOut{},envOut{};
      std::array<Envelope,3> env{};std::array<std::array<Filter,2>,2> filter{};std::array<DCBlock,2> dc{};
      Values p{};Random random;int driftCounter=0;
    };
    std::array<Voice,maxVoices> voices{};
    std::array<float,16> bend{},wheel{},pressure{};std::array<bool,16> sustain{};
    std::array<std::array<float,128>,16> held{};std::array<std::array<uint64_t,128>,16> heldOrder{};
    Values target{},smooth{},global{};Effects effects;Sequencer sequencer;std::array<NoteEvent,256> seqEvents{};
    Downsample downL,downR;DCBlock outputDC[2];
    float sr=48000,osr=96000;double beat=0;uint64_t counter=0,noteAge=0;int controlCounter=0,lastMode=0;bool lastArp=false;
    void trigger(int note,float velocity,int channel,int parent,bool legato=false);
    void release(int note,int channel);void directNote(int note,float velocity,int channel,bool on);
    void updateVoice(Voice&,float bpm);void renderVoice(Voice&,float& l,float& r,float bpm);
    void applyMods(Values& dest,const Voice&);
    void controlTick(float bpm);
};
}
