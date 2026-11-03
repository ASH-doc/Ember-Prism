// sa-parameters.h — Native parameter contract; shared by DSP, UI, patches and automation.
#pragma once
#include <array>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
namespace SynthA {
constexpr int maxParams = 512;
constexpr int maxVoices = 16;
constexpr double pi = 3.14159265358979323846;
template<class T> T clamp(T x,T lo,T hi) { return std::max(lo,std::min(x,hi)); }
struct Param {
    std::string id, label, unit, group;
    float min=0, max=1, initial=0, skew=1;
    std::vector<std::string> choices;
    bool discrete=false;
    float normalise(float v) const { return std::pow(clamp((v-min)/(max-min),0.f,1.f),skew); }
    float denormalise(float n) const { return min+(max-min)*std::pow(clamp(n,0.f,1.f),1.f/skew); }
    float limit(float v) const { if(!std::isfinite(v)) return initial; v=clamp(v,min,max); return discrete?std::round(v):v; }
};
const std::vector<Param>& parameters();
int indexOf(const std::string& id);
using Values = std::array<float,maxParams>;
Values defaults();
const std::vector<std::string>& modSources();
// Musical lengths in beats: 1/32, 1/16T, 1/16, 1/8T, 1/8, dotted 1/8, 1/4, 1/2, 1 bar, 2 bars.
constexpr std::array<float,10> divisions { .125f,1.f/6,.25f,1.f/3,.5f,.75f,1.f,2.f,4.f,8.f };
}
