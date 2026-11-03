// EMBER (Synth A) - DSP building blocks. Everything here belongs to Synth A only.
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <vector>

namespace sa
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
    uint32_t s = 0x1234567u;
    float next()   { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xFFFFFF) * (1.0f / 8388608.0f) - 1.0f; } // -1..1
    float uni()    { return next() * 0.5f + 0.5f; }
};

// ---------------------------------------------------------------- oscillators
inline float polyBlep (float t, float dt)
{
    if (t < dt)         { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt)  { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}
inline float oscSaw (float t, float dt) { return 2.0f * t - 1.0f - polyBlep (t, dt); }
inline float oscPulse (float t, float dt, float pw)
{
    float y = t < pw ? 1.0f : -1.0f;
    y += polyBlep (t, dt);
    float t2 = t - pw; if (t2 < 0.0f) t2 += 1.0f;
    y -= polyBlep (t2, dt);
    return y;
}
// triangle = leaky integral of a 50% band-limited pulse (state kept by caller)
inline float oscTri (float t, float dt, float& state)
{
    state = state * 0.9995f + 4.0f * dt * oscPulse (t, dt, 0.5f);
    return state;
}

// ---------------------------------------------------------------- envelope (analog-style exponential segments)
struct Adsr
{
    enum Stage { Idle, Att, Dec, Sus, Rel };
    Stage st = Idle;
    float v = 0.0f;

    void gateOn()  { st = Att; }
    void gateOff() { if (st != Idle) st = Rel; }
    void kill()    { st = Idle; v = 0.0f; }
    bool active() const { return st != Idle; }

    // advance by n samples
    float step (int n, float a, float d, float s, float r, float sr)
    {
        switch (st)
        {
            case Att:
            {
                v += (1.25f - v) * (1.0f - std::exp (-1.609f * (float) n / (std::max (a, 0.0005f) * sr)) );
                if (v >= 1.0f) { v = 1.0f; st = Dec; }
                break;
            }
            case Dec:
                v += (s - v) * (1.0f - std::exp (-4.6f * (float) n / (std::max (d, 0.001f) * sr)));
                if (std::fabs (v - s) < 0.0005f) { v = s; st = Sus; }
                break;
            case Sus: v = s; break;
            case Rel:
                v *= std::exp (-4.6f * (float) n / (std::max (r, 0.001f) * sr));
                if (v < 0.0001f) { v = 0.0f; st = Idle; }
                break;
            default: v = 0.0f; break;
        }
        return v;
    }
};

// ---------------------------------------------------------------- LFO (phase supplied by the engine)
struct Lfo
{
    float phase = 0.0f, last = 0.0f, sh = 0.0f, from = 0.0f, to = 0.0f;
    Rng rng;
    float eval (int shape, float ph)
    {
        const bool wrap = ph < last; last = ph;
        if (wrap) { from = to; to = rng.next(); sh = rng.next(); }
        switch (shape)
        {
            case 0: return std::sin (2.0f * kPi * ph);
            case 1: return 4.0f * std::fabs (ph - 0.5f) - 1.0f;
            case 2: return 2.0f * ph - 1.0f;
            case 3: return ph < 0.5f ? 1.0f : -1.0f;
            case 4: return sh;
            default: { const float x = ph * ph * (3.0f - 2.0f * ph); return from + (to - from) * x; }
        }
    }
};

// ---------------------------------------------------------------- filters
struct FiltCoef
{
    int model = 0, type = 0;
    float g = 0.1f, G = 0.1f, k = 0.0f, a1 = 1, a2 = 0, a3 = 0, drive = 1.0f, comp = 1.0f, ladderDen = 1.0f, G4 = 0.0f;
};
struct FiltState { float s1 = 0, s2 = 0, s3 = 0, s4 = 0; };

inline void filtSet (FiltCoef& c, int model, int type, float cutHz, float res, float drive, float sr)
{
    cutHz = std::max (15.0f, std::min (cutHz, 0.45f * sr));
    c.model = model; c.type = type;
    c.g = std::tan (kPi * cutHz / sr);
    c.drive = 1.0f + drive * (model == 2 ? 7.0f : 4.0f);
    if (model == 0)
    {
        c.G = c.g / (1.0f + c.g);
        c.k = res * 4.05f;
        c.G4 = c.G * c.G * c.G * c.G;
        c.ladderDen = 1.0f / (1.0f + c.k * c.G4);
        c.comp = 1.0f + 0.30f * res;     // Moog: only partially restore the bass the resonance eats
    }
    else
    {
        const float q = model == 2 ? 0.995f : 0.985f;
        c.k = 2.0f * (1.0f - res * q) + 0.01f;
        c.a1 = 1.0f / (1.0f + c.g * (c.g + c.k));
        c.a2 = c.g * c.a1;
        c.a3 = c.g * c.a2;
        c.comp = 1.0f;
    }
}

inline float filtRun (const FiltCoef& c, FiltState& s, float x)
{
    if (c.model == 0)
    {
        const float G = c.G, iG = 1.0f - G;
        const float sg = G * G * G * iG * s.s1 + G * G * iG * s.s2 + G * iG * s.s3 + iG * s.s4;
        float u = fastTanh ((x * c.drive - c.k * sg) * c.ladderDen);
        const float y1 = G * u + iG * s.s1;
        const float x2 = fastTanh (y1 * 0.92f) * 1.087f;
        const float y2 = G * x2 + iG * s.s2;
        const float x3 = fastTanh (y2 * 0.92f) * 1.087f;
        const float y3 = G * x3 + iG * s.s3;
        const float x4 = fastTanh (y3 * 0.92f) * 1.087f;
        const float y4 = G * x4 + iG * s.s4;
        s.s1 = flushDenorm (2.0f * y1 - s.s1);
        s.s2 = flushDenorm (2.0f * y2 - s.s2);
        s.s3 = flushDenorm (2.0f * y3 - s.s3);
        s.s4 = flushDenorm (2.0f * y4 - s.s4);
        return y4 * c.comp / std::sqrt (c.drive);
    }

    // zero-delay-feedback state variable core, three personalities
    float in = x;
    if (c.model == 1)      in = fastTanh (x * c.drive) / std::sqrt (c.drive);              // smooth Juno / Jupiter flavour
    else if (c.model == 2) in = fastTanh (x * c.drive * 0.7f) / std::sqrt (c.drive * 0.7f); // gritty
    else                   in = fastTanh (x * (1.0f + (c.drive - 1.0f) * 0.5f)) / std::sqrt (1.0f + (c.drive - 1.0f) * 0.5f);
    const float v3 = in - s.s2;
    const float v1 = c.a1 * s.s1 + c.a2 * v3;
    const float v2 = s.s2 + c.a2 * s.s1 + c.a3 * v3;
    float n1 = 2.0f * v1 - s.s1, n2 = 2.0f * v2 - s.s2;
    if (c.model == 1) { n1 = fastTanh (n1 * 0.6f) * 1.667f; }
    else if (c.model == 2)  // MS-20-ish: asymmetric clip on the band state makes the resonance scream
    {
        n1 = n1 > 0.0f ? fastTanh (n1 * 1.6f) / 1.6f * 1.2f : fastTanh (n1 * 0.9f) / 0.9f;
    }
    s.s1 = flushDenorm (n1); s.s2 = flushDenorm (n2);
    switch (c.type)
    {
        case 0:  return v2;
        case 1:  return in - c.k * v1 - v2;
        case 2:  return v1 * c.k;
        default: return in - c.k * v1;
    }
}

// ---------------------------------------------------------------- small delay line
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

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void shelf (bool high, float f, float gainDb, float sr)
    {
        const float A = std::pow (10.0f, gainDb / 40.0f), w = 2.0f * kPi * f / sr, cs = std::cos (w), sn = std::sin (w);
        const float alpha = sn / 2.0f * std::sqrt (2.0f), sq = 2.0f * std::sqrt (A) * alpha;
        float a0;
        if (! high) { b0 = A * ((A + 1) - (A - 1) * cs + sq); b1 = 2 * A * ((A - 1) - (A + 1) * cs); b2 = A * ((A + 1) - (A - 1) * cs - sq);
                      a0 = (A + 1) + (A - 1) * cs + sq; a1 = -2 * ((A - 1) + (A + 1) * cs); a2 = (A + 1) + (A - 1) * cs - sq; }
        else        { b0 = A * ((A + 1) + (A - 1) * cs + sq); b1 = -2 * A * ((A - 1) + (A + 1) * cs); b2 = A * ((A + 1) + (A - 1) * cs - sq);
                      a0 = (A + 1) - (A - 1) * cs + sq; a1 = 2 * ((A - 1) - (A + 1) * cs); a2 = (A + 1) - (A - 1) * cs - sq; }
        b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
    }
    float run (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
};
} // namespace sa
