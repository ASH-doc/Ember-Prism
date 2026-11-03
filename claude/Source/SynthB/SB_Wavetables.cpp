#include "SB_Wavetables.h"
#include <juce_dsp/juce_dsp.h>
#include <complex>
#include <mutex>
#include <algorithm>

namespace sb
{
namespace
{
constexpr int kH = kBaseN / 8;   // 256 harmonics: the top harmonic gets 8 samples per cycle, so cubic interpolation stays clean

const char* const kNames[kTables] = {
    "Saw-Square", "Saw-Triangle", "Pulse Sweep", "Sine-Saw", "Hollow Saw", "Bright-Dull",
    "Resonant Sweep", "Glass Peaks", "Digital Steps", "Harmonic Ladder", "Comb Metal", "Crystal",
    "Vowel AEIOU", "Choir", "Talk", "Nasal Sweep", "Reed",
    "Organ", "Pipe", "Bowed String", "Noise Texture", "Marimba",
    "FM 1:1", "FM 1:2", "FM 1:3", "FM Bell", "Mallet Stack",
    "Air Shimmer", "Whisper", "Breath" };

WaveTable tables[kTables];
std::once_flag once;
const float PI = 3.14159265358979f;

float hash01 (int a, int b = 0)
{
    uint32_t h = (uint32_t) a * 374761393u + (uint32_t) b * 668265263u + 0x9E3779B9u;
    h = (h ^ (h >> 13)) * 1274126177u; h ^= h >> 16;
    return (float) (h & 0xFFFFFF) / 16777216.0f;
}
float gauss (float x, float c, float w) { const float d = (x - c) / w; return std::exp (-0.5f * d * d); }
float smooth (float x) { return x * x * (3.0f - 2.0f * x); }

// vowel formants in harmonic units (assuming a ~130 Hz voice): F1, F2, F3
const float kVowel[5][3] = { { 4.8f, 9.0f, 20.0f }, { 3.3f, 17.0f, 23.0f }, { 2.0f, 19.0f, 25.0f }, { 3.5f, 6.5f, 20.0f }, { 2.2f, 5.0f, 20.0f } };
void vowelAt (float pos, const int* order, float* out)
{
    const float x = std::min (3.999f, std::max (0.0f, pos * 4.0f));
    const int i = (int) x; const float t = smooth (x - (float) i);
    for (int k = 0; k < 3; ++k) out[k] = kVowel[order[i]][k] * (1 - t) + kVowel[order[i + 1]][k] * t;
}

// harmonic recipes: fill b[k] (sine coefficient) and a[k] (cosine coefficient) for k = 1..kH
void harmonics (int tbl, float f, float* a, float* b)
{
    for (int k = 0; k <= kH; ++k) a[k] = b[k] = 0.0f;
    static const int vOrder[5] = { 0, 1, 2, 3, 4 }, tOrder[5] = { 0, 3, 4, 1, 2 };
    for (int k = 1; k <= kH; ++k)
    {
        const float kf = (float) k;
        const bool odd = (k & 1) != 0;
        float bs = 0, ac = 0;
        switch (tbl)
        {
            case 0:  bs = (odd ? 1.0f : (1 - f)) / kf; break;                                         // saw -> square
            case 1:  { const float tri = odd ? ((((k - 1) / 2) & 1) ? -1.0f : 1.0f) / (kf * kf) * 1.2f : 0.0f; bs = (1 - f) / kf + f * tri; } break;
            case 2:  { const float w = 0.5f - 0.46f * f; const float s = std::sin (kf * PI * w); ac = 2 / (kf * PI) * s * std::cos (kf * PI * w); bs = 2 / (kf * PI) * s * s; } break;
            case 3:  bs = k == 1 ? 1.0f : f / kf; break;                                              // sine -> saw
            case 4:  bs = (1.0f / kf) * ((k % 3 == 0) ? (1 + 2.5f * f) : 1.0f) * (odd ? 1.0f : (1 - 0.8f * f)); break;
            case 5:  bs = 1.0f / std::pow (kf, 0.35f + 1.5f * f); break;                              // bright -> dull
            case 6:  { const float h = 2 + 90 * f * f; bs = (1.0f / kf) * (1 + 9 * gauss (kf, h, 1 + h * 0.10f)); } break;
            case 7:  { const float g = gauss (kf, 3 + 22 * f, 2.0f) + 0.7f * gauss (kf, 10 + 50 * f, 3.0f) + 0.5f * gauss (kf, 24 + 120 * f, 6.0f); bs = g / std::sqrt (kf) + 0.15f / kf; } break;
            case 8:  { const int n = 1 + (int) (7 * f); bs = ((k % n == 0) ? 1.0f : 0.12f) / std::pow (kf, 0.8f); } break;
            case 9:  { const float lim = 1 + f * 60; bs = (kf <= lim ? 1.0f : std::max (0.0f, 1 - (kf - lim) / 3)) / kf; } break;
            case 10: { const float P = 3 + 12 * f; bs = (0.55f + 0.45f * std::cos (2 * PI * kf / P)) / kf; } break;
            case 11: { const int m = 2 + (int) (f * 6); bs = ((k % m == 1) ? 1.0f : 0.06f) / kf; const float ph = hash01 (k) * PI * f * 2; ac = bs * std::sin (ph); bs *= std::cos (ph); } break;
            case 12: case 13: case 14: case 15: case 16:
            {
                float fm[3];
                if (tbl == 12) vowelAt (f, vOrder, fm);
                else if (tbl == 13) vowelAt (f, vOrder, fm);
                else if (tbl == 14) vowelAt (f, tOrder, fm);
                else if (tbl == 15) { fm[0] = 2.5f; fm[1] = 5 + 20 * f; fm[2] = 14.0f; }
                else { fm[0] = 8 + 6 * f; fm[1] = 14 + 8 * f; fm[2] = 26.0f; }
                const float wd = tbl == 13 ? 2.2f : (tbl == 14 ? 1.1f : 1.5f);
                float g = 1.0f * gauss (kf, fm[0], wd) + 0.7f * gauss (kf, fm[1], wd * 1.8f) + 0.45f * gauss (kf, fm[2], wd * 2.2f) + 0.02f;
                if (tbl == 16 && ! odd) g *= 0.08f;
                bs = g / kf;
                if (tbl == 13) { const float ph = hash01 (k, 7) * 2 * PI * 0.5f; ac = bs * std::sin (ph); bs *= std::cos (ph); }
            } break;
            case 17: { static const int hs[9] = { 1, 2, 3, 4, 6, 8, 10, 12, 16 }; for (int i = 0; i < 9; ++i) if (k == hs[i]) bs = 0.25f + 0.75f * (0.5f + 0.5f * std::sin (2 * PI * (f * (1 + 0.35f * i) + 0.11f * i))); } break;
            case 18: bs = odd ? 1.0f / kf : (k == 2 ? 0.35f * (1 - f) : 0.02f / kf); if (k <= 7) bs *= 1 + 1.5f * f; break;
            case 19: { const float ph = hash01 (k, 3) * 2 * PI * f; bs = (1.0f / kf) * std::cos (ph); ac = (1.0f / kf) * std::sin (ph); } break;
            case 20: { const float fp = f * 7; const int i = (int) fp; const float t = smooth (fp - (float) i);
                       bs = (hash01 (k, i) * (1 - t) + hash01 (k, i + 1) * t) / std::pow (kf, 0.7f); } break;
            case 21: { if (k == 1) bs = 1.0f; if (k == 4) bs = 0.8f * f + 0.1f; if (k == 10) bs = 0.5f * f * f; if (k == 3) bs = 0.2f * (1 - f); } break;
            case 26: { if (k == 1) bs = 1.0f; if (k == 4) bs = 0.8f * f; if (k == 9) bs = 0.55f * f * f; if (k == 15) bs = 0.35f * f * f * f; if (k == 2) bs = 0.15f; } break;
            case 27: { const float c = 40 + 300 * f; bs = (kf < 8 ? 0.0f : gauss (std::log (kf), std::log (c), 0.9f)) / std::pow (kf, 0.3f);
                       const float ph = hash01 (k, 5) * 2 * PI; ac = bs * std::sin (ph); bs *= std::cos (ph); } break;
            case 28: { const float lim = 40 + 460 * f; const float g = (kf <= lim ? 1.0f : 0.0f) * (0.3f + hash01 (k, 11)) / std::pow (kf, 0.5f); const float ph = hash01 (k, 13) * 2 * PI;
                       bs = g * std::cos (ph); ac = g * std::sin (ph); if (k == 1) bs += 0.3f; } break;
            case 29: { bs = k == 1 ? 1.0f : f * 0.35f * hash01 (k, 17) / std::pow (kf, 0.3f); if (k > 1) { const float ph = hash01 (k, 19) * 2 * PI; ac = bs * std::sin (ph); bs *= std::cos (ph); } } break;
            default: break;
        }
        b[k] = bs; a[k] = ac;
    }
}

float waveformSample (int tbl, float f, float th)   // time-domain recipes (FM family)
{
    const float I = 9.0f * f;
    switch (tbl)
    {
        case 22: return std::sin (th + I * std::sin (th));
        case 23: return std::sin (th + I * std::sin (2 * th));
        case 24: return std::sin (th + I * 0.8f * std::sin (3 * th));
        default: return 0.65f * std::sin (th + I * 0.7f * std::sin (4 * th)) + 0.35f * std::sin (5 * th + I * 0.5f * std::sin (7 * th));  // 25: bell
    }
}

void buildTable (int tbl)
{
    WaveTable& wt = tables[tbl];
    for (int m = 0; m < kMips; ++m) wt.mip[m].assign ((size_t) kFrames * (size_t) (kBaseN >> m), 0.0f);

    std::vector<float> a ((size_t) kH + 1), b ((size_t) kH + 1);
    std::vector<std::complex<float>> in ((size_t) kBaseN), out ((size_t) kBaseN);
    const bool timeDomain = tbl >= 22 && tbl <= 25;

    for (int fr = 0; fr < kFrames; ++fr)
    {
        const float f = (float) fr / (float) (kFrames - 1);
        if (timeDomain)
        {
            juce::dsp::FFT fft (11);
            for (int n = 0; n < kBaseN; ++n) in[(size_t) n] = waveformSample (tbl, f, 2 * PI * (float) n / (float) kBaseN);
            fft.perform (reinterpret_cast<juce::dsp::Complex<float>*> (in.data()), reinterpret_cast<juce::dsp::Complex<float>*> (out.data()), false);
            for (int k = 1; k <= kH; ++k) { a[(size_t) k] = 2 * out[(size_t) k].real() / (float) kBaseN; b[(size_t) k] = -2 * out[(size_t) k].imag() / (float) kBaseN; }
        }
        else harmonics (tbl, f, a.data(), b.data());

        // loudness normalisation from the full-band frame: constant RMS, but never above full scale
        float scale = 1.0f;
        for (int m = 0; m < kMips; ++m)
        {
            const int N = kBaseN >> m, H = kH >> m, order = 11 - m;
            juce::dsp::FFT fft (order);
            std::vector<std::complex<float>> ci ((size_t) N, { 0, 0 }), co ((size_t) N);
            for (int k = 1; k <= H; ++k)
            {
                const std::complex<float> c (a[(size_t) k] * 0.5f, -b[(size_t) k] * 0.5f);
                ci[(size_t) k] = c; ci[(size_t) (N - k)] = std::conj (c);
            }
            fft.perform (reinterpret_cast<juce::dsp::Complex<float>*> (ci.data()), reinterpret_cast<juce::dsp::Complex<float>*> (co.data()), true);
            float* dst = wt.mip[m].data() + (size_t) fr * (size_t) N;
            float peak = 1e-9f; double sq = 0;
            for (int n = 0; n < N; ++n) { dst[n] = co[(size_t) n].real() * (float) N; peak = std::max (peak, std::fabs (dst[n])); sq += (double) dst[n] * dst[n]; }
            if (m == 0)
            {
                const float rms = (float) std::sqrt (sq / N) + 1e-9f;
                scale = std::min (0.33f / rms, 0.95f / peak);
            }
            for (int n = 0; n < N; ++n) dst[n] *= scale;
        }
    }
}
} // namespace

void generateWavetables()
{
    std::call_once (once, [] { for (int t = 0; t < kTables; ++t) buildTable (t); });
}
const WaveTable& wavetable (int index) { return tables[std::max (0, std::min (kTables - 1, index))]; }
const char* wavetableName (int index) { return kNames[std::max (0, std::min (kTables - 1, index))]; }
} // namespace sb


