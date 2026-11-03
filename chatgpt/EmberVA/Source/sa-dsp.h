// sa-dsp.h — Band-limited sources, envelopes, TPT filters and oversampling decimator.
#pragma once
#include "sa-parameters.h"
#include <cstdint>
namespace SynthA {
inline float soft(float x) { x=clamp(x,-3.f,3.f); return x*(27+x*x)/(27+9*x*x); }
struct Random {
    uint32_t state=0x82a7f32du;
    float next() { state^=state<<13; state^=state>>17; state^=state<<5; return float(state>>8)*(1.f/8388608.f)-1.f; }
};
inline float blep(float t,float dt) {
    if(t<dt) { t/=dt; return t+t-t*t-1; }
    if(t>1-dt) { t=(t-1)/dt; return t*t+t+t+1; } return 0;
}
inline float wave(float phase,float dt,int type,float pw) {
    dt=clamp(dt,.0000001f,.45f);
    if(type==0 || type==4) return 2*phase-1-blep(phase,dt);
    if(type==1) { float t=phase-pw; if(t<0)t+=1; return (phase<pw?1.f:-1.f)+blep(phase,dt)-blep(t,dt); }
    if(type==3) return std::sin(float(2*pi)*phase);
    // Band-limited triangle: finite odd harmonic series, no discontinuous corners.
    float sum=0; const int maxH=std::min(15,int(.45f/dt));
    for(int h=1;h<=maxH;h+=2) sum+=((h&2)?-1.f:1.f)*std::sin(float(2*pi*h)*phase)/float(h*h);
    return .81056947f*sum;
}
struct Envelope {
    enum Stage { idle,attack,decay,sustain,release } stage=idle;
    float value=0, releaseStart=0, position=0;
    void on(bool retrigger=true) { if(retrigger) { stage=attack; position=0; } }
    void off() { if(stage!=idle && stage!=release) { stage=release; releaseStart=value; position=0; } }
    float tick(float a,float d,float s,float r,float curve,float sr) {
      switch(stage) {
        case attack: value+=1.f/(std::max(.001f,a)*sr); if(value>=1) { value=1;stage=decay;position=0; } break;
        case decay: position+=1.f/(std::max(.005f,d)*sr); value=s+(1-s)*std::pow(std::max(0.f,1-position),curve); if(position>=1) {value=s;stage=sustain;} break;
        case sustain: value+=.002f*(s-value); break;
        case release: position+=1.f/(std::max(.005f,r)*sr); value=releaseStart*std::pow(std::max(0.f,1-position),curve); if(position>=1 || value<1.e-7f) {value=0;stage=idle;} break;
        default: value=0;
      } return value;
    }
};
struct Filter {
    std::array<float,4> z{}; float s1=0,s2=0,g=.02f,targetG=.02f;
    void tune(float hz,float sr) { targetG=std::tan(float(pi)*clamp(hz,15.f,sr*.21f)/sr); }
    float tick(float x,int model,int mode,float resonance,float drive) {
      g+=.012f*(targetG-g);
      const float gain=1+drive*5;
      x=soft(x*gain)/(1+drive*2);
      if(model==0) {
        const float G=g/(1+g), G2=G*G, G3=G2*G, G4=G3*G;
        const float feedback=4.25f*resonance;
        const float S=(G3*z[0]+G2*z[1]+G*z[2]+z[3])/(1+g);
        float y=soft((x-feedback*S)/(1+feedback*G4)); const float input=y;
        std::array<float,4> stages{};int stage=0;
        for(auto& state:z) { const float v=(y-state)*G; y=v+state; state=y+v;stages[size_t(stage++)]=y; }
        const float high=input-4*stages[0]+6*stages[1]-4*stages[2]+stages[3];
        if(mode==1)y=high;else if(mode==2)y=4*(stages[1]-2*stages[2]+stages[3]);else if(mode==3)y+=high;
        return y*(1+resonance*.65f);
      }
      // Topology-preserving state-variable integrators; bounded nonlinear excitation for Snarl.
      float k=2-1.985f*resonance;
      if(model==2) { k=2-2.04f*resonance; x=soft(x*1.8f+.15f)-soft(.15f); }
      if(model==3) x=soft(x*1.2f);
      const float a1=1/(1+g*(g+k)), a2=g*a1, a3=g*a2;
      const float v3=x-s2, v1=a1*s1+a2*v3, v2=s2+a2*s1+a3*v3;
      s1=clamp(2*v1-s1,-8.f,8.f); s2=clamp(2*v2-s2,-8.f,8.f);
      float out=mode==1?x-k*v1-v2:mode==2?v1:mode==3?x-k*v1:v2;
      if(model==2) out=soft(out*1.2f);
      return out;
    }
};
struct DCBlock {
    float x=0,y=0;
    float tick(float a) { const float b=a-x+.9993f*y; x=a; y=b; return b; }
};
struct Downsample {
    // 63-tap Hann-windowed low-pass at 2x; protects output from above-Nyquist content.
    std::array<float,63> taps{},history{}; int head=0;
    Downsample() { float sum=0; for(int n=0;n<63;++n) { const int k=n-31; const float sinc=k==0?.46f:std::sin(float(pi)*.46f*k)/(float(pi)*k); taps[n]=sinc*(.5f-.5f*std::cos(float(2*pi)*n/62)); sum+=taps[n]; } for(auto& x:taps)x/=sum; }
    void push(float x) { history[head]=x; head=(head+1)%63; }
    float read() const { float y=0; int h=head; for(int n=0;n<63;++n) { if(--h<0)h=62; y+=history[h]*taps[n]; } return y; }
    void reset(){history.fill(0);head=0;}
};
}
