// sb-filters.cpp — TPT multimode, comb, vowel, shelves and 2x reconstruction.
#include "sb-filters.h"
namespace SynthB {
void DigitalFilter::setup(float sampleRate){sr=sampleRate;comb.allocate(int(sr/15)+8);clear();}
void DigitalFilter::clear(){first={};second={};formants={};shelf=0;comb.clear();}
void DigitalFilter::coefficients(float hz,float resonance,float vowel){
 hz=bounded(hz,20.f,sr*.22f);target=std::tan(float(tau*.5)*hz/sr);q=2-1.96f*resonance;combLength=sr/hz;combFeedback=.94f*resonance;vowelPosition=vowel;
 constexpr float vowels[5][3]={{800,1150,2900},{400,1700,2600},{300,2200,3000},{500,850,2500},{350,700,2200}};
 float p=vowel*3.999f;int a=int(p),b=std::min(4,a+1);for(int i=0;i<3;++i){float f=vowels[a][i]+(vowels[b][i]-vowels[a][i])*(p-float(a));formantG[size_t(i)]=std::tan(float(tau*.5)*f/sr);}
}
float DigitalFilter::process(float input,int kind,float drive){
 if(drive>.0001f)input=std::tanh(input*(1+drive*8))/(1+drive*2);
 g+=(target-g)*.018f;const auto out=first.run(input,g,q);float result=out[0];
 switch(kind){case 1:result=second.run(out[0],g,q)[0];break;case 2:result=out[1];break;case 3:result=out[2];break;case 4:result=input-q*out[2];break;case 5:{float t=comb.tap(combLength);comb.write(bounded(input+t*combFeedback,-4.f,4.f));result=(input+t)*.55f;break;}case 6:result=0;for(int i=0;i<3;++i)result+=formants[size_t(i)].run(input,formantG[size_t(i)],.12f+q*.12f)[2]*(i==0?.8f:.45f);break;case 7:shelf+=std::min(.8f,g)*(input-shelf);result=input+(input-shelf)*(1.5f-q*.5f);break;case 8:shelf+=std::min(.8f,g)*(input-shelf);result=input+shelf*(1.5f-q*.5f);break;default:break;}
 return bounded(result,-8.f,8.f);
}
Decimator::Decimator(){float sum=0;for(int i=0;i<47;++i){int n=i-23;float ideal=n==0?.46f:std::sin(float(tau*.5)*.46f*float(n))/(float(tau*.5)*float(n));weights[size_t(i)]=ideal*(.42f-.5f*std::cos(float(tau*i/46))+.08f*std::cos(float(2*tau*i/46)));sum+=weights[size_t(i)];}for(auto& w:weights)w/=sum;}
float Decimator::output()const{float sum=0;int at=write;for(int i=0;i<47;++i){if(--at<0)at=46;sum+=history[size_t(at)]*weights[size_t(i)];}return sum;}
}
