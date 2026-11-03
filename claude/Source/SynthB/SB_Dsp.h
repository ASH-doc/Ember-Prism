// PRISM (Synth B) - DSP building blocks. Everything here belongs to Synth B only.
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <array>
#include <vector>

namespace sb
{
constexpr float kPi = 3.14159265358979f;

inline float fastTanh (float x)
{
    x = std::max (-3.0f, std::min (3.0f, x));
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}
inline float flushDenorm (float x) { return std::fabs (x) < 1.0e-20f ? 0.0f : x; }

struct Rng
{
    uint32_t s = 0x2468ACEu;
    float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xFFFFFF) * (1.0f / 8388608.0f) - 1.0f; }
    float uni()  { return next() * 0.5f + 0.5f; }
};

// ---------------------------------------------------------------- envelope: smooth exponential ADSR
struct Adsr
{
    enum Stage { Idle, Att, Dec, Sus, Rel };
    Stage st = Idle;
    float v = 0.0f;
    void gateOn()  { st = Att; }
    void gateOff() { if (st != Idle) st = Rel; }
    void kill()    { st = Idle; v = 0.0f; }
    bool active() const { return st != Idle; }
    float step (int n, float a, float d, float s, float r, float sr)
    {
        const float fn = (float) n;
        switch (st)
        {
            case Att:
                v += (1.2f - v) * (1.0f - std::exp (-1.79f * fn / (std::max (a, 0.0005f) * sr)));
                if (v >= 1.0f) { v = 1.0f; st = Dec; }
                break;
            case Dec:
                v += (s - v) * (1.0f - std::exp (-4.6f * fn / (std::max (d, 0.001f) * sr)));
                if (std::fabs (v - s) < 0.0005f) { v = s; st = Sus; }
                break;
            case Sus: v = s; break;
            case Rel:
                v *= std::exp (-4.6f * fn / (std::max (r, 0.001f) * sr));
                if (v < 0.0001f) { v = 0.0f; st = Idle; }
                break;
            default: v = 0.0f; break;
        }
        return v;
    }
};

// ---------------------------------------------------------------- LFO with a drawable 16-point shape
struct Lfo
{
    float last = 0.0f, sh = 0.0f;
    Rng rng;
    float eval (int shape, float ph, const float* custom /* 16 points, -1..1 */)
    {
        if (ph < last) sh = rng.next();
        last = ph;
        switch (shape)
        {
            case 0: return std::sin (2.0f * kPi * ph);
            case 1: return 4.0f * std::fabs (ph - 0.5f) - 1.0f;
            case 2: return 2.0f * ph - 1.0f;
            case 3: return ph < 0.5f ? 1.0f : -1.0f;
            case 4: return sh;
            default:
            {
                const float x = ph * 16.0f; const int i = (int) x & 15; const float t = x - std::floor (x);
                const float a = custom[i], b = custom[(i + 1) & 15], s = t * t * (3.0f - 2.0f * t);
                return a + (b - a) * s;
            }
        }
    }
};

// ---------------------------------------------------------------- filters (clean TPT state-variable family)
struct FiltState
{
    float s[8] = {};
    float bz1 = 0, bz2 = 0;                  // shelf biquad
    std::vector<float> comb;                 // comb delay line (heap, so the engine itself stays small)
    int cw = 0; float clp = 0.0f;
    FiltState() : comb (4096, 0.0f) {}
    void reset() { for (auto& x : s) x = 0; bz1 = bz2 = 0; std::fill (comb.begin(), comb.end(), 0.0f); cw = 0; clp = 0; }
};

struct FiltCoef
{
    int type = 0;
    float g = 0.1f, k = 1.0f, a1 = 1, a2 = 0, a3 = 0, drive = 1.0f, mix = 1.0f;
    float combDelay = 100.0f, combFb = 0.0f;
    float fg[3] = { 0.1f, 0.1f, 0.1f }, fk = 0.5f, fa[3] = {}, fgain[3] = { 1, 0.7f, 0.5f };
    float b0 = 1, b1 = 0, b2 = 0, sa1 = 0, sa2 = 0;     // shelf
};

