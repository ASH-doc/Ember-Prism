// PRISM (Synth B) - its own 8-slot effects rack. Every slot can hold any module, so the order is freely re-arrangeable.
// Clean, wide and airy. Nothing here is shared with Synth A or the host.
#pragma once
#include "SB_Dsp.h"

namespace sb
{
enum FxType { FX_NONE, FX_CHORUS, FX_PHASER, FX_DELAY, FX_REVERB, FX_DIST, FX_EQ, FX_COMP, FX_FILTER, FX_WIDENER, FX_FLANGER };

struct RackSlot
{
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        chL.init ((int) (0.05f * sr)); chR.init ((int) (0.05f * sr)); flL.init ((int) (0.02f * sr)); flR.init ((int) (0.02f * sr));
        dlL.init ((int) (2.1f * sr)); dlR.init ((int) (2.1f * sr)); haas.init ((int) (0.03f * sr));
        static const int base[8] = { 1129, 1327, 1559, 1831, 2053, 2339, 2621, 2917 };
        for (int i = 0; i < 8; ++i) { rvBase[i] = (int) ((float) base[i] * sr / 48000.0f); rv[i].init ((int) ((float) rvBase[i] * 1.6f) + 64); }
        static const int apT[2] = { 241, 373 };
        for (int i = 0; i < 2; ++i) for (int c = 0; c < 2; ++c) ap[c][i].init ((int) ((float) apT[i] * sr / 48000.0f) + 8);
        reset();
    }

    void reset()
    {
        chL.clear(); chR.clear(); flL.clear(); flR.clear(); dlL.clear(); dlR.clear(); haas.clear();
        for (auto& d : rv) d.clear();
        for (auto& c : ap) for (auto& d : c) d.clear();
        for (auto& v : rvLp) v = 0;
        for (auto& z : phZ) z = 0;
        phFb[0] = phFb[1] = 0; lfoA = lfoB = 0; fbL = fbR = 0; dlLpL = dlLpR = 0; compEnv = 0; compGain = 1; toneLp[0] = toneLp[1] = 0;
        for (auto& b : eqB) b.reset();
        for (auto& s : fS) s = 0;
        sHp = 0; airL = airR = 0;
        mixCur = 0; lastType = -1;
    }

