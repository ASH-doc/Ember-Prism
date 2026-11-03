// sb-tables.cpp — Six original spectral families; every frame has octave band-limited copies.
#include "sb-tables.h"
#include <stdexcept>
namespace SynthB {
void transform(std::vector<std::complex<float>>& a,bool inverse){
 const size_t n=a.size();for(size_t i=1,j=0;i<n;++i){size_t bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
 for(size_t length=2;length<=n;length<<=1){const float angle=float(tau/double(length))*(inverse?1.f:-1.f);const std::complex<float> step(std::cos(angle),std::sin(angle));for(size_t base=0;base<n;base+=length){std::complex<float> w(1,0);for(size_t j=0;j<length/2;++j){auto x=a[base+j],y=a[base+j+length/2]*w;a[base+j]=x+y;a[base+j+length/2]=x-y;w*=step;}}}
 if(inverse)for(auto& v:a)v/=float(n);
}
static float cubic(const float* frame,int length,float phase){
 float x=(phase-std::floor(phase))*float(length);int i=int(x);float f=x-float(i);const int mask=length-1;
 float a=frame[(i-1)&mask],b=frame[i&mask],c=frame[(i+1)&mask],d=frame[(i+2)&mask];
 return b+.5f*f*(c-a+f*(2*a-5*b+4*c-d+f*(3*(b-c)+d-a)));
}
float WaveSet::sample(float phase,float position,float mip)const{
 position=bounded(position,0.f,1.f)*float(frames-1);const int f0=int(position),f1=std::min(frames-1,f0+1);float fraction=position-float(f0);
 mip=bounded(mip,0.f,float(mipCount-1));int m=int(mip),next=std::min(m+1,mipCount-1);
 auto read=[&](int layer){const auto& l=layers[size_t(layer)];float a=cubic(l.samples.data()+size_t(f0*l.length),l.length,phase),b=cubic(l.samples.data()+size_t(f1*l.length),l.length,phase);return a+(b-a)*fraction;};
 const float a=read(m);return a+(read(next)-a)*(mip-float(m));
}
float WaveSet::visual(float phase,float position)const{return sample(phase,position,0);}
std::unique_ptr<WaveSet> WaveSet::fromFrames(const std::vector<float>& input,int frameCount,std::string name){
 if(frameCount<1||frameCount>64||input.size()!=size_t(frameCount*frameLength))throw std::invalid_argument("Expected 1–64 frames of 2048 samples");
 auto set=std::make_unique<WaveSet>();set->frames=frameCount;set->title=std::move(name);
 for(int m=0;m<mipCount;++m){auto& layer=set->layers[size_t(m)];layer.length=std::max(64,frameLength>>m);layer.samples.resize(size_t(frameCount*layer.length));}
 std::vector<std::complex<float>> spectrum(frameLength);
 for(int frame=0;frame<frameCount;++frame){
  double mean=0;float peak=0;for(int i=0;i<frameLength;++i){float x=input[size_t(frame*frameLength+i)];if(!std::isfinite(x))throw std::invalid_argument("Non-finite waveform");mean+=x;}
  mean/=frameLength;for(int i=0;i<frameLength;++i){float x=input[size_t(frame*frameLength+i)]-float(mean);spectrum[size_t(i)]=x;peak=std::max(peak,std::abs(x));}
  if(peak>0)for(auto& s:spectrum)s*=.95f/peak;transform(spectrum,false);spectrum[0]=0;
  for(int m=0;m<mipCount;++m){auto& layer=set->layers[size_t(m)];const int length=layer.length,maxH=std::max(1,1023>>m);std::vector<std::complex<float>> bins(size_t(length),{0,0});
   for(int h=1;h<=maxH;++h){float taper=h>maxH*.85f?.5f+.5f*std::cos(float(3.141592653589793)*(float(h)/float(maxH)-.85f)/.15f):1;bins[size_t(h)]=spectrum[size_t(h)]*(float(length)/frameLength*taper);bins[size_t(length-h)]=std::conj(bins[size_t(h)]);}
   // Retain the fundamental in the highest octave instead of tapering it to zero.
   if(maxH==1){bins[1]=spectrum[1]*(float(length)/frameLength);bins[size_t(length-1)]=std::conj(bins[1]);}
   transform(bins,true);for(int i=0;i<length;++i)layer.samples[size_t(frame*length+i)]=bins[size_t(i)].real();
  }
 }
 return set;
}
Tables::Tables(){
 for(int table=0;table<30;++table){const int family=table/5,variant=table%5;std::vector<float> frames(size_t(64*WaveSet::frameLength));Entropy rng;rng.bits+=uint64_t(table+1)*1717986919ULL;
  for(int f=0;f<64;++f){const float position=float(f)/63;std::vector<std::complex<float>> spectrum(WaveSet::frameLength);
   for(int h=1;h<768;++h){const float harmonic=float(h);float amplitude=0,phase=0;
    switch(family){
     case 0:{const float odd=h%2?1.f:0.f;amplitude=(1-position+position*odd)/std::pow(harmonic,1.f+.5f*float(variant)/4);amplitude*=.6f+.4f*std::cos(harmonic*position*(.2f+.1f*variant));phase=-float(tau)*.25f;break;}
     case 1:amplitude=(.15f+.85f*std::pow(.5f+.5f*std::cos(harmonic*(.24f+.08f*variant)+position*float(tau)*2),4))/std::pow(harmonic,.8f+.1f*variant);phase=.3f*std::sin(harmonic*.17f)*position;break;
     case 2:{float center=3+position*(12+variant*3);float formant=std::exp(-std::pow((harmonic-center)/(1.3f+variant*.4f),2.f))+.5f*std::exp(-std::pow((harmonic-center*2.3f)/3,2.f));amplitude=(.18f/harmonic+formant/std::sqrt(harmonic));phase=.04f*harmonic;break;}
     case 3:amplitude=(.25f+.75f*std::pow(.5f+.5f*std::sin(harmonic*.7f+position*float(tau)*(variant+1)),2))/std::pow(harmonic,1.1f);phase=.7f*std::sin(harmonic*.11f+position*float(tau));break;
     case 4:{float cluster=2+variant+position*15;amplitude=(h==1?.4f:0)+std::exp(-std::pow((harmonic-cluster)/(1+position*3),2.f))+.55f*std::exp(-std::pow((harmonic-cluster*2)/(2+variant),2.f));amplitude/=std::sqrt(harmonic);phase=(h%3)*position*.8f;break;}
     default:amplitude=(.05f+.95f*std::abs(rng.next()))/std::pow(harmonic,.5f+position*.8f);amplitude*=.2f+.8f*std::pow(std::sin(harmonic*.3f+position*3),2.f);phase=rng.next()*float(tau)*(.15f+position*.15f);break;
    }
    spectrum[size_t(h)]=std::polar(amplitude,phase);spectrum[size_t(WaveSet::frameLength-h)]=std::conj(spectrum[size_t(h)]);
   }
   spectrum[1]+=std::complex<float>(.4f,0);spectrum[WaveSet::frameLength-1]=std::conj(spectrum[1]);transform(spectrum,true);
   for(int i=0;i<WaveSet::frameLength;++i)frames[size_t(f*WaveSet::frameLength+i)]=spectrum[size_t(i)].real();
  }
  library[size_t(table)]=WaveSet::fromFrames(frames,64,tableNames()[size_t(table)]);
 }
}
const Tables& Tables::get(){static Tables t;return t;}
const WaveSet& Tables::at(int index,const WaveSet* imported)const{if(index==30&&imported)return *imported;return *library[size_t(bounded(index,0,29))];}
float warpedPhase(float phase,int mode,float amount,float modulation){
 switch(mode){case 1:phase*=1+amount*7;break;case 2:phase=std::pow(phase,1+amount*4);break;case 3:phase=1-std::pow(1-phase,1+amount*4);break;case 4:phase+=amount*((phase<.5f?phase*2:2-phase*2)-phase);break;case 5:{float pivot=.5f+amount*.44f;phase=phase<pivot?phase*.5f/pivot:.5f+(phase-pivot)*.5f/(1-pivot);break;}case 6:phase+=modulation*amount*.3f;break;case 9:{float width=.5f-amount*.45f;phase=phase<width?phase*.5f/width:.5f+(phase-width)*.5f/(1-width);break;}case 11:phase=phase*(1-amount)+(1-phase)*amount;break;default:break;}
 return phase-std::floor(phase);
}
float warpedAmplitude(float x,int mode,float amount,float modulator){if(mode==7)return x*(1-amount+amount*(modulator*.5f+.5f));if(mode==8)return x*(1-amount+amount*modulator);if(mode==10){float steps=std::pow(2.f,14-amount*12);return std::round(x*steps)/steps;}return x;}
float warpBandwidth(int mode,float amount){switch(mode){case 1:return 1+amount*7;case 2:case 3:return 1+amount*4;case 5:case 9:return 1/(1-amount*.9f);case 6:return 1+amount*8;default:return 1;}}
}
