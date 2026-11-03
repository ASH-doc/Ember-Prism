// PRISM (Synth B) - parameter table. X(name, label, min, max, default, skew, choices)  ("" = float)
#pragma once
#include <cstddef>

namespace sb
{
#define SB_TABLE_NAMES "Saw-Square|Saw-Triangle|Pulse Sweep|Sine-Saw|Hollow Saw|Bright-Dull|Resonant Sweep|Glass Peaks|Digital Steps|Harmonic Ladder|Comb Metal|Crystal|Vowel AEIOU|Choir|Talk|Nasal Sweep|Reed|Organ|Pipe|Bowed String|Noise Texture|Marimba|FM 1:1|FM 1:2|FM 1:3|FM Bell|Mallet Stack|Air Shimmer|Whisper|Breath"
#define SB_WARPS "None|Sync|Bend +|Bend -|Mirror|Asym|FM|AM / Ring|PWM|Quantize|Flip"
#define SB_FILTERS "LP 12|LP 24|HP 12|Band Pass|Notch|Comb|Formant|Air Shelf"
#define SB_LFOSHAPES "Sine|Triangle|Saw|Square|S&H|Drawn"
#define SB_SYNC "Free|1/1|1/2|1/4|1/8|1/16|1/8T|1/16T"
#define SB_SRC "None|LFO 1|LFO 2|LFO 3|LFO 4|Env 1|Env 2|Env 3|Macro 1|Macro 2|Macro 3|Macro 4|Macro 5|Macro 6|Macro 7|Macro 8|Velocity|Key|Mod Wheel|Aftertouch|Random|Pitch Bend"
#define SB_DST "None|Pitch|Osc1 Pitch|Osc2 Pitch|Osc1 Pos|Osc2 Pos|Osc1 Warp1|Osc1 Warp2|Osc2 Warp1|Osc2 Warp2|Osc1 Lvl|Osc2 Lvl|Sub Lvl|Noise Lvl|F1 Cutoff|F1 Res|F2 Cutoff|F2 Res|Detune|WT Spread|Amp|Pan|LFO1 Rate|LFO2 Rate"
#define SB_FXTYPES "None|Chorus|Phaser|Delay|Reverb|Distortion|EQ|Compressor|Filter|Widener|Flanger"

#define SB_OSC(X, n, tbl, lvl) \
    X(o##n##_table, "Osc" #n " Table", 0, 29, tbl, 1, SB_TABLE_NAMES) \
    X(o##n##_pos, "Osc" #n " Position", 0, 1, 0, 1, "") \
    X(o##n##_oct, "Osc" #n " Oct", -3, 3, 0, 1, "") \
    X(o##n##_semi, "Osc" #n " Semi", -12, 12, 0, 1, "") \
    X(o##n##_fine, "Osc" #n " Fine", -50, 50, 0, 1, "") \
    X(o##n##_lvl, "Osc" #n " Level", 0, 1, lvl, 1, "") \
    X(o##n##_pan, "Osc" #n " Pan", -1, 1, 0, 1, "") \
    X(o##n##_phase, "Osc" #n " Phase", 0, 1, 0, 1, "") \
    X(o##n##_rand, "Osc" #n " Start", 0, 1, 1, 1, "Fixed|Random") \
    X(o##n##_w1, "Osc" #n " Warp 1", 0, 10, 0, 1, SB_WARPS) \
    X(o##n##_w1a, "Osc" #n " Warp 1 Amt", 0, 1, 0, 1, "") \
    X(o##n##_w2, "Osc" #n " Warp 2", 0, 10, 0, 1, SB_WARPS) \
    X(o##n##_w2a, "Osc" #n " Warp 2 Amt", 0, 1, 0, 1, "")

#define SB_FILT(X, n, type, cut) \
    X(f##n##_type, "F" #n " Type", 0, 7, type, 1, SB_FILTERS) \
    X(f##n##_cut, "F" #n " Cutoff", 20, 20000, cut, 0.28f, "") \
    X(f##n##_res, "F" #n " Resonance", 0, 1, 0.15f, 1, "") \
    X(f##n##_drive, "F" #n " Drive", 0, 1, 0, 1, "") \
    X(f##n##_mix, "F" #n " Mix", 0, 1, 1, 1, "") \
    X(f##n##_key, "F" #n " Key Track", 0, 1, 0.3f, 1, "") \
    X(f##n##_env, "F" #n " Env Amount", -1, 1, 0, 1, "") \
    X(f##n##_vel, "F" #n " Velocity", 0, 1, 0.2f, 1, "")

#define SB_ENV(X, n, a, d, s, r) \
    X(e##n##_a, "Env" #n " Attack", 0.001f, 8, a, 0.3f, "") \
    X(e##n##_d, "Env" #n " Decay", 0.005f, 8, d, 0.3f, "") \
    X(e##n##_s, "Env" #n " Sustain", 0, 1, s, 1, "") \
    X(e##n##_r, "Env" #n " Release", 0.005f, 10, r, 0.3f, "")

#define SB_LFO(X, n, shape, rate) \
    X(l##n##_shape, "LFO" #n " Shape", 0, 5, shape, 1, SB_LFOSHAPES) \
    X(l##n##_rate, "LFO" #n " Rate", 0.02f, 30, rate, 0.35f, "") \
    X(l##n##_sync, "LFO" #n " Sync", 0, 7, 0, 1, SB_SYNC)

#define SB_MOD(X, n) \
    X(m##n##_src, "Mod" #n " Source", 0, 21, 0, 1, SB_SRC) \
    X(m##n##_dst, "Mod" #n " Dest", 0, 23, 0, 1, SB_DST) \
    X(m##n##_amt, "Mod" #n " Amount", -1, 1, 0, 1, "")

#define SB_SEQ(X, n, p, g) \
    X(sp##n, "Seq Pitch " #n, -12, 12, p, 1, "") \
    X(sg##n, "Seq Gate " #n, 0, 1, g, 1, "") \
    X(sv##n, "Seq Vel " #n, 0, 1, 0.8f, 1, "")

#define SB_RACK(X, n) \
    X(r##n##_type, "FX" #n " Type", 0, 10, 0, 1, SB_FXTYPES) \
    X(r##n##_on, "FX" #n " On", 0, 1, 1, 1, "Off|On") \
    X(r##n##_a, "FX" #n " A", 0, 1, 0.5f, 1, "") \
    X(r##n##_b, "FX" #n " B", 0, 1, 0.5f, 1, "") \
    X(r##n##_c, "FX" #n " C", 0, 1, 0.5f, 1, "") \
    X(r##n##_mix, "FX" #n " Mix", 0, 1, 0.35f, 1, "")

#define SB_PARAMS(X) \
    SB_OSC(X, 1, 0, 0.8f) \
    SB_OSC(X, 2, 2, 0.0f) \
    X(sub_lvl, "Sub Level", 0, 1, 0, 1, "") \
    X(sub_wave, "Sub Wave", 0, 1, 0, 1, "Sine|Triangle") \
    X(sub_oct, "Sub Oct", 0, 1, 0, 1, "-1 Oct|-2 Oct") \
    X(noise_lvl, "Noise", 0, 1, 0, 1, "") \
    X(noise_col, "Noise Color", 0, 1, 1, 1, "White|Air") \
    X(uni_voices, "Unison Voices", 1, 16, 1, 1, "") \
    X(uni_detune, "Unison Detune", 0, 1, 0.3f, 1, "") \
    X(uni_curve, "Detune Curve", 0, 2, 2, 1, "Linear|Exponential|Supersaw") \
    X(uni_spread, "Unison Spread", 0, 1, 0.8f, 1, "") \
    X(uni_blend, "Unison Blend", 0, 1, 0.6f, 1, "") \
    X(uni_wtspread, "WT Spread", 0, 1, 0, 1, "") \
    SB_FILT(X, 1, 1, 18000) \
    SB_FILT(X, 2, 0, 20000) \
    X(f_route, "Routing", 0, 2, 0, 1, "Serial|Parallel|Osc1>F1 Osc2>F2") \
    SB_ENV(X, 1, 0.004f, 0.5f, 0.8f, 0.3f) \
    SB_ENV(X, 2, 0.003f, 0.4f, 0.2f, 0.3f) \
    SB_ENV(X, 3, 0.01f, 0.6f, 0.0f, 0.3f) \
    SB_LFO(X, 1, 0, 3) \
    SB_LFO(X, 2, 1, 0.4f) \
    SB_LFO(X, 3, 5, 1) \
    SB_LFO(X, 4, 4, 6) \
    X(mac1, "Macro 1", 0, 1, 0, 1, "") X(mac2, "Macro 2", 0, 1, 0, 1, "") X(mac3, "Macro 3", 0, 1, 0, 1, "") X(mac4, "Macro 4", 0, 1, 0, 1, "") \
    X(mac5, "Macro 5", 0, 1, 0, 1, "") X(mac6, "Macro 6", 0, 1, 0, 1, "") X(mac7, "Macro 7", 0, 1, 0, 1, "") X(mac8, "Macro 8", 0, 1, 0, 1, "") \
    SB_MOD(X, 1) SB_MOD(X, 2) SB_MOD(X, 3) SB_MOD(X, 4) SB_MOD(X, 5) SB_MOD(X, 6) SB_MOD(X, 7) SB_MOD(X, 8) SB_MOD(X, 9) SB_MOD(X, 10) \
    X(play_mode, "Voice Mode", 0, 2, 0, 1, "Poly|Mono|Legato") \
    X(glide, "Glide", 0, 2, 0, 0.4f, "") \
    X(bend_range, "Bend Range", 1, 24, 2, 1, "") \
    X(arp_on, "Arp", 0, 1, 0, 1, "Off|On") \
    X(arp_mode, "Arp Mode", 0, 5, 0, 1, "Up|Down|Up-Down|Random|As Played|Chord") \
    X(arp_rate, "Arp Rate", 0, 4, 2, 1, "1/4|1/8|1/16|1/16T|1/32") \
    X(arp_oct, "Arp Octaves", 1, 4, 2, 1, "") \
    X(arp_gate, "Arp Gate", 0.05f, 1, 0.6f, 1, "") \
    X(arp_swing, "Arp Swing", 0, 0.6f, 0, 1, "") \
    X(arp_hold, "Arp Hold", 0, 1, 0, 1, "Off|On") \
    X(seq_on, "Sequencer", 0, 1, 0, 1, "Off|On") \
    X(seq_rate, "Seq Rate", 0, 2, 1, 1, "1/8|1/16|1/32") \
    X(seq_len, "Seq Length", 1, 16, 16, 1, "") \
    SB_SEQ(X, 1, 0, 1) SB_SEQ(X, 2, 0, 1) SB_SEQ(X, 3, 12, 1) SB_SEQ(X, 4, 0, 1) SB_SEQ(X, 5, 0, 1) SB_SEQ(X, 6, 7, 1) SB_SEQ(X, 7, 0, 1) SB_SEQ(X, 8, 12, 1) \
    SB_SEQ(X, 9, 0, 1) SB_SEQ(X, 10, 0, 1) SB_SEQ(X, 11, 12, 1) SB_SEQ(X, 12, 0, 1) SB_SEQ(X, 13, 0, 1) SB_SEQ(X, 14, 7, 1) SB_SEQ(X, 15, 0, 1) SB_SEQ(X, 16, 12, 1) \
    SB_RACK(X, 1) SB_RACK(X, 2) SB_RACK(X, 3) SB_RACK(X, 4) SB_RACK(X, 5) SB_RACK(X, 6) SB_RACK(X, 7) SB_RACK(X, 8) \
    X(master, "Level", 0, 1, 0.75f, 1, "")

enum P : int
{
#define X(n, l, mn, mx, d, sk, ch) n,
    SB_PARAMS(X)
#undef X
    COUNT
};

struct Def { const char* id; const char* name; float mn, mx, def, skew; const char* choices; };

inline const Def* defs()
{
    static const Def d[COUNT] = {
#define X(n, l, mn, mx, dv, sk, ch) { "sb_" #n, l, (float) (mn), (float) (mx), (float) (dv), (float) (sk), ch },
        SB_PARAMS(X)
#undef X
    };
    return d;
}

enum Dest { D_NONE, D_PITCH, D_O1PITCH, D_O2PITCH, D_O1POS, D_O2POS, D_O1W1, D_O1W2, D_O2W1, D_O2W2, D_O1LVL, D_O2LVL, D_SUBLVL, D_NOISE,
            D_F1CUT, D_F1RES, D_F2CUT, D_F2RES, D_DETUNE, D_WTSPREAD, D_AMP, D_PAN, D_L1RATE, D_L2RATE, D_COUNT };
enum Src { S_NONE, S_LFO1, S_LFO2, S_LFO3, S_LFO4, S_ENV1, S_ENV2, S_ENV3, S_MAC1, S_MAC2, S_MAC3, S_MAC4, S_MAC5, S_MAC6, S_MAC7, S_MAC8,
           S_VEL, S_KEY, S_MOD, S_AT, S_RAND, S_BEND, S_COUNT };

// which modulation destination (if any) a parameter knob corresponds to - used by drag-and-drop modulation in the UI
inline int destForParam (int p)
{
    switch (p)
    {
        case o1_oct: case o1_semi: case o1_fine: return D_O1PITCH;
        case o2_oct: case o2_semi: case o2_fine: return D_O2PITCH;
        case o1_pos: return D_O1POS;   case o2_pos: return D_O2POS;
        case o1_w1a: return D_O1W1;    case o1_w2a: return D_O1W2;
        case o2_w1a: return D_O2W1;    case o2_w2a: return D_O2W2;
        case o1_lvl: return D_O1LVL;   case o2_lvl: return D_O2LVL;
        case sub_lvl: return D_SUBLVL; case noise_lvl: return D_NOISE;
        case f1_cut: return D_F1CUT;   case f1_res: return D_F1RES;
        case f2_cut: return D_F2CUT;   case f2_res: return D_F2RES;
        case uni_detune: return D_DETUNE; case uni_wtspread: return D_WTSPREAD;
        case master: return D_AMP;
        case l1_rate: return D_L1RATE; case l2_rate: return D_L2RATE;
        default: return D_NONE;
    }
}
} // namespace sb
