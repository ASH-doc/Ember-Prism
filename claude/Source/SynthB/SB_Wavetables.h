// PRISM (Synth B) - the original generated wavetable library (30 tables x 32 frames), band-limited by octave mip levels.
#pragma once
#include <vector>
#include <cmath>

namespace sb
{
constexpr int kTables = 30;
constexpr int kFrames = 32;
constexpr int kBaseN = 2048;      // samples per frame at mip 0 (256 harmonics = 8x oversampled)
constexpr int kMips = 8;          // mip m: kBaseN >> m samples, 256 >> m harmonics

struct WaveTable
{
    std::vector<float> mip[kMips];    // [frame * (kBaseN >> m) + i]
};

void generateWavetables();                    // call once (idempotent, thread-safe enough for startup)
const WaveTable& wavetable (int index);
const char* wavetableName (int index);

inline float cubic (const float* p, int i, float t, int mask)
{
    const float y0 = p[(i - 1) & mask], y1 = p[i & mask], y2 = p[(i + 1) & mask], y3 = p[(i + 2) & mask];
    const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * t + c2) * t + c1) * t + y1;
}

// read one band-limited sample: framePos 0..1 across the table, phase 0..1 within the cycle
inline float readTable (const WaveTable& wt, int mip, float framePos, float phase)
{
    const int N = kBaseN >> mip, mask = N - 1;
    const float fp = framePos * (float) (kFrames - 1);
    int f0 = (int) fp; if (f0 > kFrames - 2) f0 = kFrames - 2; if (f0 < 0) f0 = 0;
    const float ff = fp - (float) f0;
    const float x = phase * (float) N;
    const int i = (int) x; const float t = x - (float) i;
    const float* base = wt.mip[mip].data();
    const float a = cubic (base + f0 * N, i, t, mask), b = cubic (base + (f0 + 1) * N, i, t, mask);
    return a + (b - a) * ff;
}
} // namespace sb