inline void svfCoef (float g, float k, float& a1, float& a2, float& a3) { a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2; }

inline void filtSet (FiltCoef& c, int type, float cutHz, float res, float drive, float mix, float sr)
{
    cutHz = std::max (15.0f, std::min (cutHz, 0.45f * sr));
    c.type = type; c.mix = mix;
    c.drive = 1.0f + drive * 5.0f;
    c.g = std::tan (kPi * cutHz / sr);
    c.k = 2.0f * (1.0f - res * 0.97f) + 0.02f;
    if (type == 1) c.k = 2.0f * (1.0f - res * 0.9f) + 0.1f;   // each of the two LP24 stages
    svfCoef (c.g, c.k, c.a1, c.a2, c.a3);
    if (type == 5)
    {
        c.combDelay = std::max (4.0f, std::min (4000.0f, sr / cutHz));
        c.combFb = 0.2f + res * 0.76f;
    }
    else if (type == 6)   // formant: cutoff sweeps the five vowels, resonance narrows the formants
    {
        static const float F[5][3] = { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 }, { 570, 840, 2410 }, { 300, 870, 2240 } };
        const float pos = std::max (0.0f, std::min (3.999f, std::log (cutHz / 20.0f) / std::log (1000.0f) * 4.0f));
        const int i = (int) pos; const float t = pos - (float) i;
        c.fk = 2.0f * (1.0f - res * 0.9f) + 0.12f;
        for (int j = 0; j < 3; ++j)
        {
            const float fr = F[i][j] * (1 - t) + F[i + 1][j] * t;
            c.fg[j] = std::tan (kPi * std::min (fr, 0.45f * sr) / sr);
            float a1, a2, a3; svfCoef (c.fg[j], c.fk, a1, a2, a3); c.fa[j] = a1; // a1 stored; a2/a3 recomputed in run
        }
    }
    else if (type == 7)   // high "air" shelf
    {
        const float A = std::pow (10.0f, res * 12.0f / 40.0f), w = 2.0f * kPi * cutHz / sr, cs = std::cos (w), sn = std::sin (w);
        const float alpha = sn / 2.0f * 1.41421356f, sq = 2.0f * std::sqrt (A) * alpha;
        const float a0 = (A + 1) - (A - 1) * cs + sq;
        c.b0 = A * ((A + 1) + (A - 1) * cs + sq) / a0; c.b1 = -2 * A * ((A - 1) + (A + 1) * cs) / a0; c.b2 = A * ((A + 1) + (A - 1) * cs - sq) / a0;
        c.sa1 = 2 * ((A - 1) - (A + 1) * cs) / a0; c.sa2 = ((A + 1) - (A - 1) * cs - sq) / a0;
    }
}