    void process (float* L, float* R, int n, int type, bool on, float a, float b, float c, float mix, double bpm)
    {
        if (type != lastType) { reset(); lastType = type; }
        const float target = (type != FX_NONE && on) ? mix : 0.0f;
        if (target < 0.0005f && mixCur < 0.0005f) { mixCur = 0; return; }

        float wl[16], wr[16];
        for (int i = 0; i < n; ++i) { wl[i] = L[i]; wr[i] = R[i]; }
        run (type, wl, wr, n, a, b, c, bpm);
        const float m0 = mixCur;
        for (int i = 0; i < n; ++i)
        {
            const float m = m0 + (target - m0) * (float) (i + 1) / (float) n;
            L[i] += (wl[i] - L[i]) * m; R[i] += (wr[i] - R[i]) * m;
        }
        mixCur = target;
    }

private:
    void run (int type, float* L, float* R, int n, float a, float b, float c, double bpm)
    {
        switch (type)
        {
            case FX_CHORUS:
            {
                const float rate = 0.1f + a * 2.5f, depth = 0.0007f * sr + b * 0.0045f * sr;
                for (int i = 0; i < n; ++i)
                {
                    lfoA += rate / sr; if (lfoA >= 1) lfoA -= 1;
                    chL.push (L[i]); chR.push (R[i]);
                    float oL = 0, oR = 0;
                    for (int v = 0; v < 3; ++v)
                    {
                        const float ph = lfoA + (float) v / 3.0f;
                        const float sL = std::sin (2 * kPi * ph), sR = std::sin (2 * kPi * (ph + 0.25f * c));
                        oL += chL.read (0.013f * sr + depth * (0.5f + 0.5f * sL) + 1); oR += chR.read (0.013f * sr + depth * (0.5f + 0.5f * sR) + 1);
                    }
                    L[i] = oL * 0.45f + L[i] * 0.3f; R[i] = oR * 0.45f + R[i] * 0.3f;
                }
                break;
            }
            case FX_FLANGER:
            {
                const float rate = 0.05f + a * 1.5f, depth = 0.0002f * sr + b * 0.0030f * sr, fb = c * 0.85f;
                for (int i = 0; i < n; ++i)
                {
                    lfoA += rate / sr; if (lfoA >= 1) lfoA -= 1;
                    flL.push (L[i] + fbL * fb); flR.push (R[i] + fbR * fb);
                    const float dL = flL.read (depth * (0.5f + 0.5f * std::sin (2 * kPi * lfoA)) + 2), dR = flR.read (depth * (0.5f + 0.5f * std::sin (2 * kPi * (lfoA + 0.25f))) + 2);
                    fbL = flushDenorm (dL); fbR = flushDenorm (dR);
                    L[i] += dL * 0.7f; R[i] += dR * 0.7f; L[i] *= 0.6f; R[i] *= 0.6f;
                }
                break;
            }
            case FX_PHASER:
            {
                const float rate = 0.05f + a * 3.0f, fb = c * 0.85f;
                for (int i = 0; i < n; ++i)
                {
                    lfoA += rate / sr; if (lfoA >= 1) lfoA -= 1;
                    const float sw = 0.5f + 0.5f * std::sin (2 * kPi * lfoA);
                    const float f = 200.0f * std::exp2 (4.3f * sw * b + 0.3f);
                    const float t = std::tan (kPi * std::min (f, 0.4f * sr) / sr), k = (t - 1) / (t + 1);
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        float x = (ch ? R[i] : L[i]) + phFb[ch] * fb;
                        for (int s = 0; s < 6; ++s) { const float y = k * x + phZ[ch * 6 + s]; phZ[ch * 6 + s] = flushDenorm (x - k * y); x = y; }
                        phFb[ch] = x;
                        (ch ? R[i] : L[i]) = 0.5f * ((ch ? R[i] : L[i]) + x);
                    }
                }
                break;
            }
            case FX_DELAY:
            {
                static const float beats[6] = { 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
                const int idx = std::max (0, std::min (5, (int) (a * 5.999f)));
                const float tgt = std::min (2.0f * sr, (float) (beats[idx] * 60.0 / std::max (30.0, bpm)) * sr), fb = b * 0.9f;
                for (int i = 0; i < n; ++i)
                {
                    dlTime += (tgt - dlTime) * 0.0005f;
                    const float wl = dlL.read (dlTime), wr = dlR.read (dlTime);
                    dlLpL += 0.55f * (wl - dlLpL); dlLpR += 0.55f * (wr - dlLpR);
                    dlL.push (flushDenorm (L[i] + (c * dlLpR + (1 - c) * dlLpL) * fb));
                    dlR.push (flushDenorm (R[i] + (c * dlLpL + (1 - c) * dlLpR) * fb));
                    L[i] = wl; R[i] = wr;
                }
                break;
            }
            case FX_REVERB:
            {
                const float rt60 = 0.35f + a * a * 9.0f, damp = 0.05f + 0.75f * (1.0f - b), air = c;
                const float sz = 0.6f + 0.4f * std::min (1.0f, a * 2.0f);
                float g[8]; int len[8];
                for (int j = 0; j < 8; ++j) { len[j] = std::max (8, (int) ((float) rvBase[j] * sz)); g[j] = std::pow (10.0f, -3.0f * (float) len[j] / (sr * rt60)); }
                for (int i = 0; i < n; ++i)
                {
                    float x = (L[i] + R[i]) * 0.5f;
                    for (int s = 0; s < 2; ++s)    // input diffusion
                    {
                        const float d = ap[0][s].read ((float) (s ? 373 : 241) * sr / 48000.0f);
                        const float y = -0.6f * x + d; ap[0][s].push (flushDenorm (x + 0.6f * y)); x = y;
                    }
                    float t[8];
                    for (int j = 0; j < 8; ++j)
                    {
                        const float m = 1.0f + 0.0007f * air * std::sin (2 * kPi * (lfoB + (float) j * 0.13f));
                        t[j] = rv[j].read ((float) len[j] * m);
                        rvLp[j] += (1.0f - damp) * (t[j] - rvLp[j]); t[j] = rvLp[j] * g[j];
                    }
                    lfoB += 0.23f / sr; if (lfoB >= 1) lfoB -= 1;
                    // 8x8 Hadamard mix (orthonormal)
                    for (int s = 1; s < 8; s <<= 1)
                        for (int j = 0; j < 8; j += 2 * s)
                            for (int k = j; k < j + s; ++k) { const float u = t[k], v = t[k + s]; t[k] = u + v; t[k + s] = u - v; }
                    const float norm = 0.35355339f;
                    for (int j = 0; j < 8; ++j) rv[j].push (flushDenorm (x * 0.5f + t[j] * norm));
                    float oL = (t[0] + t[2] + t[4] + t[6]) * 0.3f, oR = (t[1] + t[3] + t[5] + t[7]) * 0.3f;
                    airL += 0.25f * (oL - airL); airR += 0.25f * (oR - airR);        // brighten: add back the high part
                    oL += (oL - airL) * air * 1.5f; oR += (oR - airR) * air * 1.5f;
                    L[i] = oL; R[i] = oR;
                }
                break;
            }
            case FX_DIST:
            {
                const float drive = 1.0f + a * 14.0f, tone = 0.15f + 0.85f * c, comp = 1.0f / std::sqrt (drive);
                const float levels = std::exp2 (3.0f + (1.0f - b) * 9.0f);
                for (int i = 0; i < n; ++i)
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        float x = (ch ? R[i] : L[i]) * drive, y;
                        if (b < 0.4f) y = fastTanh (x);
                        else if (b < 0.75f) { const float t = (b - 0.4f) / 0.35f; const float fold = std::sin (x * 1.4f); y = fastTanh (x) * (1 - t) + fold * t; }
                        else { const float t = (b - 0.75f) / 0.25f; y = fastTanh (x) * (1 - t) + (std::round (std::max (-1.0f, std::min (1.0f, x)) * levels) / levels) * t; }
                        toneLp[ch] += tone * (y * comp - toneLp[ch]);
                        (ch ? R[i] : L[i]) = toneLp[ch];
                    }
                break;
            }
            case FX_EQ:
            {
                eqB[0].setShelf (false, 150.0f, (a - 0.5f) * 24.0f, sr); eqB[1].copyCoef (eqB[0]);
                eqB[2].setPeak (1200.0f, (b - 0.5f) * 24.0f, 0.8f, sr);   eqB[3].copyCoef (eqB[2]);
                eqB[4].setShelf (true, 6500.0f, (c - 0.5f) * 24.0f, sr);  eqB[5].copyCoef (eqB[4]);
                // separate states per channel: biquads 0,2,4 = left, 1,3,5 = right
                for (int i = 0; i < n; ++i)
                {
                    L[i] = eqB[4].run (eqB[2].run (eqB[0].run (L[i])));
                    R[i] = eqB[5].run (eqB[3].run (eqB[1].run (R[i])));
                }
                break;
            }
            case FX_COMP:
            {
                const float thr = -40.0f + a * 40.0f, ratio = 1.5f + b * 8.5f, makeup = std::pow (10.0f, c * 12.0f / 20.0f);
                const float aC = std::exp (-1.0f / (0.008f * sr)), rC = std::exp (-1.0f / (0.12f * sr));
                for (int i = 0; i < n; ++i)
                {
                    const float lvl = std::max (std::fabs (L[i]), std::fabs (R[i]));
                    compEnv = lvl > compEnv ? aC * compEnv + (1 - aC) * lvl : rC * compEnv + (1 - rC) * lvl;
                    const float db = 20.0f * std::log10 (compEnv + 1e-9f);
                    const float over = db - thr, red = over > 0 ? over * (1.0f - 1.0f / ratio) : 0.0f;
                    const float gt = std::pow (10.0f, -red / 20.0f);
                    compGain += (gt - compGain) * 0.1f;
                    L[i] *= compGain * makeup; R[i] *= compGain * makeup;
                }
                break;
            }
            case FX_FILTER:
            {
                const float fc = 40.0f * std::pow (400.0f, a), q = 2.0f * (1.0f - b * 0.93f) + 0.05f;
                const float gg = std::tan (kPi * std::min(fc, 0.45f * sr) / sr), a1 = 1.0f / (1.0f + gg * (gg + q)), a2 = gg * a1, a3 = gg * a2;
                for (int i = 0; i < n; ++i)
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        float& s1 = fS[ch * 2]; float& s2 = fS[ch * 2 + 1];
                        const float x = ch ? R[i] : L[i], v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
                        s1 = flushDenorm (2 * v1 - s1); s2 = flushDenorm (2 * v2 - s2);
                        const float lp = v2, bp = v1 * q, hp = x - q * v1 - v2;
                        (ch ? R[i] : L[i]) = c < 0.5f ? lp + (bp - lp) * (c * 2.0f) : bp + (hp - bp) * ((c - 0.5f) * 2.0f);
                    }
                break;
            }
            case FX_WIDENER:
            {
                const float width = a * 2.0f, hz = 40.0f + 260.0f * c, hpc = std::exp (-2 * kPi * hz / sr);
                for (int i = 0; i < n; ++i)
                {
                    float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]);
                    sHp = (1 - hpc) * s + hpc * sHp;               // low part of the side signal
                    s = s - sHp * (c > 0.01f ? 1.0f : 0.0f);       // bass mono
                    s *= width;
                    float l = m + s, r = m - s;
                    haas.push (r);
                    r = r * (1 - b * 0.5f) + haas.read (1.0f + b * 0.015f * sr) * (b * 0.5f);
                    L[i] = l; R[i] = r;
                }
                break;
            }
            default: break;
        }
    }

    float sr = 48000.0f;
    DelayLine chL, chR, flL, flR, dlL, dlR, haas, rv[8], ap[2][2];
    int rvBase[8] = {};
    float rvLp[8] = {}, phZ[12] = {}, phFb[2] = {}, lfoA = 0, lfoB = 0, fbL = 0, fbR = 0, dlTime = 12000.0f, dlLpL = 0, dlLpR = 0;
    float compEnv = 0, compGain = 1, toneLp[2] = {}, fS[4] = {}, sHp = 0, airL = 0, airR = 0, mixCur = 0;
    Biquad eqB[6];
    int lastType = -1;
};
} // namespace sb
