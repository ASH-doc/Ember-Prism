// sa-sequencer.h — Sample-clock arpeggiator and 16-step level/tie gate.
#pragma once
#include "sa-dsp.h"
namespace SynthA {
struct NoteEvent { int note=0,channel=1; float velocity=0; bool on=false; };
class Sequencer {
 public:
    Sequencer(); void reset(); void input(int note,float velocity,int channel,bool on);
    void control(int cc,int value,int channel); void update(const Values&);
    int tick(double beat,float bpm,float sr,std::array<NoteEvent,256>& out);
    float gate(double beat,float sr); void restartGate(double beat);
 private:
    struct Held { int note=0,channel=1;float velocity=0;bool down=false;uint64_t order=0; };
    std::array<Held,128> notes{};int count=0;uint64_t order=0;
    std::array<NoteEvent,128> sounding{};int soundingCount=0;
    std::array<bool,16> sustain{};
    int onId,modeId,octId,rateId,swingId,lengthId,holdId,gateOn,gateRate,gateDepth,gateSmooth,gateShape,gateRestart;
    std::array<int,16> velocities{},steps{},ties{};
    Values p{}; bool wasOn=false,wasHold=false; double nextBeat=-1,offBeat=-1,gateOrigin=0,lastBeat=-1;
    int step=0;float gateValue=1;Random random;
};
}
