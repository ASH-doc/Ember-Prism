#include "SB_Engine.h"

namespace sb
{
static float midiToHz (float n) { return 440.0f * std::exp2 ((n - 69.0f) / 12.0f); }
static const float kLfoCyclesPerBeat[8] = { 0, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 3.0f, 6.0f };
static const double kArpBeats[5] = { 1.0, 0.5, 0.25, 1.0 / 6.0, 0.125 };
static const double kSeqBeats[3] = { 0.5, 0.25, 0.125 };
static inline float frac1 (float x) { return x - std::floor (x); }

Engine::Engine()
{
    generateWavetables();
    for (int i = 0; i < COUNT; ++i) { pr[i] = sm[i] = defs()[i].def; isFloat[i] = defs()[i].choices[0] == 0; }
    for (auto& s : scope) s.store (0.0f);
    for (int l = 0; l < 4; ++l) for (int p = 0; p < 16; ++p) custom[(size_t) (l * 16 + p)].store (std::sin (2.0f * kPi * (float) p / 16.0f));
    for (auto& m : modShown) m.store (0.0f);
    for (auto& m : srcShown) m.store (0.0f);
    for (int i = 0; i < kVoices; ++i) voices[i].rng.s = 0x7F4A7C15u * (uint32_t) (i + 1);
    for (int i = 0; i < 4; ++i) lfos[i].rng.s = 0xC2B2AE35u * (uint32_t) (i + 2);
}

void Engine::prepare (double sampleRate, int)
{
    sr = (float) sampleRate;
    for (auto& r : rack) r.prepare (sampleRate);
    panic();
}

void Engine::readScope (float* dest, int count) const
{
    const int p = scopePos.load();
    for (int i = 0; i < count; ++i) dest[i] = scope[(size_t) ((p - count + i + kScope * 2) % kScope)].load();
}

// ------------------------------------------------------------------------------------ note handling
void Engine::startVoice (Voice& v, int key, float note, float vel, bool retrig)
{
    const bool wasActive = v.active && v.env[0].active();
    v.active = true; v.gate = true; v.sustained = false; v.stealing = false; v.stealGain = 1.0f;
    v.key = key; v.target = note; v.vel = vel; v.order = ++orderCounter; v.rnd = v.rng.next();
    if (ch (play_mode) == 0 || ! wasActive || sm[glide] <= 0.0001f) v.note = note;
    if (! wasActive)
    {
        const bool rand1 = ch (o1_rand) != 0, rand2 = ch (o2_rand) != 0;
        for (int i = 0; i < kMaxUni; ++i)
        {
            v.ph1[i] = rand1 || i > 0 ? v.rng.uni() : pr[o1_phase];
            v.ph2[i] = rand2 || i > 0 ? v.rng.uni() : pr[o2_phase];
            v.y1prev[i] = v.y2prev[i] = 0.0f;
        }
        v.phSub = 0.0f; v.lastAmp = 0.0f; v.airLp = 0.0f;
    }
    if (retrig || ! wasActive) for (auto& e : v.env) e.gateOn();
}

void Engine::voiceOn (int key, float note, float vel)
{
    const int mode = ch (play_mode);
    if (mode != 0)
    {
        Voice& v = voices[0];
        const bool join = mode == 2 && v.active && v.gate;
        startVoice (v, key, note, vel, ! join);
        return;
    }
    Voice* pick = nullptr;
    for (auto& v : voices) if (! v.active) { pick = &v; break; }
    if (pick) { startVoice (*pick, key, note, vel, true); return; }
    float best = 1e9f;
    for (auto& v : voices) if (! v.gate && ! v.stealing && v.env[0].v < best) { best = v.env[0].v; pick = &v; }
    if (! pick) { uint32_t oldest = 0xFFFFFFFFu; for (auto& v : voices) if (! v.stealing && v.order < oldest) { oldest = v.order; pick = &v; } }
    if (! pick) pick = &voices[0];
    pick->stealing = true; pick->pKey = key; pick->pNote = note; pick->pVel = vel;
}

void Engine::trigger (int key, float vel) { voiceOn (key, (float) key, vel); }

void Engine::releaseKey (int key)
{
    const int mode = ch (play_mode);
    if (mode != 0)
    {
        monoStack.erase (std::remove_if (monoStack.begin(), monoStack.end(), [key] (auto& p) { return p.first == key; }), monoStack.end());
        Voice& v = voices[0];
        if (! v.active || v.key != key) return;
        if (! monoStack.empty())
        {
            auto back = monoStack.back();
            startVoice (v, back.first, (float) back.first, back.second, mode == 1);
            return;
        }
    }
    for (auto& v : voices)
        if (v.active && v.gate && v.key == key)
        {
            if (sustain) v.sustained = true;
            else { v.gate = false; for (auto& e : v.env) e.gateOff(); }
        }
}

void Engine::noteOn (int note, float velocity, int)
{
    const float vel = std::max (0.02f, std::min (1.0f, velocity));
    if (std::find (heldKeys.begin(), heldKeys.end(), note) == heldKeys.end()) heldKeys.push_back (note);
    if (ch (seq_on)) return;
    if (ch (arp_on))
    {
        if (ch (arp_hold) && heldKeys.size() == 1) arpHeld.clear();
        if (std::find (arpHeld.begin(), arpHeld.end(), note) == arpHeld.end()) arpHeld.push_back (note);
        arpDirty = true;
        return;
    }
    if (ch (play_mode) != 0) { monoStack.erase (std::remove_if (monoStack.begin(), monoStack.end(), [note] (auto& p) { return p.first == note; }), monoStack.end()); monoStack.push_back ({ note, vel }); }
    trigger (note, vel);
}

void Engine::noteOff (int note, int)
{
    heldKeys.erase (std::remove (heldKeys.begin(), heldKeys.end(), note), heldKeys.end());
    if (ch (seq_on)) return;
    if (ch (arp_on))
    {
        if (! ch (arp_hold)) { arpHeld.erase (std::remove (arpHeld.begin(), arpHeld.end(), note), arpHeld.end()); arpDirty = true; }
        return;
    }
    releaseKey (note);
}

void Engine::controlChange (int cc, int value)
{
    if (cc == 1) modWheel = (float) value / 127.0f;
    else if (cc == 64)
    {
        const bool down = value >= 64;
        if (sustain && ! down)
            for (auto& v : voices) if (v.sustained) { v.sustained = false; v.gate = false; for (auto& e : v.env) e.gateOff(); }
        sustain = down;
    }
    else if (cc == 120) panic();
    else if (cc == 123) allNotesOff();
}

void Engine::allNotesOff()
{
    sustain = false; heldKeys.clear(); monoStack.clear(); arpHeld.clear(); arpDirty = true;
    arpKeyOn = -1; arpChordOn.clear(); seqKeyOn = -1;
    for (auto& v : voices)
        if (v.active) { v.gate = false; v.sustained = false; v.stealing = false; for (auto& e : v.env) e.gateOff(); }
}

void Engine::panic()
{
    allNotesOff();
    for (auto& v : voices)
    {
        v.active = false; v.stealing = false; for (auto& e : v.env) e.kill();
        for (auto& f : v.f1) f.reset();
        for (auto& f : v.f2) f.reset();
    }
    for (auto& r : rack) r.reset();
    bend = 0; modWheel = 0; at = 0;
}

// ------------------------------------------------------------------------------------ arpeggiator + step sequencer
void Engine::rebuildArp()
{
    arpSeq.clear();
    std::vector<int> base (arpHeld);
    const int mode = ch (arp_mode), octs = std::max (1, std::min (4, (int) (pr[arp_oct] + 0.5f)));
    if (mode != 4) std::sort (base.begin(), base.end());
    std::vector<int> up;
    for (int o = 0; o < octs; ++o) for (int k : base) up.push_back (k + 12 * o);
    if (mode == 1) { arpSeq = up; std::reverse (arpSeq.begin(), arpSeq.end()); }
    else if (mode == 2) { arpSeq = up; for (int i = (int) up.size() - 2; i >= 1; --i) arpSeq.push_back (up[(size_t) i]); }
    else arpSeq = up;
    if (arpIdx >= (int) arpSeq.size()) arpIdx = 0;
    arpDirty = false;
}

void Engine::arpUpdate()
{
    auto stopArp = [this]
    {
        if (arpKeyOn >= 0) { releaseKey (arpKeyOn); arpKeyOn = -1; }
        for (int k : arpChordOn) releaseKey (k);
        arpChordOn.clear();
    };
    if (! ch (arp_on) || ch (seq_on)) { if (arpKeyOn >= 0 || ! arpChordOn.empty()) stopArp(); arpLast = -1; return; }
    if (arpDirty) rebuildArp();

    const double stepBeats = kArpBeats[std::max (0, std::min (4, ch (arp_rate)))];
    const double pos = beatPos / stepBeats;
    const long stepIdx = (long) std::floor (pos);
    const float fr = (float) (pos - (double) stepIdx);
    const float swOff = (stepIdx & 1) ? pr[arp_swing] * 0.5f : 0.0f;

    if (stepIdx != arpLast && fr >= swOff)
    {
        arpLast = stepIdx;
        stopArp();
        if (! arpSeq.empty())
        {
            if (ch (arp_mode) == 5) { for (int k : arpHeld) { trigger (k, 0.85f); arpChordOn.push_back (k); } }
            else
            {
                int i;
                if (ch (arp_mode) == 3) i = (int) (rng.uni() * (float) arpSeq.size()) % (int) arpSeq.size();
                else { i = arpIdx % (int) arpSeq.size(); arpIdx = (arpIdx + 1) % (int) arpSeq.size(); }
                arpKeyOn = arpSeq[(size_t) i]; arpStepIdx.store (i);
                trigger (arpKeyOn, 0.85f);
            }
        }
    }
    else if ((arpKeyOn >= 0 || ! arpChordOn.empty()) && fr >= swOff + pr[arp_gate] * (1.0f - swOff)) stopArp();
}

void Engine::seqUpdate()
{
    auto stopSeq = [this] { if (seqKeyOn >= 0) { releaseKey (seqKeyOn); seqKeyOn = -1; } };
    if (! ch (seq_on)) { stopSeq(); seqLast = -1; return; }
    if (heldKeys.empty()) { stopSeq(); seqLast = -1; return; }
    const double stepBeats = kSeqBeats[std::max (0, std::min (2, ch (seq_rate)))];
    const double pos = beatPos / stepBeats;
    const long idx = (long) std::floor (pos);
    const float fr = (float) (pos - (double) idx);
    const int len = std::max (1, std::min (16, (int) (pr[seq_len] + 0.5f)));
    if (idx != seqLast)
    {
        seqLast = idx;
        stopSeq();
        const int s = (int) (idx % len);
        seqStepIdx.store (s);
        if (pr[sg1 + 3 * s] > 0.5f)
        {
            const int key = std::max (0, std::min (127, heldKeys.back() + (int) std::round (pr[sp1 + 3 * s])));
            trigger (key, 0.3f + 0.7f * pr[sv1 + 3 * s]);
            seqKeyOn = key;
        }
    }
    else if (seqKeyOn >= 0 && fr > 0.7f) stopSeq();
}

// ------------------------------------------------------------------------------------ voice rendering
namespace
{
struct OscCfg
{
    const WaveTable* wt = nullptr;
    int mip = 0, w[2] = { 0, 0 };
    float wa[2] = { 0, 0 }, lvl = 0;
    float pos[kMaxUni], inc[kMaxUni], gl[kMaxUni], gr[kMaxUni];
    bool used = false;
};

inline float evalOsc (const OscCfg& c, int u, float ph, float other)
{
    float t = ph;
    for (int k = 0; k < 2; ++k)
    {
        const float a = c.wa[k];
        switch (c.w[k])
        {
            case 1: t = frac1 (t * (1.0f + a * 7.0f)); break;
            case 2: t = t + a * (t * t - t); break;
            case 3: t = t + a * (t - t * t); break;
            case 4: { const float m = t < 0.5f ? 2.0f * t : 2.0f - 2.0f * t; t = t + a * (m - t); } break;
            case 5: { const float pv = 0.5f + 0.45f * a; t = t < pv ? 0.5f * t / pv : 0.5f + 0.5f * (t - pv) / (1.0f - pv); } break;
            case 6: t = frac1 (t + a * 0.6f * other); break;
            default: break;
        }
    }
    float y = readTable (*c.wt, c.mip, c.pos[u], t);
    for (int k = 0; k < 2; ++k)
    {
        const float a = c.wa[k];
        switch (c.w[k])
        {
            case 7: y *= (1.0f - a) + a * 1.4f * other; break;
            case 8: y = (y - a * readTable (*c.wt, c.mip, c.pos[u], frac1 (t + 0.5f))) * (1.0f - 0.4f * a); break;
            case 9: { const float lv = std::exp2 (2.0f + (1.0f - a) * 10.0f); y = std::round (y * lv) / lv; } break;
            case 10: if (t > 1.0f - a) y = -y; break;
            default: break;
        }
    }
    return y;
}
} // namespace

void Engine::renderVoice (Voice& v, float* oL, float* oR, int n)
{
    #define Q(p) sm[p]
    const float fn = (float) n;

    if (v.stealing)
    {
        v.stealGain -= fn / 40.0f;
        if (v.stealGain <= 0.0f) startVoice (v, v.pKey, v.pNote, v.pVel, true);
    }
    const float e1 = v.env[0].step (n, Q (e1_a), Q (e1_d), Q (e1_s), Q (e1_r), sr);
    const float e2 = v.env[1].step (n, Q (e2_a), Q (e2_d), Q (e2_s), Q (e2_r), sr);
    const float e3 = v.env[2].step (n, Q (e3_a), Q (e3_d), Q (e3_s), Q (e3_r), sr);
    if (! v.env[0].active()) { v.active = false; return; }

    if (ch (play_mode) != 0 && Q (glide) > 0.0001f) v.note += (v.target - v.note) * (1.0f - std::exp (-fn / (Q (glide) * sr * 0.35f)));
    else v.note = v.target;

    // ---- modulation
    float src[S_COUNT] = {};
    for (int i = 0; i < 4; ++i) src[S_LFO1 + i] = lfoV[i];
    src[S_ENV1] = e1; src[S_ENV2] = e2; src[S_ENV3] = e3;
    for (int i = 0; i < 8; ++i) src[S_MAC1 + i] = sm[mac1 + i];
    src[S_VEL] = v.vel; src[S_KEY] = (v.note - 60.0f) / 48.0f; src[S_MOD] = modWheel; src[S_AT] = at; src[S_RAND] = v.rnd; src[S_BEND] = bend;
    float md[D_COUNT] = {};
    for (int s = 0; s < 10; ++s)
    {
        const int so = ch (m1_src + 3 * s), ds = ch (m1_dst + 3 * s);
        if (so > 0 && ds > 0) md[ds] += src[so] * Q (m1_amt + 3 * s);
    }
    if (v.order == orderCounter)
    {
        for (int d = 0; d < D_COUNT; ++d) modShown[(size_t) d].store (md[d]);
        for (int s = 0; s < S_COUNT; ++s) srcShown[(size_t) s].store (src[s]);
    }

    // ---- pitch
    const float baseSt = v.note + md[D_PITCH] * 24.0f + bend * Q (bend_range);
    const float st1 = baseSt + std::round (Q (o1_oct)) * 12.0f + std::round (Q (o1_semi)) + Q (o1_fine) / 100.0f + md[D_O1PITCH] * 24.0f;
    const float st2 = baseSt + std::round (Q (o2_oct)) * 12.0f + std::round (Q (o2_semi)) + Q (o2_fine) / 100.0f + md[D_O2PITCH] * 24.0f;
    const float stSub = baseSt - 12.0f * (1.0f + (float) ch (sub_oct));

    // ---- oscillator configuration (per chunk)
    const int nU = std::max (1, std::min (kMaxUni, (int) std::round (Q (uni_voices))));
    const float detune = std::max (0.0f, std::min (1.0f, Q (uni_detune) + md[D_DETUNE] * 0.5f));
    const float maxC = 80.0f * std::pow (detune, 1.4f), wts = std::max (0.0f, std::min (1.0f, Q (uni_wtspread) + md[D_WTSPREAD] * 0.5f));
    const int curve = ch (uni_curve);
    float uw[kMaxUni], ux[kMaxUni], sumSq = 0.0f;
    for (int i = 0; i < nU; ++i)
    {
        const float x = nU > 1 ? ((float) i - 0.5f * (float) (nU - 1)) / (0.5f * (float) (nU - 1)) : 0.0f;
        ux[i] = x;
        uw[i] = nU == 1 ? 1.0f : (std::fabs (x) < 1e-4f ? 1.15f - 0.45f * Q (uni_blend) : 0.3f + 0.7f * Q (uni_blend));
        sumSq += uw[i] * uw[i];
    }
    const float unorm = 1.0f / std::sqrt (sumSq);

    OscCfg oc[2];
    auto setupOsc = [&] (OscCfg& c, int which, float st)
    {
        const int base = which == 0 ? o1_table : o2_table;
        const int dPos = which == 0 ? D_O1POS : D_O2POS, dW1 = which == 0 ? D_O1W1 : D_O2W1, dW2 = which == 0 ? D_O1W2 : D_O2W2, dLvl = which == 0 ? D_O1LVL : D_O2LVL;
        c.wt = &wavetable (ch (base));
        const float pos0 = Q (base + 1) + md[dPos] * 0.5f;
        c.lvl = std::max (0.0f, Q (base + 5) + md[dLvl]);
        c.w[0] = ch (base + 9); c.w[1] = ch (base + 11);
        c.wa[0] = std::max (0.0f, std::min (1.0f, Q (base + 10) + md[dW1] * 0.5f));
        c.wa[1] = std::max (0.0f, std::min (1.0f, Q (base + 12) + md[dW2] * 0.5f));
        c.used = c.lvl > 0.0005f;
        const float hz = midiToHz (st);
        float wf = 1.0f;
        for (int k = 0; k < 2; ++k) { if (c.w[k] == 1) wf = std::max (wf, 1.0f + c.wa[k] * 7.0f); else if (c.w[k] == 2 || c.w[k] == 3) wf = std::max (wf, 1.0f + c.wa[k] * 2.0f); else if (c.w[k] == 4) wf = std::max (wf, 1.0f + c.wa[k]); else if (c.w[k] == 6) wf = std::max (wf, 1.5f + 3.0f * c.wa[k]); }
        const float hzMax = hz * wf * (1.0f + 0.04f * detune);
        c.mip = std::max (0, std::min (kMips - 1, (int) std::ceil (std::log2 (std::max (1e-3f, 256.0f * hzMax / (0.45f * sr))))));
        const float oPan = Q (base + 6);
        for (int i = 0; i < nU; ++i)
        {
            const float x = ux[i];
            const float dx = curve == 0 ? x : (curve == 1 ? x * std::fabs (x) : x * (0.35f + 0.65f * std::fabs (x)));
            c.inc[i] = std::min (0.45f, hz * std::exp2 (dx * maxC / 1200.0f) / sr);
            c.pos[i] = std::max (0.0f, std::min (1.0f, pos0 + x * wts * 0.5f));
            const float pan = std::max (-1.0f, std::min (1.0f, x * Q (uni_spread) + oPan));
            const float ang = (pan + 1.0f) * kPi * 0.25f;
            c.gl[i] = std::cos (ang) * 1.4142f * uw[i] * unorm * c.lvl;
            c.gr[i] = std::sin (ang) * 1.4142f * uw[i] * unorm * c.lvl;
        }
    };
    setupOsc (oc[0], 0, st1);
    setupOsc (oc[1], 1, st2);
    // an oscillator that feeds another one through FM / AM still has to run even when silent
    bool needO1 = oc[0].used, needO2 = oc[1].used;
    for (int k = 0; k < 2; ++k)
    {
        if ((oc[0].w[k] == 6 || oc[0].w[k] == 7) && oc[0].wa[k] > 0.001f && oc[0].used) needO2 = true;
        if ((oc[1].w[k] == 6 || oc[1].w[k] == 7) && oc[1].wa[k] > 0.001f && oc[1].used) needO1 = true;
    }
    const float incSub = std::min (0.45f, midiToHz (stSub) / sr);
    const float lvSub = std::max (0.0f, Q (sub_lvl) + md[D_SUBLVL]), lvNoise = std::max (0.0f, Q (noise_lvl) + md[D_NOISE]) * 0.5f;
    const bool subTri = ch (sub_wave) == 1, airNoise = ch (noise_col) == 1;

    // ---- filters
    const int route = ch (f_route);
    auto cutHz = [&] (float base, float keyT, float envAmt, float velAmt, float modOct)
    {
        return base * std::exp2 (keyT * (v.note - 60.0f) / 12.0f + envAmt * e2 * 5.0f + velAmt * (v.vel - 0.5f) * 2.0f + modOct * 4.0f);
    };
    FiltCoef c1, c2;
    filtSet (c1, ch (f1_type), cutHz (Q (f1_cut), Q (f1_key), Q (f1_env), Q (f1_vel), md[D_F1CUT]), std::max (0.0f, std::min (1.0f, Q (f1_res) + md[D_F1RES] * 0.5f)), Q (f1_drive), Q (f1_mix), sr);
    filtSet (c2, ch (f2_type), cutHz (Q (f2_cut), Q (f2_key), Q (f2_env), Q (f2_vel), md[D_F2CUT]), std::max (0.0f, std::min (1.0f, Q (f2_res) + md[D_F2RES] * 0.5f)), Q (f2_drive), Q (f2_mix), sr);
    const bool f2Off = ch (f2_type) == 0 && Q (f2_cut) > 19000.0f && Q (f2_mix) >= 0.999f && route == 0 && md[D_F2CUT] == 0.0f;   // transparent: skip it

    // ---- amp
    const float ampTarget = e1 * (0.25f + 0.75f * v.vel) * std::max (0.0f, std::min (2.0f, 1.0f + md[D_AMP])) * (v.stealing ? std::max (0.0f, v.stealGain) : 1.0f);
    float amp = v.lastAmp;
    const float ampStep = (ampTarget - amp) / fn;
    const float pan = std::max (-1.0f, std::min (1.0f, md[D_PAN]));
    const float panL = std::min (1.0f, 1.0f - pan), panR = std::min (1.0f, 1.0f + pan);

    for (int s = 0; s < n; ++s)
    {
        float aL = 0, aR = 0, bL = 0, bR = 0;   // a = osc1 bus, b = osc2 bus
        for (int u = 0; u < nU; ++u)
        {
            float y1 = 0, y2 = 0;
            if (needO1)
            {
                float ph = v.ph1[u] + oc[0].inc[u]; if (ph >= 1.0f) ph -= 1.0f; v.ph1[u] = ph;
                y1 = evalOsc (oc[0], u, ph, v.y2prev[u]); v.y1prev[u] = y1;
                if (oc[0].used) { aL += y1 * oc[0].gl[u]; aR += y1 * oc[0].gr[u]; }
            }
            if (needO2)
            {
                float ph = v.ph2[u] + oc[1].inc[u]; if (ph >= 1.0f) ph -= 1.0f; v.ph2[u] = ph;
                y2 = evalOsc (oc[1], u, ph, v.y1prev[u]); v.y2prev[u] = y2;
                if (oc[1].used) { bL += y2 * oc[1].gl[u]; bR += y2 * oc[1].gr[u]; }
            }
        }
        v.phSub += incSub; if (v.phSub >= 1.0f) v.phSub -= 1.0f;
        const float ys = subTri ? 4.0f * std::fabs (v.phSub - 0.5f) - 1.0f : std::sin (2.0f * kPi * v.phSub);
        float nz = v.rng.next();
        if (airNoise) { v.airLp += 0.12f * (nz - v.airLp); nz = (nz - v.airLp) * 0.7f; }
        const float extra = ys * lvSub + nz * lvNoise;

        float xl, xr;
        if (route == 2)
        {
            xl = filtRun (c1, v.f1[0], aL) + filtRun (c2, v.f2[0], bL + extra);
            xr = filtRun (c1, v.f1[1], aR) + filtRun (c2, v.f2[1], bR + extra);
        }
        else
        {
            const float inL = aL + bL + extra, inR = aR + bR + extra;
            if (route == 0)
            {
                xl = filtRun (c1, v.f1[0], inL); xr = filtRun (c1, v.f1[1], inR);
                if (! f2Off) { xl = filtRun (c2, v.f2[0], xl); xr = filtRun (c2, v.f2[1], xr); }
            }
            else
            {
                xl = (filtRun (c1, v.f1[0], inL) + filtRun (c2, v.f2[0], inL)) * 0.7071f;
                xr = (filtRun (c1, v.f1[1], inR) + filtRun (c2, v.f2[1], inR)) * 0.7071f;
            }
        }
        amp += ampStep;
        oL[s] += xl * amp * panL; oR[s] += xr * amp * panR;
    }
    v.lastAmp = ampTarget;
    #undef Q
}

// ------------------------------------------------------------------------------------ chunk + process
void Engine::chunk (float* L, float* R, int n)
{
    const float a = 1.0f - std::exp (-(float) n / (0.004f * sr));
    for (int i = 0; i < COUNT; ++i) sm[i] = isFloat[i] ? sm[i] + (pr[i] - sm[i]) * a : pr[i];

    // global LFOs
    float rateMul[4] = { 1, 1, 1, 1 };
    for (int s = 0; s < 10; ++s)
    {
        const int so = ch (m1_src + 3 * s), ds = ch (m1_dst + 3 * s);
        if (ds != D_L1RATE && ds != D_L2RATE) continue;
        float sv = 0.0f;
        if (so >= S_LFO3 && so <= S_LFO4) sv = lfoV[so - S_LFO1];
        else if (so >= S_MAC1 && so <= S_MAC8) sv = sm[mac1 + (so - S_MAC1)];
        else if (so == S_MOD) sv = modWheel; else if (so == S_AT) sv = at; else if (so == S_BEND) sv = bend;
        rateMul[ds == D_L1RATE ? 0 : 1] *= std::exp2 (sv * sm[m1_amt + 3 * s] * 3.0f);
    }
    for (int i = 0; i < 4; ++i)
    {
        const int sync = ch (l1_sync + 3 * i), shape = ch (l1_shape + 3 * i);
        float ph;
        if (sync == 0) { lfoAcc[i] += sm[l1_rate + 3 * i] * rateMul[i] * (float) n / sr; lfoAcc[i] -= std::floor (lfoAcc[i]); ph = lfoAcc[i]; }
        else ph = (float) (std::fmod (beatPos * (double) kLfoCyclesPerBeat[sync], 1.0));
        float cs[16];
        for (int p = 0; p < 16; ++p) cs[p] = custom[(size_t) (i * 16 + p)].load();
        lfoV[i] = lfos[i].eval (shape, ph, cs);
    }

    arpUpdate();
    seqUpdate();

    for (int i = 0; i < n; ++i) bufL[i] = bufR[i] = 0.0f;
    for (auto& v : voices) if (v.active) renderVoice (v, bufL, bufR, n);

    for (int r = 0; r < 8; ++r)
    {
        const int b = r1_type + 6 * r;
        rack[r].process (bufL, bufR, n, ch (b), ch (b + 1) != 0, sm[b + 2], sm[b + 3], sm[b + 4], sm[b + 5], (double) bpm);
    }

    const float outGain = sm[master] * 0.22f;
    for (int i = 0; i < n; ++i)
    {
        L[i] = bufL[i] * outGain; R[i] = bufR[i] * outGain;
        const int p = scopePos.load();
        scope[(size_t) (p % kScope)].store (0.5f * (L[i] + R[i]));
        scopePos.store (p + 1);
    }
    beatPos += (double) n * (double) bpm / 60.0 / (double) sr;
}

void Engine::process (float* left, float* right, int numSamples, const duo::TransportInfo& t)
{
    bpm = t.bpm > 20.0 ? (float) t.bpm : 138.0f;
    if (t.playing) beatPos = t.ppq;
    for (int done = 0; done < numSamples; done += kCR)
        chunk (left + done, right + done, std::min (kCR, numSamples - done));
}
} // namespace sb


