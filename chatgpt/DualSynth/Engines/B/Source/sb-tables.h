// sb-tables.h — Original spectral tables, variable-size mip levels and cubic playback.
#pragma once
#include "sb-schema.h"
#include <complex>
#include <memory>
namespace SynthB {
void transform(std::vector<std::complex<float>>&,bool inverse);
struct WaveLevel {int length=0;std::vector<float> samples;};
struct WaveSet {
 static constexpr int frameLength=2048, mipCount=10;
 std::string title;int frames=64;std::array<WaveLevel,mipCount> layers;
 float sample(float phase,float position,float mip)const;
 float visual(float phase,float position)const;
 static std::unique_ptr<WaveSet> fromFrames(const std::vector<float>& samples,int frameCount,std::string name,bool normalize=true);
};
class Tables {
 public:Tables();std::array<std::unique_ptr<WaveSet>,30> library;
 const WaveSet& at(int index,const WaveSet* imported=nullptr,int claudeSelection=0)const;
 private:std::array<std::unique_ptr<WaveSet>,30> claude;
 public:
 static const Tables& get();
};
std::unique_ptr<WaveSet> makeClaudeTable(int index);
float warpedPhase(float phase,int mode,float amount,float modulator);
float warpedAmplitude(float sample,int mode,float amount,float modulator);
float warpBandwidth(int mode,float amount);
}
