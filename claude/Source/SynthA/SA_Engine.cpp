#include "SA_Engine.h"

namespace sa
{
static float midiToHz (float n) { return 440.0f * std::exp2 ((n - 69.0f) / 12.0f); }
static const float kLfoCyclesPerBeat[8] = { 0, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 3.0f, 6.0f };
static const double kArpBeats[5] = { 1.0, 0.5, 0.25, 1.0 / 6.0, 0.125 };
static const double kGateBeats[3] = { 0.5, 0.25, 0.125 };

Engine::Engine()
{
    for (int i = 0; i < COUNT; ++i) { pr[i] = sm[i] = defs()[i].def; isFloat[i] = defs()[i].choices[0] == 0; }
    for (auto& s : scope) s.store (0.0f);
    for (auto& l : lfoShown) l.store (0.0f);
    for (int i = 0; i < kVoices; ++i) voices[i].rng.s = 0x9E3779B9u * (uint32_t) (i + 1);
    for (int i = 0; i < 3; ++i) lfos[i].rng.s = 0xA511E9B3u * (uint32_t) (i + 3);
}

void Engine::prepare (double sampleRate, int)
{
    sr = (float) sampleRate;
    fx.prepare (sampleRate);
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
    const bool mono = ch (play_mode) != 0;
    if (! mono || ! wasActive || sm[glide] <= 0.0001f) v.note = note;
    if (! wasActive)
    {
        for (int i = 0; i < kMaxUni; ++i) { v.ph1[i] = v.rng.uni(); v.ph2[i] = v.rng.uni(); v.tri1[i] = v.tri2[i] = v.y2prev[i] = 0.0f; }
        v.ph3 = v.rng.uni(); v.phSub = 0.0f; v.tri3 = 0.0f; v.lastAmp = 0.0f;
        for (int i = 0; i < 3; ++i) { v.tolP[i] = v.rng.next() * 3.0f; v.drift[i] = 0.0f; }
        v.tolCut = v.rng.next() * 0.04f;
    }
    if (retrig || ! wasActive) for (auto& e : v.env) e.gateOn();
}

void Engine::voiceOn (int key, float note, float vel)
{
    const int mode = ch (play_mode);
    if (mode != 0)
    {
        Voice& v = voices[0];
        const bool legatoJoin = mode == 2 && v.active && v.gate;
        startVoice (v, key, note, vel, ! legatoJoin);
        return;
    }
    Voice* pick = nullptr;
    for (auto& v : voices) if (! v.active) { pick = &v; break; }
    if (pick) { startVoice (*pick, key, note, vel, true); return; }
    // steal: prefer released voices with the lowest level, otherwise the oldest
    float best = 1e9f;
    for (auto& v : voices) if (! v.gate && ! v.stealing && v.env[0].v < best) { best = v.env[0].v; pick = &v; }
    if (! pick) { uint32_t oldest = 0xFFFFFFFFu; for (auto& v : voices) if (! v.stealing && v.order < oldest) { oldest = v.order; pick = &v; } }
    if (! pick) pick = &voices[0];
    pick->stealing = true; pick->pKey = key; pick->pNote = note; pick->pVel = vel;
}

void Engine::trigger (int key, float vel)
{
    if (ch (chord_on) && ! chord.empty() && ch (play_mode) == 0)
        for (int iv : chord) voiceOn (key, (float) (key + iv), vel);
    else
        voiceOn (key, (float) key, vel);
}

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
    if (ch (arp_on))
    {
        if (ch (arp_hold) && heldKeys.size() == 1) { arpHeld.clear(); }
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
    if (arpKeyOn >= 0) { arpKeyOn = -1; arpChordOn.clear(); }
    for (auto& v : voices)
        if (v.active) { v.gate = false; v.sustained = false; v.stealing = false; for (auto& e : v.env) e.gateOff(); }
}

void Engine::panic()
{
    allNotesOff();
    for (auto& v : voices) { v.active = false; v.stealing = false; for (auto& e : v.env) e.kill(); v.f1[0] = v.f1[1] = v.f2[0] = v.f2[1] = FiltState(); }
    fx.reset(); arpKeyOn = -1; arpChordOn.clear(); gateGain = 1.0f; bend = 0; modWheel = 0; at = 0;
}

void Engine::learnChord()
{
    chord.clear();
    if (heldKeys.size() < 2) return;
    const int root = *std::min_element (heldKeys.begin(), heldKeys.end());
    for (int k : heldKeys) chord.push_back (k - root);
    std::sort (chord.begin(), chord.end());
}

// ------------------------------------------------------------------------------------ arpeggiator
void Engine::rebuildArp()
{
    arpSeq.clear();
    std::vector<int> base (arpHeld);
    const int mode = ch (arp_mode), octs = std::max (1, std::min (4, (int) (pr[arp_oct] + 0.5f)));
    if (mode != 4) std::sort (base.begin(), base.end());
    std::vector<int> up;
    for (int o = 0; o < octs; ++o) for (int k : base) up.push_back (k + 12 * o);
    if (mode == 1) { arpSeq = up; std::reverse (arpSeq.begin(), arpSeq.end()); }
    else if (mode == 2)
    {
        arpSeq = up;
        for (int i = (int) up.size() - 2; i >= 1; --i) arpSeq.push_back (up[(size_t) i]);
    }
    else arpSeq = up;
    if (arpIdx >= (int) arpSeq.size()) arpIdx = 0;
    arpDirty = false;
}

void Engine::arpUpdate (int)
{
    auto stopArp = [this]
    {
        if (arpKeyOn >= 0) { releaseKey (arpKeyOn); arpKeyOn = -1; }
        for (int k : arpChordOn) releaseKey (k);
        arpChordOn.clear();
    };
    if (! ch (arp_on)) { if (arpKeyOn >= 0 || ! arpChordOn.empty()) stopArp(); arpLast = -1; return; }
    if (arpDirty) rebuildArp();

    const double stepBeats = kArpBeats[std::max (0, std::min (4, ch (arp_rate)))];
    const double pos = beatPos / stepBeats;
    const long stepIdx = (long) std::floor (pos);
    const float frac = (float) (pos - (double) stepIdx);
    const float swOff = (stepIdx & 1) ? pr[arp_swing] * 0.5f : 0.0f;

    if (stepIdx != arpLast && frac >= swOff)
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
    else if ((arpKeyOn >= 0 || ! arpChordOn.empty()) && frac >= swOff + pr[arp_gate] * (1.0f - swOff)) stopArp();
}

// ------------------------------------------------------------------------------------ voice rendering
void Engine::renderVoice (Voice& v, float* oL, float* oR, int n)
{
    #define Q(p) sm[p]
    const float fn = (float) n;

    if (v.stealing)
    {
        v.stealGain -= fn / 40.0f;
        if (v.stealGain <= 0.0f) { startVoice (v, v.pKey, v.pNote, v.pVel, true); }
    }
    const float e1 = v.env[0].step (n, Q (e1_a), Q (e1_d), Q (e1_s), Q (e1_r), sr);
    const float e2 = v.env[1].step (n, Q (e2_a), Q (e2_d), Q (e2_s), Q (e2_r), sr);
    const float e3 = v.env[2].step (n, Q (e3_a), Q (e3_d), Q (e3_s), Q (e3_r), sr);
    if (! v.env[0].active()) { v.active = false; return; }

    const int mode = ch (play_mode);
    if (mode != 0 && Q (glide) > 0.0001f) v.note += (v.target - v.note) * (1.0f - std::exp (-fn / (Q (glide) * sr * 0.35f)));
    else v.note = v.target;

    // ---- modulation sources and matrix
    float src[S_COUNT] = {};
    src[S_LFO1] = lfoV[0]; src[S_LFO2] = lfoV[1]; src[S_LFO3] = lfoV[2];
    src[S_ENV1] = e1; src[S_ENV2] = e2; src[S_ENV3] = e3; src[S_VEL] = v.vel; src[S_KEY] = (v.note - 60.0f) / 48.0f;
    src[S_MOD] = modWheel; src[S_AT] = at; src[S_RAND] = v.rnd;
    src[S_MAC1] = Q (mac1); src[S_MAC2] = Q (mac2); src[S_MAC3] = Q (mac3); src[S_MAC4] = Q (mac4); src[S_BEND] = bend;
    float md[D_COUNT] = {};
    for (int s = 0; s < 8; ++s)
    {
        const int sr_ = ch (m1_src + 3 * s), ds = ch (m1_dst + 3 * s);
        if (sr_ > 0 && ds > 0) md[ds] += src[sr_] * Q (m1_amt + 3 * s);
    }

    const float ageAmt = Q (age), warmAmt = std::max (0.0f, std::min (1.0f, Q (warmth) + md[D_WARMTH] * 0.5f));

    // ---- analog drift (slow random walk per oscillator, scaled by Age) and per-voice tolerance
    const float da = fn / (0.9f * sr), driftGain = 1.0f / (0.577f * std::sqrt (da * 0.5f));
    float driftSt[3];
    for (int i = 0; i < 3; ++i)
    {
        v.drift[i] += da * (v.rng.next() - v.drift[i]);
        driftSt[i] = (v.drift[i] * driftGain * 7.0f + v.tolP[i]) * ageAmt / 100.0f;
    }

    // ---- pitch
    const float baseSt = v.note + md[D_PITCH] * 24.0f + bend * Q (bend_range) + std::sin (vibPhase) * modWheel * 0.55f;
    const float st1 = baseSt + std::round (Q (o1_oct)) * 12.0f + std::round (Q (o1_semi)) + Q (o1_fine) / 100.0f + md[D_O1PITCH] * 24.0f + driftSt[0];
    const float st2 = baseSt + std::round (Q (o2_oct)) * 12.0f + std::round (Q (o2_semi)) + Q (o2_fine) / 100.0f + md[D_O2PITCH] * 24.0f + driftSt[1];
    const float st3 = baseSt + std::round (Q (o3_oct)) * 12.0f + std::round (Q (o3_semi)) + Q (o3_fine) / 100.0f + driftSt[2];
    const float stSub = baseSt - 12.0f * (1.0f + (float) ch (sub_oct)) + driftSt[0];
    const int wave1 = ch (o1_wave), wave2 = ch (o2_wave), wave3 = ch (o3_wave);

    // ---- unison tables (per chunk)
    float inc1[kMaxUni], gl1[kMaxUni], gr1[kMaxUni], inc2[kMaxUni], gl2[kMaxUni], gr2[kMaxUni];
    const float detune = std::max (0.0f, std::min (1.0f, Q (uni_detune) + md[D_DETUNE] * 0.5f));
    auto uniSetup = [&] (int wave, float st, float* inc, float* gl, float* gr, int& cnt)
    {
        cnt = wave == 4 ? 7 : std::max (1, std::min (kMaxUni, (int) std::round (Q (uni_voices))));
        const float maxC = (wave == 4 ? 95.0f : 60.0f) * std::pow (detune, 1.6f);
        const float hz = midiToHz (st);
        float w[kMaxUni], sumSq = 0.0f;
        for (int i = 0; i < cnt; ++i)
        {
            const float x = cnt > 1 ? ((float) i - 0.5f * (float) (cnt - 1)) / (0.5f * (float) (cnt - 1)) : 0.0f;
            const float tol = (float) (((i * 37 + (int) v.key) % 7) - 3) * 0.12f * ageAmt;
            inc[i] = std::min (0.45f, hz * std::exp2 ((x * maxC + tol) / 1200.0f) / sr);
            const float ang = (x * Q (uni_spread) + 1.0f) * kPi * 0.25f;
            gl[i] = std::cos (ang) * 1.4142f; gr[i] = std::sin (ang) * 1.4142f;
            w[i] = cnt == 1 ? 1.0f : (std::fabs (x) < 1e-4f ? 1.15f - 0.45f * Q (uni_blend) : 0.3f + 0.7f * Q (uni_blend));
            sumSq += w[i] * w[i];
        }
        const float norm = 1.0f / std::sqrt (sumSq);
        for (int i = 0; i < cnt; ++i) { gl[i] *= w[i] * norm; gr[i] *= w[i] * norm; }
    };
    int nU1, nU2;
    uniSetup (wave1, st1, inc1, gl1, gr1, nU1);
    uniSetup (wave2, st2, inc2, gl2, gr2, nU2);
    const float inc3 = std::min (0.45f, midiToHz (st3) / sr), incSub = std::min (0.45f, midiToHz (stSub) / sr);

    const float lv1 = std::max (0.0f, Q (o1_lvl) + md[D_O1LVL]), lv2 = std::max (0.0f, Q (o2_lvl) + md[D_O2LVL]);
    const float lv3 = std::max (0.0f, Q (o3_lvl) + md[D_O3LVL]), lvSub = std::max (0.0f, Q (sub_lvl) + md[D_SUBLVL]);
    const float lvNoise = std::max (0.0f, Q (noise_lvl) + md[D_NOISE]) * 0.5f;
    const float pw1 = std::max (0.05f, std::min (0.95f, Q (o1_pw) + md[D_O1PW] * 0.45f));
    const float pw2 = std::max (0.05f, std::min (0.95f, Q (o2_pw) + md[D_O2PW] * 0.45f));
    const float fmAmt = std::max (0.0f, std::min (1.0f, Q (o2_fm) + md[D_FM] * 0.5f)), ring = Q (o2_ring);
    const bool sync = ch (o2_sync) != 0, subSquare = ch (sub_wave) == 1, pinkNoise = ch (noise_col) == 1;

    // ---- filters
    const int route = ch (f_route);
    auto cutHz = [&] (float base, float keyT, float envAmt, float velAmt, float modOct)
    {
        const float oct = keyT * (v.note - 60.0f) / 12.0f + envAmt * e2 * 5.0f + velAmt * (v.vel - 0.5f) * 2.0f + modOct * 4.0f + v.tolCut * ageAmt * 4.0f;
        return base * std::exp2 (oct);
    };
    FiltCoef c1, c2;
    filtSet (c1, ch (f1_model), ch (f1_type), cutHz (Q (f1_cut), Q (f1_key), Q (f1_env), Q (f1_vel), md[D_F1CUT]),
             std::max (0.0f, std::min (1.0f, Q (f1_res) + md[D_F1RES] * 0.5f)), std::max (0.0f, std::min (1.0f, Q (f1_drive) + md[D_DRIVE] * 0.5f)), sr);
    filtSet (c2, ch (f2_model), ch (f2_type), cutHz (Q (f2_cut), Q (f2_key), Q (f2_env), Q (f2_vel), md[D_F2CUT]),
             std::max (0.0f, std::min (1.0f, Q (f2_res) + md[D_F2RES] * 0.5f)), std::max (0.0f, std::min (1.0f, Q (f2_drive) + md[D_DRIVE] * 0.5f)), sr);

    // ---- amplitude and pan
    const float ampTarget = e1 * (0.25f + 0.75f * v.vel) * std::max (0.0f, std::min (2.0f, 1.0f + md[D_AMP])) * (v.stealing ? std::max (0.0f, v.stealGain) : 1.0f);
    float amp = v.lastAmp;
    const float ampStep = (ampTarget - amp) / fn;
    const float pan = std::max (-1.0f, std::min (1.0f, md[D_PAN]));
    const float panL = std::min (1.0f, 1.0f - pan), panR = std::min (1.0f, 1.0f + pan);

    // flavor + warmth shaping
    static const float flGain[4] = { 1.0f, 1.9f, 0.8f, 2.8f }, flBias[4] = { 0.06f, 0.0f, 0.0f, 0.12f };
    const int fl = ch (flavor);
    const float fg = flGain[fl], fb = flBias[fl], fnorm = 1.0f / (0.55f + 0.45f * fg);
    auto shape = [&] (float x)
    {
        float y = (fastTanh (x * fg + fb) - fastTanh (fb)) * fnorm;
        const float wb = 0.3f * warmAmt;
        y = (fastTanh (y * (1.0f + 0.9f * warmAmt) + wb) - fastTanh (wb)) / (1.0f + 0.5f * warmAmt);
        return y;
    };

    for (int s = 0; s < n; ++s)
    {
        float aL = 0.0f, aR = 0.0f;
        const int maxU = std::max (nU1, nU2);
        for (int u = 0; u < maxU; ++u)
        {
            bool wrapped = false; float y1 = 0.0f;
            if (u < nU1)
            {
                float ph = v.ph1[u] + inc1[u];
                if (ph >= 1.0f) { ph -= 1.0f; wrapped = true; }
                v.ph1[u] = ph;
                float t = ph;
                if (fmAmt > 0.0f) { t += fmAmt * 0.35f * v.y2prev[u]; t -= std::floor (t); }
                switch (wave1)
                {
                    case 1: y1 = oscPulse (t, inc1[u], pw1); break;
                    case 2: y1 = oscTri (t, inc1[u], v.tri1[u]); break;
                    case 3: y1 = std::sin (2.0f * kPi * t); break;
                    default: y1 = oscSaw (t, inc1[u]); break;
                }
                aL += y1 * lv1 * gl1[u]; aR += y1 * lv1 * gr1[u];
            }
            if (u < nU2)
            {
                float ph = v.ph2[u] + inc2[u];
                if (ph >= 1.0f) ph -= 1.0f;
                if (sync && wrapped) ph = std::fmod (v.ph1[u] * inc2[u] / std::max (inc1[u], 1e-6f), 1.0f);
                v.ph2[u] = ph;
                float y2;
                switch (wave2)
                {
                    case 1: y2 = oscPulse (ph, inc2[u], pw2); break;
                    case 2: y2 = oscTri (ph, inc2[u], v.tri2[u]); break;
                    case 3: y2 = std::sin (2.0f * kPi * ph); break;
                    default: y2 = oscSaw (ph, inc2[u]); break;
                }
                v.y2prev[u] = y2;
                if (ring > 0.0f && u < nU1) y2 = y2 * (1.0f - ring) + ring * 1.6f * y1 * y2;
                aL += y2 * lv2 * gl2[u]; aR += y2 * lv2 * gr2[u];
            }
        }
        // osc 3, sub, noise -> bus B (centre)
        v.ph3 += inc3; if (v.ph3 >= 1.0f) v.ph3 -= 1.0f;
        float y3;
        switch (wave3)
        {
            case 1: y3 = oscPulse (v.ph3, inc3, 0.5f); break;
            case 2: y3 = oscTri (v.ph3, inc3, v.tri3); break;
            case 3: y3 = std::sin (2.0f * kPi * v.ph3); break;
            default: y3 = oscSaw (v.ph3, inc3); break;
        }
        v.phSub += incSub; if (v.phSub >= 1.0f) v.phSub -= 1.0f;
        const float ys = subSquare ? (v.phSub < 0.5f ? 0.8f : -0.8f) : std::sin (2.0f * kPi * v.phSub);
        float nz = v.rng.next();
        if (pinkNoise)
        {
            v.pink[0] = 0.99765f * v.pink[0] + nz * 0.0990460f; v.pink[1] = 0.96300f * v.pink[1] + nz * 0.2965164f; v.pink[2] = 0.57000f * v.pink[2] + nz * 1.0526913f;
            nz = (v.pink[0] + v.pink[1] + v.pink[2] + nz * 0.1848f) * 0.35f;
        }
        const float bus = y3 * lv3 + ys * lvSub + nz * lvNoise;

        float xl, xr;
        if (route == 2)
        {
            const float fa = filtRun (c1, v.f1[0], shape (aL)), fb2 = filtRun (c1, v.f1[1], shape (aR));
            const float gb = filtRun (c2, v.f2[0], shape (bus)), gb2 = filtRun (c2, v.f2[1], shape (bus));
            xl = fa + gb; xr = fb2 + gb2;
        }
        else
        {
            const float inL = shape (aL + bus), inR = shape (aR + bus);
            if (route == 0) { xl = filtRun (c2, v.f2[0], filtRun (c1, v.f1[0], inL)); xr = filtRun (c2, v.f2[1], filtRun (c1, v.f1[1], inR)); }
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

    // global LFOs (free-running or locked to the beat)
    float rateMul[3] = { 1, 1, 1 };
    for (int s = 0; s < 8; ++s)
    {
        const int sr_ = ch (m1_src + 3 * s), ds = ch (m1_dst + 3 * s);
        if (ds != D_L1RATE && ds != D_L2RATE) continue;
        float sv = 0.0f;
        switch (sr_) { case S_LFO3: sv = lfoV[2]; break; case S_MOD: sv = modWheel; break; case S_AT: sv = at; break; case S_BEND: sv = bend; break;
                       case S_MAC1: sv = sm[mac1]; break; case S_MAC2: sv = sm[mac2]; break; case S_MAC3: sv = sm[mac3]; break; case S_MAC4: sv = sm[mac4]; break; default: break; }
        rateMul[ds == D_L1RATE ? 0 : 1] *= std::exp2 (sv * sm[m1_amt + 3 * s] * 3.0f);
    }
    for (int i = 0; i < 3; ++i)
    {
        const int sync = ch (l1_sync + 3 * i), shape = ch (l1_shape + 3 * i);
        float ph;
        if (sync == 0) { lfoAcc[i] += sm[l1_rate + 3 * i] * rateMul[i] * (float) n / sr; lfoAcc[i] -= std::floor (lfoAcc[i]); ph = lfoAcc[i]; }
        else ph = (float) (std::fmod (beatPos * (double) kLfoCyclesPerBeat[sync], 1.0));
        lfoV[i] = lfos[i].eval (shape, ph);
        lfoShown[i].store (lfoV[i]);
    }
    vibPhase += 2.0f * kPi * 5.5f * (float) n / sr; if (vibPhase > 2.0f * kPi) vibPhase -= 2.0f * kPi;

    arpUpdate (n);

    for (int i = 0; i < n; ++i) bufL[i] = bufR[i] = 0.0f;
    for (auto& v : voices) if (v.active) renderVoice (v, bufL, bufR, n);

    // trance gate (post voices, pre effects)
    {
        float target = 1.0f;
        if (ch (gate_on))
        {
            const double sb = kGateBeats[std::max (0, std::min (2, ch (gate_rate)))];
            const int idx = (int) (((long) std::floor (beatPos / sb)) & 15);
            gateStepIdx.store (idx);
            target = 1.0f - sm[gate_depth] * (1.0f - (sm[gs1 + idx] > 0.5f ? 1.0f : sm[gs1 + idx] * 2.0f));
        }
        const float tc = 0.0008f + 0.04f * sm[gate_smooth];
        const float g1 = gateGain + (target - gateGain) * (1.0f - std::exp (-(float) n / (tc * sr)));
        for (int i = 0; i < n; ++i) { const float g = gateGain + (g1 - gateGain) * (float) (i + 1) / (float) n; bufL[i] *= g; bufR[i] *= g; }
        gateGain = g1;
    }

    FxChain::Settings fs;
    fs.drvOn = ch (fx_drv_on); fs.chOn = ch (fx_ch_on); fs.phOn = ch (fx_ph_on); fs.dlOn = ch (fx_dl_on); fs.rvOn = ch (fx_rv_on); fs.eqOn = ch (fx_eq_on); fs.pumpOn = ch (fx_pump_on);
    fs.drvType = ch (fx_drv_type); fs.chMode = ch (fx_ch_mode); fs.dlTime = ch (fx_dl_time); fs.dlPP = ch (fx_dl_pp); fs.pumpRate = ch (fx_pump_rate);
    fs.drvAmt = sm[fx_drv_amt]; fs.chRate = sm[fx_ch_rate]; fs.chDepth = sm[fx_ch_depth]; fs.chMix = sm[fx_ch_mix];
    fs.phRate = sm[fx_ph_rate]; fs.phDepth = sm[fx_ph_depth]; fs.phFb = sm[fx_ph_fb]; fs.phMix = sm[fx_ph_mix];
    fs.dlFb = sm[fx_dl_fb]; fs.dlTone = sm[fx_dl_tone]; fs.dlMix = sm[fx_dl_mix];
    fs.rvSize = sm[fx_rv_size]; fs.rvDamp = sm[fx_rv_damp]; fs.rvMix = sm[fx_rv_mix];
    fs.eqLo = sm[fx_eq_lo]; fs.eqHi = sm[fx_eq_hi]; fs.pumpDepth = sm[fx_pump_depth]; fs.pumpRel = sm[fx_pump_rel];
    fx.process (bufL, bufR, n, fs, (double) bpm, beatPos);

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
} // namespace sa