inline float filtRun (const FiltCoef& c, FiltState& st, float x)
{
    const float dry = x;
    float in = c.drive > 1.0001f ? fastTanh (x * c.drive) / std::sqrt (c.drive) : x, y = 0.0f;
    switch (c.type)
    {
        case 0: case 1: case 2: case 3: case 4:
        {
            auto stage = [&] (float inp, float& s1, float& s2, int mode)
            {
                const float v3 = inp - s2, v1 = c.a1 * s1 + c.a2 * v3, v2 = s2 + c.a2 * s1 + c.a3 * v3;
                s1 = flushDenorm (2.0f * v1 - s1); s2 = flushDenorm (2.0f * v2 - s2);
                switch (mode) { case 0: return v2; case 1: return inp - c.k * v1 - v2; case 2: return v1 * c.k; default: return inp - c.k * v1; }
            };
            if (c.type == 0) y = stage (in, st.s[0], st.s[1], 0);
            else if (c.type == 1) y = stage (stage (in, st.s[0], st.s[1], 0), st.s[2], st.s[3], 0);
            else if (c.type == 2) y = stage (in, st.s[0], st.s[1], 1);
            else if (c.type == 3) y = stage (in, st.s[0], st.s[1], 2);
            else y = stage (in, st.s[0], st.s[1], 3);
            break;
        }
        case 5:
        {
            float rp = (float) st.cw - c.combDelay; while (rp < 0) rp += 4096.0f;
            const int i0 = (int) rp; const float f = rp - (float) i0;
            const float d = st.comb[(size_t) (i0 & 4095)] * (1 - f) + st.comb[(size_t) ((i0 + 1) & 4095)] * f;
            st.clp += 0.6f * (d - st.clp);
            y = in + st.clp * c.combFb;
            st.comb[(size_t) st.cw] = flushDenorm (fastTanh (y));
            st.cw = (st.cw + 1) & 4095;
            y *= 0.55f;
            break;
        }
        case 6:
        {
            for (int j = 0; j < 3; ++j)
            {
                const float g = c.fg[j], a1 = c.fa[j], a2 = g * a1, a3 = g * a2;
                float& s1 = st.s[j * 2]; float& s2 = st.s[j * 2 + 1];
                const float v3 = in - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
                s1 = flushDenorm (2.0f * v1 - s1); s2 = flushDenorm (2.0f * v2 - s2);
                y += v1 * c.fk * c.fgain[j];
            }
            y *= 1.5f;
            break;
        }
        default:
        {
            const float o = c.b0 * in + st.bz1;
            st.bz1 = c.b1 * in - c.sa1 * o + st.bz2; st.bz2 = c.b2 * in - c.sa2 * o;
            y = o;
            break;
        }
    }
    return c.mix >= 0.999f ? y : dry + (y - dry) * c.mix;
}

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void setShelf (bool high, float f, float gainDb, float sr)
    {
        const float A = std::pow (10.0f, gainDb / 40.0f), w = 2.0f * kPi * f / sr, cs = std::cos (w), sn = std::sin (w);
        const float alpha = sn / 2.0f * 1.41421356f, sq = 2.0f * std::sqrt (A) * alpha;
        float a0;
        if (! high) { b0 = A * ((A + 1) - (A - 1) * cs + sq); b1 = 2 * A * ((A - 1) - (A + 1) * cs); b2 = A * ((A + 1) - (A - 1) * cs - sq);
                      a0 = (A + 1) + (A - 1) * cs + sq; a1 = -2 * ((A - 1) + (A + 1) * cs); a2 = (A + 1) + (A - 1) * cs - sq; }
        else        { b0 = A * ((A + 1) + (A - 1) * cs + sq); b1 = -2 * A * ((A - 1) + (A + 1) * cs); b2 = A * ((A + 1) + (A - 1) * cs - sq);
                      a0 = (A + 1) - (A - 1) * cs + sq; a1 = 2 * ((A - 1) - (A + 1) * cs); a2 = (A + 1) - (A - 1) * cs - sq; }
        b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
    }
    void setPeak (float f, float gainDb, float q, float sr)
    {
        const float A = std::pow (10.0f, gainDb / 40.0f), w = 2.0f * kPi * f / sr, cs = std::cos (w), alpha = std::sin (w) / (2 * q);
        const float a0 = 1 + alpha / A;
        b0 = (1 + alpha * A) / a0; b1 = -2 * cs / a0; b2 = (1 - alpha * A) / a0; a1 = -2 * cs / a0; a2 = (1 - alpha / A) / a0;
    }
    void copyCoef (const Biquad& o) { b0 = o.b0; b1 = o.b1; b2 = o.b2; a1 = o.a1; a2 = o.a2; }
    float run (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
};

struct DelayLine
{
    std::vector<float> buf; int w = 0, mask = 0;
    void init (int minSize) { int n = 1; while (n < minSize + 4) n <<= 1; buf.assign ((size_t) n, 0.0f); mask = n - 1; w = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
    void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    float read (float d) const
    {
        float rp = (float) w - d; while (rp < 0) rp += (float) (mask + 1);
        const int i0 = (int) rp; const float f = rp - (float) i0;
        return buf[(size_t) (i0 & mask)] * (1.0f - f) + buf[(size_t) ((i0 + 1) & mask)] * f;
    }
};
} // namespace sb


