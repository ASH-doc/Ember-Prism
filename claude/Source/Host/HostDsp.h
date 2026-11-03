// AETHER DUO host DSP: glue compressor, master reverb, safety limiter. Pure DSP (no UI, no JUCE types) so it can be ported to a plugin later.
// These are owned by the host and are original - nothing is shared with the two synths' own effects.
#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include <atomic>

namespace duo
{
constexpr float kPi = 3.14159265358979f;
inline float dbToLin (float db) { return std::pow (10.0f, db * 0.05f); }
inline float linToDb (float x) { return 20.0f * std::log10 (std::max (x, 1.0e-9f)); }
inline float flushDn (float x) { return std::fabs (x) < 1.0e-20f ? 0.0f : x; }

// ------------------------------------------------------------------------------------------ stereo-linked glue compressor
struct GlueCompressor
{
    struct Params { bool on = true; float thresholdDb = -14, ratio = 2, attackMs = 10, releaseMs = 120, kneeDb = 6, makeupDb = 0, mix = 1; };

    void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }
    void reset() { grDb = 0; hpL = hpR = 0; onMix = 1; }

    // processes in place; returns the gain reduction (positive dB) at the end of the block
    float process (float* L, float* R, int n, const Params& p)
    {
        const float aA = std::exp (-1.0f / (std::max (0.1f, p.attackMs) * 0.001f * sr)), aR = std::exp (-1.0f / (std::max (5.0f, p.releaseMs) * 0.001f * sr));
        const float hpc = std::exp (-2.0f * kPi * 90.0f / sr);                  // side-chain high-pass: bass does not pump the whole mix
        const float makeup = dbToLin (p.makeupDb), ratio = std::max (1.0f, p.ratio), knee = std::max (0.01f, p.kneeDb);
        const float tgtOn = p.on ? 1.0f : 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float dl = L[i], dr = R[i];
            hpL = (1 - hpc) * dl + hpc * hpL; hpR = (1 - hpc) * dr + hpc * hpR;
            const float sl = dl - hpL, sr_ = dr - hpR;
            const float level = linToDb (std::max (std::fabs (sl), std::fabs (sr_)));
            const float over = level - p.thresholdDb;
            float red;
            if (over <= -knee * 0.5f) red = 0.0f;
            else if (over >= knee * 0.5f) red = over * (1.0f - 1.0f / ratio);
            else { const float x = over + knee * 0.5f; red = (1.0f - 1.0f / ratio) * x * x / (2.0f * knee); }   // soft knee
            grDb = red > grDb ? aA * grDb + (1 - aA) * red : aR * grDb + (1 - aR) * red;
            onMix += (tgtOn - onMix) * 0.002f;
            const float g = dbToLin (-grDb) * makeup;
            const float wl = dl * g, wr = dr * g;
            const float m = p.mix * onMix;
            L[i] = dl + (wl - dl) * m; R[i] = dr + (wr - dr) * m;
        }
        return grDb;
    }
    float sr = 48000.0f, grDb = 0, hpL = 0, hpR = 0, onMix = 1;
};

