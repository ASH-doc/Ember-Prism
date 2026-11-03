// EMBER (Synth A) - parameter table. One X-macro line per parameter: name, label, min, max, default, skew, choices ("" = float).
#pragma once
#include <cstddef>

namespace sa
{
#define SA_STEP_DEFAULTS_ON 1.0f
#define SA_PARAMS(X) \
    /* oscillators */ \
    X(o1_wave, "Osc1 Wave", 0, 4, 0, 1, "Saw|Pulse|Triangle|Sine|Hypersaw") \
    X(o1_oct, "Osc1 Oct", -2, 2, 0, 1, "") \
    X(o1_semi, "Osc1 Semi", -12, 12, 0, 1, "") \
    X(o1_fine, "Osc1 Fine", -50, 50, 0, 1, "") \
    X(o1_pw, "Osc1 PW", 0.05f, 0.95f, 0.5f, 1, "") \
    X(o1_lvl, "Osc1 Level", 0, 1, 0.8f, 1, "") \
    X(o2_wave, "Osc2 Wave", 0, 4, 0, 1, "Saw|Pulse|Triangle|Sine|Hypersaw") \
    X(o2_oct, "Osc2 Oct", -2, 2, 0, 1, "") \
    X(o2_semi, "Osc2 Semi", -12, 12, 0, 1, "") \
    X(o2_fine, "Osc2 Fine", -50, 50, 7, 1, "") \
    X(o2_pw, "Osc2 PW", 0.05f, 0.95f, 0.5f, 1, "") \
    X(o2_lvl, "Osc2 Level", 0, 1, 0.6f, 1, "") \
    X(o2_sync, "Osc2 Sync", 0, 1, 0, 1, "Off|On") \
    X(o2_ring, "Ring Mod", 0, 1, 0, 1, "") \
    X(o2_fm, "FM Amount", 0, 1, 0, 1, "") \
    X(o3_wave, "Osc3 Wave", 0, 3, 0, 1, "Saw|Pulse|Triangle|Sine") \
    X(o3_oct, "Osc3 Oct", -2, 2, -1, 1, "") \
    X(o3_semi, "Osc3 Semi", -12, 12, 0, 1, "") \
    X(o3_fine, "Osc3 Fine", -50, 50, -5, 1, "") \
    X(o3_lvl, "Osc3 Level", 0, 1, 0, 1, "") \
    X(sub_lvl, "Sub Level", 0, 1, 0.3f, 1, "") \
    X(sub_wave, "Sub Wave", 0, 1, 0, 1, "Sine|Square") \
    X(sub_oct, "Sub Oct", 0, 1, 0, 1, "-1 Oct|-2 Oct") \
    X(noise_lvl, "Noise", 0, 1, 0, 1, "") \
    X(noise_col, "Noise Color", 0, 1, 1, 1, "White|Pink") \
    /* unison + analog character */ \
    X(uni_voices, "Unison Voices", 1, 8, 1, 1, "") \
    X(uni_detune, "Unison Detune", 0, 1, 0.3f, 1, "") \
    X(uni_spread, "Unison Spread", 0, 1, 0.7f, 1, "") \
    X(uni_blend, "Unison Blend", 0, 1, 0.6f, 1, "") \
    X(warmth, "Warmth", 0, 1, 0.4f, 1, "") \
    X(age, "Age", 0, 1, 0.25f, 1, "") \
    X(flavor, "Osc Flavor", 0, 3, 0, 1, "Warm|Brass|Glass|Gritty") \
    /* filters */ \
    X(f1_model, "F1 Model", 0, 3, 0, 1, "Moog Ladder 24|Multimode 12|Screamer 12|State Variable") \
    X(f1_type, "F1 Type", 0, 3, 0, 1, "LP|HP|BP|Notch") \
    X(f1_cut, "F1 Cutoff", 20, 20000, 1800, 0.28f, "") \
    X(f1_res, "F1 Resonance", 0, 1, 0.2f, 1, "") \
    X(f1_drive, "F1 Drive", 0, 1, 0.2f, 1, "") \
    X(f1_key, "F1 Key Track", 0, 1, 0.5f, 1, "") \
    X(f1_env, "F1 Env Amount", -1, 1, 0.45f, 1, "") \
    X(f1_vel, "F1 Velocity", 0, 1, 0.3f, 1, "") \
    X(f2_model, "F2 Model", 0, 3, 1, 1, "Moog Ladder 24|Multimode 12|Screamer 12|State Variable") \
    X(f2_type, "F2 Type", 0, 3, 0, 1, "LP|HP|BP|Notch") \
    X(f2_cut, "F2 Cutoff", 20, 20000, 20000, 0.28f, "") \
    X(f2_res, "F2 Resonance", 0, 1, 0.1f, 1, "") \
    X(f2_drive, "F2 Drive", 0, 1, 0, 1, "") \
    X(f2_key, "F2 Key Track", 0, 1, 0.3f, 1, "") \
    X(f2_env, "F2 Env Amount", -1, 1, 0, 1, "") \
    X(f2_vel, "F2 Velocity", 0, 1, 0, 1, "") \
    X(f_route, "Routing", 0, 2, 0, 1, "Serial|Parallel|Split") \
    /* envelopes */ \
    X(e1_a, "Amp Attack", 0.001f, 8, 0.004f, 0.3f, "") \
    X(e1_d, "Amp Decay", 0.005f, 8, 0.4f, 0.3f, "") \
    X(e1_s, "Amp Sustain", 0, 1, 0.8f, 1, "") \
    X(e1_r, "Amp Release", 0.005f, 10, 0.25f, 0.3f, "") \
    X(e2_a, "Flt Attack", 0.001f, 8, 0.003f, 0.3f, "") \
    X(e2_d, "Flt Decay", 0.005f, 8, 0.35f, 0.3f, "") \
    X(e2_s, "Flt Sustain", 0, 1, 0.25f, 1, "") \
    X(e2_r, "Flt Release", 0.005f, 10, 0.3f, 0.3f, "") \
    X(e3_a, "Env3 Attack", 0.001f, 8, 0.01f, 0.3f, "") \
    X(e3_d, "Env3 Decay", 0.005f, 8, 0.5f, 0.3f, "") \
    X(e3_s, "Env3 Sustain", 0, 1, 0, 1, "") \
    X(e3_r, "Env3 Release", 0.005f, 10, 0.3f, 0.3f, "") \
    /* LFOs */ \
    X(l1_shape, "LFO1 Shape", 0, 5, 0, 1, "Sine|Triangle|Saw|Square|S&H|Smooth Rnd") \
    X(l1_rate, "LFO1 Rate", 0.02f, 30, 4, 0.35f, "") \
    X(l1_sync, "LFO1 Sync", 0, 7, 0, 1, "Free|1/1|1/2|1/4|1/8|1/16|1/8T|1/16T") \
    X(l2_shape, "LFO2 Shape", 0, 5, 1, 1, "Sine|Triangle|Saw|Square|S&H|Smooth Rnd") \
    X(l2_rate, "LFO2 Rate", 0.02f, 30, 0.5f, 0.35f, "") \
    X(l2_sync, "LFO2 Sync", 0, 7, 0, 1, "Free|1/1|1/2|1/4|1/8|1/16|1/8T|1/16T") \
    X(l3_shape, "LFO3 Shape", 0, 5, 4, 1, "Sine|Triangle|Saw|Square|S&H|Smooth Rnd") \
    X(l3_rate, "LFO3 Rate", 0.02f, 30, 6, 0.35f, "") \
    X(l3_sync, "LFO3 Sync", 0, 7, 0, 1, "Free|1/1|1/2|1/4|1/8|1/16|1/8T|1/16T") \
    /* mod matrix: 8 slots */ \
    X(m1_src, "Mod1 Source", 0, 16, 0, 1, "None|LFO 1|LFO 2|LFO 3|Env 1|Env 2|Env 3|Velocity|Key|Mod Wheel|Aftertouch|Random|Macro 1|Macro 2|Macro 3|Macro 4|Pitch Bend") \
    X(m1_dst, "Mod1 Dest", 0, 22, 0, 1, "None|Pitch|Osc1 Pitch|Osc2 Pitch|Osc1 PW|Osc2 PW|F1 Cutoff|F1 Res|F2 Cutoff|F2 Res|Drive|Amp|Pan|Detune|Osc1 Lvl|Osc2 Lvl|Osc3 Lvl|Sub Lvl|Noise Lvl|FM Amt|Warmth|LFO1 Rate|LFO2 Rate") \
    X(m1_amt, "Mod1 Amount", -1, 1, 0, 1, "") \
    X(m2_src, "Mod2 Source", 0, 16, 0, 1, "None|LFO 1|LFO 2|LFO 3|Env 1|Env 2|Env 3|Velocity|Key|Mod Wheel|Aftertouch|Random|Macro 1|Macro 2|Macro 3|Macro 4|Pitch Bend") \
    X(m2_dst, "Mod2 Dest", 0, 22, 0, 1, "None|Pitch|Osc1 Pitch|Osc2 Pitch|Osc1 PW|Osc2 PW|F1 Cutoff|F1 Res|F2 Cutoff|F2 Res|Drive|Amp|Pan|Detune|Osc1 Lvl|Osc2 Lvl|Osc3 Lvl|Sub Lvl|Noise Lvl|FM Amt|Warmth|LFO1 Rate|LFO2 Rate") \
    X(m2_amt, "Mod2 Amount", -1, 1, 0, 1, "") \
    X(m3_src, "Mod3 Source", 0, 16, 0, 1, "None|LFO 1|LFO 2|LFO 3|Env 1|Env 2|Env 3|Velocity|Key|Mod Wheel|Aftertouch|Random|Macro 1|Macro 2|Macro 3|Macro 4|Pitch Bend") \
    X(m3_dst, "Mod3 Dest", 0, 22, 0, 1, "None|Pitch|Osc1 Pitch|Osc2 Pitch|Osc1 PW|Osc2 PW|F1 Cutoff|F1 Res|F2 Cutoff|F2 Res|Drive|Amp|Pan|Detune|Osc1 Lvl|Osc2 Lvl|Osc3 Lvl|Sub Lvl|Noise Lvl|FM Amt|Warmth|LFO1 Rate|LFO2 Rate") \
    X(m3_amt, "Mod3 Amount", -1, 1, 0, 1, "") \
    X(m4_src, "Mod4 Source", 0, 16, 0, 1, "None|LFO 1|LFO 2|LFO 3|Env 1|Env 2|Env 3|Velocity|Key|Mod Wheel|Aftertouch|Random|Macro 1|Macro 2|Macro 3|Macro 4|Pitch Bend") \
    X(m4_dst, "Mod4 Dest", 0, 22, 0, 1, "None|Pitch|Osc1 Pitch|Osc2 Pitch|Osc1 PW|Osc2 PW|F1 Cutoff|F1 Res|F2 Cutoff|F2 Res|Drive|Amp|Pan|Detune|Osc1 Lvl|Osc2 Lvl|Osc3 Lvl|Sub Lvl|Noise Lvl|FM Amt|Warmth|LFO1 Rate|LFO2 Rate") \
    X(m4_amt, "Mod4 Amount", -1, 1, 0, 1, "") \
    X(m5_src, "Mod5 Source", 0, 16, 0, 1, "None|LFO 1|LFO 2|LFO 3|Env 1|Env 2|Env 3|Velocity|Key|Mod Wheel|Aftertouch|Random|Macro 1|Macro 2|Macro 3|Macro 4|Pitch Bend") \
    X(m5_dst, "Mod5 Dest", 0, 22, 0, 1, "None|Pitch|Osc1 Pitch|Osc2 Pitch|Osc1 PW|Osc2 PW|F1 Cutoff|F1 Res|F2 Cutoff|F2 Res|Drive|Amp|Pan|Detune|Osc1 Lvl|Osc2 Lvl|Osc3 Lvl|Sub Lvl|Noise Lvl|FM Amt|Warmth|LFO1 Rate|LFO2 Rate") \
    X(m5_amt, "Mod5 Amount", -1, 1, 0, 1, "") \
    X(m6_src, "Mod6 Source", 0, 16, 0, 1, "None|LFO 1|LFO 2|LFO 3|Env 1|Env 2|Env 3|Velocity|Key|Mod Wheel|Aftertouch|Random|Macro 1|Macro 2|Macro 3|Macro 4|Pitch Bend") \
    X(m6_dst, "Mod6 Dest", 0, 22, 0, 1, "None|Pitch|Osc1 Pitch|Osc2 Pitch|Osc1 PW|Osc2 PW|F1 Cutoff|F1 Res|F2 Cutoff|F2 Res|Drive|Amp|Pan|Detune|Osc1 Lvl|Osc2 Lvl|Osc3 Lvl|Sub Lvl|Noise Lvl|FM Amt|Warmth|LFO1 Rate|LFO2 Rate") \
    X(m6_amt, "Mod6 Amount", -1, 1, 0, 1, "") \
    X(m7_src, "Mod7 Source", 0, 16, 0, 1, "None|LFO 1|LFO 2|LFO 3|Env 1|Env 2|Env 3|Velocity|Key|Mod Wheel|Aftertouch|Random|Macro 1|Macro 2|Macro 3|Macro 4|Pitch Bend") \
    X(m7_dst, "Mod7 Dest", 0, 22, 0, 1, "None|Pitch|Osc1 Pitch|Osc2 Pitch|Osc1 PW|Osc2 PW|F1 Cutoff|F1 Res|F2 Cutoff|F2 Res|Drive|Amp|Pan|Detune|Osc1 Lvl|Osc2 Lvl|Osc3 Lvl|Sub Lvl|Noise Lvl|FM Amt|Warmth|LFO1 Rate|LFO2 Rate") \
    X(m7_amt, "Mod7 Amount", -1, 1, 0, 1, "") \
    X(m8_src, "Mod8 Source", 0, 16, 0, 1, "None|LFO 1|LFO 2|LFO 3|Env 1|Env 2|Env 3|Velocity|Key|Mod Wheel|Aftertouch|Random|Macro 1|Macro 2|Macro 3|Macro 4|Pitch Bend") \
    X(m8_dst, "Mod8 Dest", 0, 22, 0, 1, "None|Pitch|Osc1 Pitch|Osc2 Pitch|Osc1 PW|Osc2 PW|F1 Cutoff|F1 Res|F2 Cutoff|F2 Res|Drive|Amp|Pan|Detune|Osc1 Lvl|Osc2 Lvl|Osc3 Lvl|Sub Lvl|Noise Lvl|FM Amt|Warmth|LFO1 Rate|LFO2 Rate") \
    X(m8_amt, "Mod8 Amount", -1, 1, 0, 1, "") \
    X(mac1, "Macro 1", 0, 1, 0, 1, "") \
    X(mac2, "Macro 2", 0, 1, 0, 1, "") \
    X(mac3, "Macro 3", 0, 1, 0, 1, "") \
    X(mac4, "Macro 4", 0, 1, 0, 1, "") \
    /* performance */ \
    X(play_mode, "Voice Mode", 0, 2, 0, 1, "Poly|Mono|Legato") \
    X(glide, "Glide", 0, 2, 0, 0.4f, "") \
    X(bend_range, "Bend Range", 1, 24, 2, 1, "") \
    X(chord_on, "Chord Stab", 0, 1, 0, 1, "Off|On") \
    X(arp_on, "Arp", 0, 1, 0, 1, "Off|On") \
    X(arp_mode, "Arp Mode", 0, 5, 0, 1, "Up|Down|Up-Down|Random|As Played|Chord") \
    X(arp_rate, "Arp Rate", 0, 4, 2, 1, "1/4|1/8|1/16|1/16T|1/32") \
    X(arp_oct, "Arp Octaves", 1, 4, 2, 1, "") \
    X(arp_gate, "Arp Gate", 0.05f, 1, 0.6f, 1, "") \
    X(arp_swing, "Arp Swing", 0, 0.6f, 0, 1, "") \
    X(arp_hold, "Arp Hold", 0, 1, 0, 1, "Off|On") \
    X(gate_on, "Trance Gate", 0, 1, 0, 1, "Off|On") \
    X(gate_rate, "Gate Rate", 0, 2, 1, 1, "1/8|1/16|1/32") \
    X(gate_depth, "Gate Depth", 0, 1, 0.9f, 1, "") \
    X(gate_smooth, "Gate Smooth", 0, 1, 0.25f, 1, "") \
    X(gs1, "Step 1", 0, 1, 1, 1, "") X(gs2, "Step 2", 0, 1, 0, 1, "") X(gs3, "Step 3", 0, 1, 1, 1, "") X(gs4, "Step 4", 0, 1, 1, 1, "") \
    X(gs5, "Step 5", 0, 1, 1, 1, "") X(gs6, "Step 6", 0, 1, 0, 1, "") X(gs7, "Step 7", 0, 1, 1, 1, "") X(gs8, "Step 8", 0, 1, 0, 1, "") \
    X(gs9, "Step 9", 0, 1, 1, 1, "") X(gs10, "Step 10", 0, 1, 0, 1, "") X(gs11, "Step 11", 0, 1, 1, 1, "") X(gs12, "Step 12", 0, 1, 1, 1, "") \
    X(gs13, "Step 13", 0, 1, 1, 1, "") X(gs14, "Step 14", 0, 1, 0, 1, "") X(gs15, "Step 15", 0, 1, 1, 1, "") X(gs16, "Step 16", 0, 1, 0, 1, "") \
    /* Ember's own effects */ \
    X(fx_drv_on, "Drive On", 0, 1, 0, 1, "Off|On") \
    X(fx_drv_type, "Drive Type", 0, 2, 1, 1, "Tube|Tape|Hard") \
    X(fx_drv_amt, "Drive Amount", 0, 1, 0.3f, 1, "") \
    X(fx_ch_on, "Chorus On", 0, 1, 0, 1, "Off|On") \
    X(fx_ch_mode, "Chorus Mode", 0, 1, 0, 1, "Chorus|Flanger") \
    X(fx_ch_rate, "Chorus Rate", 0.05f, 6, 0.6f, 0.4f, "") \
    X(fx_ch_depth, "Chorus Depth", 0, 1, 0.45f, 1, "") \
    X(fx_ch_mix, "Chorus Mix", 0, 1, 0.35f, 1, "") \
    X(fx_ph_on, "Phaser On", 0, 1, 0, 1, "Off|On") \
    X(fx_ph_rate, "Phaser Rate", 0.05f, 6, 0.3f, 0.4f, "") \
    X(fx_ph_depth, "Phaser Depth", 0, 1, 0.6f, 1, "") \
    X(fx_ph_fb, "Phaser Feedback", 0, 0.9f, 0.4f, 1, "") \
    X(fx_ph_mix, "Phaser Mix", 0, 1, 0.4f, 1, "") \
    X(fx_dl_on, "Delay On", 0, 1, 0, 1, "Off|On") \
    X(fx_dl_time, "Delay Time", 0, 5, 2, 1, "1/16|1/8|1/8.|1/4|1/4.|1/2") \
    X(fx_dl_fb, "Delay Feedback", 0, 0.92f, 0.38f, 1, "") \
    X(fx_dl_tone, "Delay Tone", 0, 1, 0.45f, 1, "") \
    X(fx_dl_pp, "Delay Mode", 0, 1, 1, 1, "Stereo|Ping-Pong") \
    X(fx_dl_mix, "Delay Mix", 0, 1, 0.22f, 1, "") \
    X(fx_rv_on, "Reverb On", 0, 1, 0, 1, "Off|On") \
    X(fx_rv_size, "Reverb Size", 0, 1, 0.55f, 1, "") \
    X(fx_rv_damp, "Reverb Damp", 0, 1, 0.65f, 1, "") \
    X(fx_rv_mix, "Reverb Mix", 0, 1, 0.2f, 1, "") \
    X(fx_eq_on, "Tone EQ On", 0, 1, 0, 1, "Off|On") \
    X(fx_eq_lo, "EQ Low", -12, 12, 0, 1, "") \
    X(fx_eq_hi, "EQ High", -12, 12, 0, 1, "") \
    X(fx_pump_on, "Pump On", 0, 1, 0, 1, "Off|On") \
    X(fx_pump_depth, "Pump Depth", 0, 1, 0.6f, 1, "") \
    X(fx_pump_rel, "Pump Release", 0.1f, 1, 0.55f, 1, "") \
    X(fx_pump_rate, "Pump Rate", 0, 1, 0, 1, "1/4|1/8") \
    X(master, "Level", 0, 1, 0.75f, 1, "")

enum P : int
{
#define X(n, l, mn, mx, d, sk, ch) n,
    SA_PARAMS(X)
#undef X
    COUNT
};

struct Def { const char* id; const char* name; float mn, mx, def, skew; const char* choices; };

inline const Def* defs()
{
    static const Def d[COUNT] = {
#define X(n, l, mn, mx, dv, sk, ch) { "sa_" #n, l, (float) (mn), (float) (mx), (float) (dv), (float) (sk), ch },
        SA_PARAMS(X)
#undef X
    };
    return d;
}

// modulation destinations (same order as the "Dest" choice list above)
enum Dest { D_NONE, D_PITCH, D_O1PITCH, D_O2PITCH, D_O1PW, D_O2PW, D_F1CUT, D_F1RES, D_F2CUT, D_F2RES, D_DRIVE, D_AMP, D_PAN,
            D_DETUNE, D_O1LVL, D_O2LVL, D_O3LVL, D_SUBLVL, D_NOISE, D_FM, D_WARMTH, D_L1RATE, D_L2RATE, D_COUNT };
enum Src { S_NONE, S_LFO1, S_LFO2, S_LFO3, S_ENV1, S_ENV2, S_ENV3, S_VEL, S_KEY, S_MOD, S_AT, S_RAND, S_MAC1, S_MAC2, S_MAC3, S_MAC4, S_BEND, S_COUNT };
} // namespace sa
