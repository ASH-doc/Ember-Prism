// EMBER (Synth A) - its own effects section. Warm, slightly dark, analog-flavoured. Nothing here is shared with Synth B or the host.
#pragma once
#include "SA_Dsp.h"

namespace sa
{
struct FxChain
{
    struct Settings
    {
        bool drvOn, chOn, phOn, dlOn, rvOn, eqOn, pumpOn;
        int drvType, chMode, dlTime, dlPP, pumpRate;
        float drvAmt, chRate, chDepth, chMix, phRate, phDepth, phFb, phMix, dlFb, dlTone, dlMix, rvSize, rvDamp, rvMix, eqLo, eqHi, pumpDepth, pumpRel;
    };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        chL.init ((int) (0.06f * sr)); chR.init ((int) (0.06f * sr));
        dlL.init ((int) (4.2f * sr));  dlR.init ((int) (4.2f * sr));
        static const int combT[8]  = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static const int apT[4]    = { 556, 441, 341, 225 };
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < 8; ++i) { combLen[ch][i] = (int) ((combT[i] + ch * 23) * sr / 44100.0f); comb[ch][i].assign ((size_t) combLen[ch][i] + 8, 0.0f); combPos[ch][i] = 0; combLp[ch][i] = 0.0f; }
            for (int i = 0; i < 4; ++i) { apLen[ch][i] = (int) ((apT[i] + ch * 23) * sr / 44100.0f); ap[ch][i].assign ((size_t) apLen[ch][i] + 8, 0.0f); apPos[ch][i] = 0; }
        }
        reset();
    }

    void reset()
    {
        chL.clear(); chR.clear(); dlL.clear(); dlR.clear();
        for (int ch = 0; ch < 2; ++ch) { for (auto& c : comb[ch]) std::fill (c.begin(), c.end(), 0.0f); for (auto& a : ap[ch]) std::fill (a.begin(), a.end(), 0.0f);
            for (auto& l : combLp[ch]) l = 0.0f; }
        for (auto& a : phZ) a = 0.0f; phFbState[0] = phFbState[1] = 0; chLfo = phLfo = 0; chFbL = chFbR = 0;
        loL.reset(); loR.reset(); hiL.reset(); hiR.reset(); dlLpL = dlLpR = 0; tapeLp[0] = tapeLp[1] = 0;
        mixDrv = mixCh = mixPh = mixDl = mixRv = mixEq = mixPump = 0.0f;
    }

    void process (float* L, float* R, int n, const Settings& s, double bpm, double beatPos)
    {
        const float step = (float) n;
        // ---------------- drive (tube / tape / hard)
        if (s.drvOn || mixDrv > 0.001f)
        {
            const float target = s.drvOn ? 1.0f : 0.0f, m0 = mixDrv, m1 = target;
            const float gain = 1.0f + s.drvAmt * 9.0f, comp = 1.0f / std::sqrt (gain), bias = s.drvType == 0 ? 0.18f * s.drvAmt : 0.0f;
            const float lp = s.drvType == 1 ? 0.35f : 1.0f;
            for (int i = 0; i < n; ++i)
            {
                const float m = m0 + (m1 - m0) * (float) (i + 1) / step;
                float l = L[i], r = R[i];
                float* ch[2] = { &l, &r };
                for (int c = 0; c < 2; ++c)
                {
                    float x = *ch[c] * gain + bias, y;
                    if (s.drvType == 2) y = std::max (-1.0f, std::min (1.0f, x * 1.3f)) * 0.9f + 0.1f * fastTanh (x);
                    else y = fastTanh (x) - fastTanh (bias);
                    tapeLp[c] += lp * (y - tapeLp[c]); y = s.drvType == 1 ? tapeLp[c] : y;
                    *ch[c] = y * comp;
                }
                L[i] += (l - L[i]) * m; R[i] += (r - R[i]) * m;
            }
            mixDrv = m1;
        }

        // ---------------- chorus / flanger
        if (s.chOn || mixCh > 0.001f)
        {
            const float m0 = mixCh, m1 = s.chOn ? s.chMix : 0.0f;
            const bool fl = s.chMode == 1;
            const float base = (fl ? 0.0016f : 0.012f) * sr, depth = (fl ? 0.0012f : 0.0045f) * s.chDepth * sr + 1.0f, fb = fl ? 0.55f : 0.0f;
            for (int i = 0; i < n; ++i)
            {
                chLfo += s.chRate / sr; if (chLfo >= 1.0f) chLfo -= 1.0f;
                const float l1 = std::sin (2.0f * kPi * chLfo), l2 = std::sin (2.0f * kPi * (chLfo + 0.25f));
                chL.push (L[i] + chFbL * fb); chR.push (R[i] + chFbR * fb);
                const float wl = chL.read (base + depth * (0.5f + 0.5f * l1) + 1.0f), wr = chR.read (base + depth * (0.5f + 0.5f * l2) + 1.0f);
                chFbL = flushDenorm (wl); chFbR = flushDenorm (wr);
                const float m = m0 + (m1 - m0) * (float) (i + 1) / step;
                L[i] += (wl - 0.0f) * m * (fl ? 0.8f : 1.0f); R[i] += wr * m * (fl ? 0.8f : 1.0f);
            }
            mixCh = m1;
        }

        // ---------------- phaser (4 all-pass stages, one-pole)
        if (s.phOn || mixPh > 0.001f)
        {
            const float m0 = mixPh, m1 = s.phOn ? s.phMix : 0.0f;
            for (int i = 0; i < n; ++i)
            {
                phLfo += s.phRate / sr; if (phLfo >= 1.0f) phLfo -= 1.0f;
                const float sw = 0.5f + 0.5f * std::sin (2.0f * kPi * phLfo);
                const float f = 180.0f * std::pow (2.0f, 4.2f * sw * s.phDepth + 0.4f * (1.0f - s.phDepth));
                const float t = std::tan (kPi * std::min (f, 0.4f * sr) / sr), a = (t - 1.0f) / (t + 1.0f);
                for (int c = 0; c < 2; ++c)
                {
                    float* p = c ? &R[i] : &L[i];
                    float x = *p + phFbState[c] * s.phFb;
                    for (int st = 0; st < 4; ++st)
                    {
                        const float y = a * x + phZ[c * 4 + st];
                        phZ[c * 4 + st] = flushDenorm (x - a * y);
                        x = y;
                    }
                    phFbState[c] = fastTanh (x);
                    const float m = m0 + (m1 - m0) * (float) (i + 1) / step;
                    *p += (x - *p) * m * 0.7f;
                }
            }
            mixPh = m1;
        }

        // ---------------- tone EQ (low + high shelf)
        if (s.eqOn || mixEq > 0.001f)
        {
            const float m1 = s.eqOn ? 1.0f : 0.0f;
            Biquad cl, ch; cl.shelf (false, 120.0f, s.eqLo, sr); ch.shelf (true, 6000.0f, s.eqHi, sr);
            // copy coefficients into the runtime filters while keeping their own state
            auto setC = [] (Biquad& d, const Biquad& c) { d.b0 = c.b0; d.b1 = c.b1; d.b2 = c.b2; d.a1 = c.a1; d.a2 = c.a2; };
            setC (loL, cl); setC (loR, cl); setC (hiL, ch); setC (hiR, ch);
            for (int i = 0; i < n; ++i)
            {
                const float m = mixEq + (m1 - mixEq) * (float) (i + 1) / step;
                const float l = hiL.run (loL.run (L[i])), r = hiR.run (loR.run (R[i]));
                L[i] += (l - L[i]) * m; R[i] += (r - R[i]) * m;
            }
            mixEq = m1;
        }

        // ---------------- analog-voiced delay (stereo or ping-pong), dark filtered feedback
        if (s.dlOn || mixDl > 0.001f)
        {
            static const float beats[6] = { 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
            const float target = std::min (4.0f * sr - 8.0f, (float) (beats[std::max (0, std::min (5, s.dlTime))] * 60.0 / std::max (30.0, bpm)) * sr);
            const float m0 = mixDl, m1 = s.dlOn ? s.dlMix : 0.0f;
            const float lpc = 0.08f + 0.9f * s.dlTone * s.dlTone;
            for (int i = 0; i < n; ++i)
            {
                dlTime += (target - dlTime) * 0.0004f;
                const float wl = dlL.read (dlTime), wr = dlR.read (dlTime);
                dlLpL += lpc * (wl - dlLpL); dlLpR += lpc * (wr - dlLpR);
                const float fl = fastTanh (dlLpL * s.dlFb), fr = fastTanh (dlLpR * s.dlFb);
                if (s.dlPP == 1) { dlL.push (flushDenorm ((L[i] + R[i]) * 0.5f + fr)); dlR.push (flushDenorm (fl)); }
                else             { dlL.push (flushDenorm (L[i] + fl)); dlR.push (flushDenorm (R[i] + fr)); }
                const float m = m0 + (m1 - m0) * (float) (i + 1) / step;
                L[i] += dlLpL * m; R[i] += dlLpR * m;
            }
            mixDl = m1;
        }

        // ---------------- dark plate reverb (8 damped combs + 4 all-passes per side)
        if (s.rvOn || mixRv > 0.001f)
        {
            const float m0 = mixRv, m1 = s.rvOn ? s.rvMix : 0.0f;
            const float fb = 0.70f + 0.28f * s.rvSize, damp = 0.15f + 0.8f * s.rvDamp;
            for (int i = 0; i < n; ++i)
            {
                const float in = (L[i] + R[i]) * 0.015f;
                float o[2] = { 0, 0 };
                for (int c = 0; c < 2; ++c)
                {
                    for (int k = 0; k < 8; ++k)
                    {
                        float& y = comb[c][k][(size_t) combPos[c][k]];
                        combLp[c][k] = y * (1.0f - damp) + combLp[c][k] * damp;
                        const float v = y; y = flushDenorm (in + combLp[c][k] * fb);
                        if (++combPos[c][k] >= combLen[c][k]) combPos[c][k] = 0;
                        o[c] += v;
                    }
                    for (int k = 0; k < 4; ++k)
                    {
                        float& b = ap[c][k][(size_t) apPos[c][k]];
                        const float v = b, y = -o[c] + v; b = flushDenorm (o[c] + v * 0.5f); o[c] = y;
                        if (++apPos[c][k] >= apLen[c][k]) apPos[c][k] = 0;
                    }
                }
                const float m = m0 + (m1 - m0) * (float) (i + 1) / step;
                L[i] += o[0] * m * 3.0f; R[i] += o[1] * m * 3.0f;
            }
            mixRv = m1;
        }

        // ---------------- pump (tempo-synced ducker)
        if (s.pumpOn || mixPump > 0.001f)
        {
            const float m1 = s.pumpOn ? 1.0f : 0.0f;
            const double bl = s.pumpRate == 0 ? 1.0 : 0.5;
            for (int i = 0; i < n; ++i)
            {
                const double pos = beatPos + (double) i * bpm / 60.0 / sr;
                const float ph = (float) std::fmod (pos / bl, 1.0);
                const float env = 1.0f - std::exp (-ph / (0.04f + 0.4f * s.pumpRel));
                const float g = 1.0f - s.pumpDepth * (1.0f - env);
                const float m = mixPump + (m1 - mixPump) * (float) (i + 1) / step;
                const float gg = 1.0f + (g - 1.0f) * m;
                L[i] *= gg; R[i] *= gg;
            }
            mixPump = m1;
        }
    }

private:
    float sr = 48000.0f;
    DelayLine chL, chR, dlL, dlR;
    float chLfo = 0, phLfo = 0, chFbL = 0, chFbR = 0, dlTime = 12000.0f, dlLpL = 0, dlLpR = 0, tapeLp[2] = { 0, 0 };
    float phZ[8] = {}, phFbState[2] = {};
    Biquad loL, loR, hiL, hiR;
    std::vector<float> comb[2][8], ap[2][4]; int combLen[2][8] = {}, combPos[2][8] = {}, apLen[2][4] = {}, apPos[2][4] = {}; float combLp[2][8] = {};
    float mixDrv = 0, mixCh = 0, mixPh = 0, mixDl = 0, mixRv = 0, mixEq = 0, mixPump = 0;
};
} // namespace sa