// ------------------------------------------------------------------------------------------ algorithmic master reverb (8-line FDN)
struct MasterReverb
{
    struct Params { bool on = true; float size = 0.5f, decay = 2.2f, damp = 0.5f, preDelayMs = 15, lowCutHz = 200, wet = 0.12f; };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        static const int base[8] = { 1051, 1307, 1597, 1877, 2131, 2423, 2713, 3001 };
        static const int apT[4] = { 142, 107, 379, 277 };
        for (int i = 0; i < 8; ++i) { baseLen[i] = (int) ((float) base[i] * sr / 48000.0f); line[i].assign ((size_t) (baseLen[i] * 2) + 128, 0.0f); }
        for (int i = 0; i < 4; ++i) { apLen[i] = (int) ((float) apT[i] * sr / 48000.0f) + 1; ap[i].assign ((size_t) apLen[i] + 4, 0.0f); }
        pre.assign ((size_t) (0.2f * sr) + 8, 0.0f);
        reset();
    }
    void reset()
    {
        for (auto& l : line) std::fill (l.begin(), l.end(), 0.0f);
        for (auto& a : ap) std::fill (a.begin(), a.end(), 0.0f);
        std::fill (pre.begin(), pre.end(), 0.0f);
        for (int i = 0; i < 8; ++i) { pos[i] = 0; lp[i] = 0; }
        for (int i = 0; i < 4; ++i) apPos[i] = 0;
        prePos = 0; hpS = 0; lfo = 0; wetCur = 0;
    }

    void process (float* L, float* R, int n, const Params& p)
    {
        const float sz = 0.5f + 1.1f * p.size;
        int len[8]; float g[8];
        const float rt = std::max (0.2f, p.decay);
        for (int j = 0; j < 8; ++j) { len[j] = std::max (16, std::min ((int) line[j].size() - 8, (int) ((float) baseLen[j] * sz))); g[j] = std::pow (10.0f, -3.0f * (float) len[j] / (sr * rt)); }
        const float damp = 0.04f + 0.8f * p.damp, hpc = std::exp (-2.0f * kPi * std::max (20.0f, p.lowCutHz) / sr);
        const int preS = std::max (1, std::min ((int) pre.size() - 2, (int) (p.preDelayMs * 0.001f * sr)));
        const float wetT = p.on ? p.wet : 0.0f;
        for (int i = 0; i < n; ++i)
        {
            float in = (L[i] + R[i]) * 0.5f;
            hpS = (1 - hpc) * in + hpc * hpS; in -= hpS;                                 // low cut on the reverb input
            pre[(size_t) prePos] = in; const float x0 = pre[(size_t) ((prePos - preS + (int) pre.size()) % (int) pre.size())]; prePos = (prePos + 1) % (int) pre.size();
            float x = x0;
            for (int s = 0; s < 4; ++s)                                                  // input diffusion
            {
                float& b = ap[s][(size_t) apPos[s]];
                const float v = b, y = -0.55f * x + v; b = flushDn (x + 0.55f * v); x = y;
                if (++apPos[s] >= apLen[s]) apPos[s] = 0;
            }
            lfo += 0.31f / sr; if (lfo >= 1) lfo -= 1;
            float t[8];
            for (int j = 0; j < 8; ++j)
            {
                const float mod = (float) len[j] * (1.0f + 0.0006f * std::sin (2 * kPi * (lfo + 0.125f * (float) j)));
                float rp = (float) pos[j] - mod; const int sz2 = (int) line[j].size(); while (rp < 0) rp += (float) sz2;
                const int i0 = (int) rp; const float fr = rp - (float) i0;
                const float d = line[j][(size_t) (i0 % sz2)] * (1 - fr) + line[j][(size_t) ((i0 + 1) % sz2)] * fr;
                lp[j] += (1 - damp) * (d - lp[j]);
                t[j] = lp[j] * g[j];
            }
            for (int s = 1; s < 8; s <<= 1)
                for (int j = 0; j < 8; j += 2 * s)
                    for (int k = j; k < j + s; ++k) { const float u = t[k], v = t[k + s]; t[k] = u + v; t[k + s] = u - v; }
            for (int j = 0; j < 8; ++j)
            {
                line[j][(size_t) pos[j]] = flushDn (x * 0.5f + t[j] * 0.35355339f);
                if (++pos[j] >= (int) line[j].size()) pos[j] = 0;
            }
            const float oL = (t[0] + t[2] + t[4] + t[6]) * 0.28f, oR = (t[1] + t[3] + t[5] + t[7]) * 0.28f;
            wetCur += (wetT - wetCur) * 0.002f;
            L[i] += oL * wetCur * 1.6f; R[i] += oR * wetCur * 1.6f;
        }
    }
    float sr = 48000.0f, hpS = 0, lfo = 0, wetCur = 0;
    int baseLen[8] = {}, pos[8] = {}, apLen[4] = {}, apPos[4] = {}, prePos = 0;
    float lp[8] = {};
    std::vector<float> line[8], ap[4], pre;
};

// ------------------------------------------------------------------------------------------ look-ahead brick-wall safety limiter
struct SafetyLimiter
{
    static constexpr int kLook = 96;       // 2 ms at 48 kHz
    float ceiling = 0.96605f;              // -0.3 dBFS

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        look = std::max (8, std::min (kLook, (int) (0.002 * sampleRate)));
        reset();
    }
    void reset()
    {
        for (int i = 0; i < kLook * 2; ++i) { dl[i] = dr[i] = 0; req[i] = 1; }
        w = 0; sum = (float) look; relGain = 1.0f; lastGr = 0.0f;
        std::fill (boxBuf, boxBuf + kLook, 1.0f); boxPos = 0;
    }
    // in place; returns max gain reduction in dB for the block
    float process (float* L, float* R, int n)
    {
        float maxGr = 0.0f;
        const float rc = std::exp (-1.0f / (0.08f * sr));
        for (int i = 0; i < n; ++i)
        {
            const int idx = w;
            dl[idx] = L[i]; dr[idx] = R[i];
            const float pk = std::max (std::fabs (L[i]), std::fabs (R[i]));
            req[idx] = pk > ceiling ? ceiling / pk : 1.0f;
            // minimum over the look-ahead window
            float m = 1.0f;
            for (int k = 0; k < look; ++k) m = std::min (m, req[(idx - k + 2 * kLook) % (2 * kLook)]);
            relGain = m < relGain ? m : relGain + (m - relGain) * (1.0f - rc);          // instant down, smooth release
            // box filter over the same window: gain reaches the minimum exactly when the peak is at the output
            sum += relGain - boxBuf[boxPos]; boxBuf[boxPos] = relGain; boxPos = (boxPos + 1) % look;
            const float g = sum / (float) look;
            const int outIdx = (idx - look + 1 + 2 * kLook) % (2 * kLook);
            float ol = dl[outIdx] * g, orr = dr[outIdx] * g;
            ol = std::max (-ceiling, std::min (ceiling, ol)); orr = std::max (-ceiling, std::min (ceiling, orr));   // last-resort clamp
            L[i] = ol; R[i] = orr;
            maxGr = std::max (maxGr, -linToDb (g));
            w = (w + 1) % (2 * kLook);
        }
        lastGr = maxGr;
        return maxGr;
    }
    int latency() const { return look - 1; }

    float sr = 48000.0f, relGain = 1.0f, lastGr = 0.0f, sum = 0;
    int look = kLook, w = 0, boxPos = 0;
    float dl[kLook * 2] = {}, dr[kLook * 2] = {}, req[kLook * 2] = {}, boxBuf[kLook] = {};
};
} // namespace duo
