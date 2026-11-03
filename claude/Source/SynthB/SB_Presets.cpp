#include "SB_Presets.h"

namespace sb
{
namespace
{
struct Bld
{
    Preset p;
    Bld (const char* n, const char* c) { p.name = n; p.category = c; }
    Bld& s (int idx, float v) { p.values.push_back ({ idx, v }); return *this; }
    Bld& m (int slot, int src, int dst, float amt) { return s (m1_src + 3 * slot, (float) src).s (m1_dst + 3 * slot, (float) dst).s (m1_amt + 3 * slot, amt); }
    // rack slot: type, a, b, c, mix   (types: 1 chorus 2 phaser 3 delay 4 reverb 5 distortion 6 eq 7 compressor 8 filter 9 widener 10 flanger)
    Bld& fx (int slot, int type, float a, float b, float c, float mix) { const int base = r1_type + 6 * slot; return s (base, (float) type).s (base + 1, 1).s (base + 2, a).s (base + 3, b).s (base + 4, c).s (base + 5, mix); }
    Bld& shape (int lfo, std::vector<float> pts) { p.shapes.push_back ({ lfo, std::move (pts) }); return *this; }
    operator Preset() const { return p; }
};
}

std::vector<float> presetValues (int index)
{
    std::vector<float> out (COUNT);
    for (int i = 0; i < COUNT; ++i) out[(size_t) i] = defs()[i].def;
    const auto& all = presets();
    if (index >= 0 && index < (int) all.size())
        for (auto& kv : all[(size_t) index].values) out[(size_t) kv.first] = kv.second;
    return out;
}

const std::vector<Preset>& presets()
{
    static const std::vector<Preset> list = [] {
        std::vector<Preset> L;
        L.push_back (Bld ("Init", "Init"));

        // ------------------------------------------------------------------ PLUCKS
        L.push_back (Bld ("Glass Pluck", "Plucks").s (o1_table, 7).s (o1_pos, 0.3f).s (uni_voices, 3).s (uni_detune, 0.15f).s (f1_type, 1).s (f1_cut, 3500).s (f1_res, 0.2f).s (f1_env, 0.5f)
            .s (e1_d, 0.35f).s (e1_s, 0).s (e1_r, 0.25f).s (e2_d, 0.25f).s (e2_s, 0).fx (0, 3, 0.2f, 0.4f, 0.4f, 0.28f).fx (1, 4, 0.45f, 0.8f, 0.5f, 0.25f));
        L.push_back (Bld ("Crystal Pluck", "Plucks").s (o1_table, 11).s (o1_pos, 0.5f).s (o1_w1, 2).s (o1_w1a, 0.3f).s (f1_type, 0).s (f1_cut, 6000).s (f1_env, 0.3f).s (e1_d, 0.4f).s (e1_s, 0).s (e1_r, 0.35f)
            .m (0, S_ENV2, D_O1POS, 0.5f).s (e2_d, 0.3f).s (e2_s, 0).fx (0, 3, 0.2f, 0.35f, 0.5f, 0.3f).fx (1, 4, 0.55f, 0.9f, 0.9f, 0.3f));
        L.push_back (Bld ("Digital Edge Pluck", "Plucks").s (o1_table, 8).s (o1_pos, 0.4f).s (o1_w1, 1).s (o1_w1a, 0.25f).s (o2_table, 0).s (o2_lvl, 0.4f).s (o2_oct, 1).s (f1_type, 1).s (f1_cut, 2500).s (f1_res, 0.3f).s (f1_env, 0.7f)
            .s (e1_d, 0.3f).s (e1_s, 0).s (e1_r, 0.2f).s (e2_d, 0.2f).s (e2_s, 0).fx (0, 5, 0.25f, 0.2f, 0.8f, 0.2f).fx (1, 3, 0.2f, 0.4f, 0.6f, 0.25f).fx (2, 4, 0.4f, 0.8f, 0.6f, 0.2f));
        L.push_back (Bld ("Bell Pluck", "Plucks").s (o1_table, 25).s (o1_pos, 0.25f).s (f1_cut, 12000).s (e1_d, 0.8f).s (e1_s, 0).s (e1_r, 0.6f).m (0, S_ENV3, D_O1POS, 0.4f).s (e3_d, 0.4f).s (e3_s, 0)
            .fx (0, 4, 0.5f, 0.9f, 0.9f, 0.3f));

        // ------------------------------------------------------------------ LEADS
        L.push_back (Bld ("Supersaw Prism", "Leads").s (o1_table, 0).s (o1_pos, 0).s (o2_table, 0).s (o2_lvl, 0.5f).s (o2_fine, 7).s (uni_voices, 12).s (uni_detune, 0.5f).s (uni_curve, 2).s (uni_spread, 1).s (uni_blend, 0.7f)
            .s (f1_type, 1).s (f1_cut, 9000).s (f1_env, 0.2f).s (e2_d, 0.5f).s (e2_s, 0.5f).s (e1_s, 0.9f).s (e1_r, 0.3f).fx (0, 1, 0.3f, 0.5f, 0.6f, 0.35f).fx (1, 9, 0.7f, 0.2f, 0.4f, 1.0f).fx (2, 4, 0.5f, 0.9f, 0.8f, 0.22f));
        L.push_back (Bld ("Airy Lead", "Leads").s (o1_table, 5).s (o1_pos, 0.15f).s (uni_voices, 6).s (uni_detune, 0.3f).s (f1_type, 1).s (f1_cut, 11000).s (f2_type, 7).s (f2_cut, 8000).s (f2_res, 0.5f).s (f2_mix, 1)
            .s (e1_s, 0.9f).s (e1_r, 0.4f).fx (0, 3, 0.35f, 0.4f, 0.6f, 0.25f).fx (1, 4, 0.6f, 0.9f, 1.0f, 0.3f));
        L.push_back (Bld ("Formant Lead", "Leads").s (o1_table, 3).s (o1_pos, 0).s (uni_voices, 3).s (uni_detune, 0.2f).s (f1_type, 6).s (f1_cut, 180).s (f1_res, 0.55f).s (f1_key, 0).m (0, S_LFO1, D_F1CUT, 0.45f).s (l1_rate, 0.4f)
            .s (f2_type, 1).s (f2_cut, 9000).s (play_mode, 1).s (glide, 0.06f).fx (0, 3, 0.35f, 0.4f, 0.5f, 0.22f).fx (1, 4, 0.5f, 0.8f, 0.7f, 0.2f));
        L.push_back (Bld ("Screaming Digital", "Leads").s (o1_table, 6).s (o1_pos, 0.5f).s (o1_w1, 6).s (o1_w1a, 0.35f).s (o2_table, 22).s (o2_pos, 0.3f).s (o2_lvl, 0.5f).s (uni_voices, 4).s (uni_detune, 0.25f)
            .s (f1_type, 1).s (f1_cut, 5000).s (f1_res, 0.25f).m (0, S_LFO1, D_O1POS, 0.3f).s (l1_rate, 0.6f).fx (0, 5, 0.35f, 0.5f, 0.7f, 0.4f).fx (1, 3, 0.2f, 0.4f, 0.5f, 0.22f).fx (2, 4, 0.5f, 0.8f, 0.7f, 0.2f));
        L.push_back (Bld ("Wide Wavetable Lead", "Leads").s (o1_table, 3).s (o1_pos, 0.7f).s (uni_voices, 16).s (uni_detune, 0.4f).s (uni_wtspread, 0.55f).s (uni_spread, 1).s (f1_type, 1).s (f1_cut, 10000)
            .s (e1_s, 0.9f).fx (0, 1, 0.25f, 0.6f, 0.7f, 0.3f).fx (1, 3, 0.35f, 0.45f, 0.6f, 0.25f).fx (2, 4, 0.55f, 0.9f, 0.9f, 0.25f));

        // ------------------------------------------------------------------ BASSES
        L.push_back (Bld ("Rolling Digital Bass", "Basses").s (o1_table, 1).s (o1_pos, 0.2f).s (o1_oct, -1).s (sub_lvl, 0.5f).s (f1_type, 1).s (f1_cut, 600).s (f1_res, 0.3f).s (f1_env, 0.6f).s (e2_d, 0.2f).s (e2_s, 0.1f)
            .s (e1_d, 0.25f).s (e1_s, 0.7f).s (e1_r, 0.1f).s (play_mode, 1).fx (0, 7, 0.6f, 0.5f, 0.3f, 1.0f));
        L.push_back (Bld ("Edgy Reese", "Basses").s (o1_table, 4).s (o1_pos, 0.3f).s (o1_oct, -1).s (o2_table, 4).s (o2_pos, 0.3f).s (o2_oct, -1).s (o2_fine, -14).s (o2_lvl, 0.7f).s (uni_voices, 2).s (uni_detune, 0.2f)
            .s (f1_type, 0).s (f1_cut, 900).s (f1_res, 0.25f).m (0, S_LFO1, D_F1CUT, 0.3f).s (l1_rate, 0.3f).fx (0, 5, 0.3f, 0.2f, 0.5f, 0.4f).fx (1, 7, 0.55f, 0.5f, 0.3f, 1.0f));
        L.push_back (Bld ("Sub Glass", "Basses").s (o1_table, 3).s (o1_pos, 0).s (o1_oct, -1).s (sub_lvl, 0.6f).s (f1_type, 1).s (f1_cut, 500).s (f1_res, 0.05f).s (play_mode, 1).s (glide, 0.03f).s (e1_s, 1).s (e1_r, 0.1f));
        L.push_back (Bld ("Movement Bass", "Basses").s (o1_table, 10).s (o1_pos, 0.4f).s (o1_oct, -1).s (f1_type, 1).s (f1_cut, 1200).s (f1_res, 0.3f).s (f1_env, 0.4f).m (0, S_LFO2, D_O1POS, 0.6f).s (l2_rate, 0.6f).s (l2_shape, 1)
            .s (e2_d, 0.3f).s (e2_s, 0.3f).s (sub_lvl, 0.4f).s (play_mode, 1).fx (0, 5, 0.3f, 0.5f, 0.6f, 0.35f));

        // ------------------------------------------------------------------ PADS
        L.push_back (Bld ("Airy Pad", "Pads").s (o1_table, 27).s (o1_pos, 0.3f).s (o2_table, 5).s (o2_lvl, 0.5f).s (uni_voices, 6).s (uni_detune, 0.3f).s (e1_a, 0.8f).s (e1_d, 1).s (e1_s, 0.85f).s (e1_r, 1.5f)
            .s (f1_type, 0).s (f1_cut, 9000).s (f2_type, 7).s (f2_cut, 7000).s (f2_res, 0.6f).fx (0, 1, 0.2f, 0.6f, 0.8f, 0.4f).fx (1, 4, 0.8f, 0.9f, 0.9f, 0.4f));
        L.push_back (Bld ("Shimmer Pad", "Pads").s (o1_table, 9).s (o1_pos, 0.3f).s (uni_voices, 8).s (uni_detune, 0.35f).s (uni_wtspread, 0.3f).s (e1_a, 1).s (e1_s, 1).s (e1_r, 2).m (0, S_LFO1, D_O1POS, 0.5f).s (l1_rate, 0.15f)
            .s (f1_type, 0).s (f1_cut, 10000).fx (0, 1, 0.25f, 0.7f, 0.9f, 0.4f).fx (1, 2, 0.15f, 0.5f, 0.4f, 0.3f).fx (2, 4, 0.9f, 0.95f, 1.0f, 0.5f));
        L.push_back (Bld ("Choir Pad", "Pads").s (o1_table, 13).s (o1_pos, 0.4f).s (uni_voices, 4).s (uni_detune, 0.2f).s (e1_a, 0.7f).s (e1_s, 1).s (e1_r, 1.5f).m (0, S_LFO1, D_O1POS, 0.25f).s (l1_rate, 0.2f)
            .s (f1_type, 0).s (f1_cut, 6000).fx (0, 1, 0.2f, 0.5f, 0.7f, 0.3f).fx (1, 4, 0.7f, 0.7f, 0.7f, 0.35f));
        L.push_back (Bld ("Glass Strings", "Pads").s (o1_table, 19).s (o1_pos, 0.5f).s (uni_voices, 6).s (uni_detune, 0.28f).s (e1_a, 0.35f).s (e1_s, 1).s (e1_r, 1).s (f1_type, 0).s (f1_cut, 7500)
            .fx (0, 1, 0.3f, 0.6f, 0.8f, 0.4f).fx (1, 4, 0.65f, 0.85f, 0.8f, 0.3f));

        // ------------------------------------------------------------------ TEXTURES
        L.push_back (Bld ("Evolving Texture", "Textures").s (o1_table, 20).s (o1_pos, 0.5f).s (o2_table, 28).s (o2_lvl, 0.5f).s (uni_voices, 4).s (uni_detune, 0.3f).m (0, S_LFO1, D_O1POS, 0.8f).s (l1_rate, 0.12f)
            .m (1, S_LFO2, D_O2POS, 0.6f).s (l2_rate, 0.09f).s (f1_type, 6).s (f1_cut, 200).s (f1_res, 0.4f).m (2, S_LFO3, D_F1CUT, 0.4f).s (l3_shape, 0).s (l3_rate, 0.07f).s (e1_a, 1.5f).s (e1_s, 1).s (e1_r, 2.5f)
            .fx (0, 3, 0.5f, 0.55f, 0.7f, 0.3f).fx (1, 4, 0.85f, 0.8f, 0.9f, 0.45f));
        L.push_back (Bld ("Wavetable Scan", "Textures").s (o1_table, 6).s (o1_pos, 0.5f).m (0, S_LFO1, D_O1POS, 0.9f).s (l1_rate, 0.4f).s (l1_shape, 1).s (uni_voices, 5).s (uni_detune, 0.25f).s (f1_type, 1).s (f1_cut, 9000)
            .s (e1_a, 0.1f).s (e1_s, 1).s (e1_r, 0.8f).fx (0, 1, 0.3f, 0.6f, 0.7f, 0.3f).fx (1, 4, 0.7f, 0.8f, 0.8f, 0.3f));
        L.push_back (Bld ("Breath Texture", "Textures").s (o1_table, 29).s (o1_pos, 0.6f).s (o2_table, 28).s (o2_lvl, 0.6f).s (noise_lvl, 0.2f).s (noise_col, 1).s (f1_type, 3).s (f1_cut, 2500).s (f1_res, 0.3f).m (0, S_LFO1, D_F1CUT, 0.4f).s (l1_rate, 0.2f)
            .s (e1_a, 1.2f).s (e1_s, 1).s (e1_r, 2).fx (0, 4, 0.85f, 0.7f, 0.9f, 0.5f));

        // ------------------------------------------------------------------ ARPS / SEQUENCES
        L.push_back (Bld ("Trance Arp 138", "Arps").s (o1_table, 7).s (o1_pos, 0.35f).s (arp_on, 1).s (arp_mode, 0).s (arp_rate, 2).s (arp_oct, 2).s (arp_gate, 0.5f).s (f1_type, 1).s (f1_cut, 2500).s (f1_res, 0.25f).s (f1_env, 0.5f)
            .s (e1_d, 0.25f).s (e1_s, 0.2f).s (e1_r, 0.2f).s (e2_d, 0.2f).s (e2_s, 0).fx (0, 3, 0.2f, 0.4f, 0.6f, 0.3f).fx (1, 4, 0.5f, 0.9f, 0.7f, 0.25f));
        L.push_back (Bld ("Step Sequence Glass", "Arps").s (o1_table, 11).s (o1_pos, 0.45f).s (seq_on, 1).s (seq_rate, 1).s (seq_len, 16).s (f1_type, 1).s (f1_cut, 4000).s (f1_res, 0.2f).s (f1_env, 0.4f)
            .s (e1_d, 0.2f).s (e1_s, 0.1f).s (e1_r, 0.15f).s (e2_d, 0.15f).s (e2_s, 0).fx (0, 3, 0.2f, 0.4f, 0.6f, 0.3f).fx (1, 4, 0.5f, 0.9f, 0.8f, 0.25f));
        L.push_back (Bld ("Marimba Arp", "Arps").s (o1_table, 21).s (o1_pos, 0.6f).s (arp_on, 1).s (arp_mode, 2).s (arp_rate, 1).s (arp_oct, 2).s (arp_gate, 0.6f).s (e1_d, 0.4f).s (e1_s, 0).s (e1_r, 0.3f).s (f1_cut, 9000)
            .fx (0, 4, 0.5f, 0.9f, 0.9f, 0.3f));

        // ------------------------------------------------------------------ KEYS / BELLS
        L.push_back (Bld ("Digital Bell", "Bells").s (o1_table, 25).s (o1_pos, 0.4f).s (f1_cut, 14000).s (e1_d, 2).s (e1_s, 0).s (e1_r, 1.2f).fx (0, 4, 0.6f, 0.9f, 1.0f, 0.3f));
        L.push_back (Bld ("Glass Keys", "Bells").s (o1_table, 24).s (o1_pos, 0.3f).s (f1_type, 0).s (f1_cut, 9000).s (e1_d, 1.2f).s (e1_s, 0.1f).s (e1_r, 0.6f).s (f1_vel, 0.5f).fx (0, 1, 0.3f, 0.5f, 0.6f, 0.25f).fx (1, 4, 0.5f, 0.9f, 0.8f, 0.2f));
        L.push_back (Bld ("Mallet Stack", "Bells").s (o1_table, 26).s (o1_pos, 0.7f).s (e1_d, 0.9f).s (e1_s, 0).s (e1_r, 0.5f).s (f1_cut, 10000).fx (0, 1, 0.3f, 0.5f, 0.7f, 0.3f).fx (1, 4, 0.55f, 0.85f, 0.8f, 0.3f));

        // ------------------------------------------------------------------ VOCAL / FORMANT
        L.push_back (Bld ("Vowel Morph", "Vocal").s (o1_table, 12).s (o1_pos, 0.5f).m (0, S_LFO1, D_O1POS, 0.9f).s (l1_rate, 0.25f).s (uni_voices, 2).s (uni_detune, 0.15f).s (e1_a, 0.1f).s (e1_s, 1).s (e1_r, 0.5f)
            .s (f1_type, 0).s (f1_cut, 7000).fx (0, 1, 0.2f, 0.4f, 0.5f, 0.3f).fx (1, 4, 0.5f, 0.7f, 0.6f, 0.25f));
        L.push_back (Bld ("Talk Lead", "Vocal").s (o1_table, 14).s (o1_pos, 0.3f).m (0, S_ENV3, D_O1POS, 0.6f).s (e3_a, 0.3f).s (e3_d, 0.5f).s (e3_s, 0.2f).s (play_mode, 1).s (glide, 0.08f).s (uni_voices, 2).s (uni_detune, 0.12f)
            .s (f1_type, 1).s (f1_cut, 6000).fx (0, 3, 0.35f, 0.35f, 0.5f, 0.22f));
        L.push_back (Bld ("Reed Solo", "Vocal").s (o1_table, 16).s (o1_pos, 0.2f).s (play_mode, 1).s (glide, 0.04f).s (e1_a, 0.03f).s (e1_s, 0.9f).s (e1_r, 0.2f).s (f1_type, 0).s (f1_cut, 5500).fx (0, 4, 0.4f, 0.6f, 0.5f, 0.2f));
        // ================================================================== batch 2
        // ---- more plucks
        L.push_back (Bld ("FM Glass Pluck", "Plucks").s (o1_table, 22).s (o1_pos, 0.15f).m (0, S_ENV2, D_O1POS, 0.5f).s (e2_d, 0.3f).s (e2_s, 0).s (e1_d, 0.4f).s (e1_s, 0).s (e1_r, 0.3f).s (f1_type, 0).s (f1_cut, 9000)
            .fx (0, 3, 0.2f, 0.4f, 0.5f, 0.28f).fx (1, 4, 0.5f, 0.9f, 0.9f, 0.28f));
        L.push_back (Bld ("Wide Marimba Pluck", "Plucks").s (o1_table, 21).s (o1_pos, 0.5f).s (uni_voices, 4).s (uni_detune, 0.12f).s (e1_d, 0.5f).s (e1_s, 0).s (e1_r, 0.4f).s (f1_cut, 10000).fx (0, 1, 0.3f, 0.5f, 0.8f, 0.3f).fx (1, 4, 0.5f, 0.85f, 0.8f, 0.25f));
        L.push_back (Bld ("Soft Digital Pluck", "Plucks").s (o1_table, 3).s (o1_pos, 0.3f).s (f1_type, 1).s (f1_cut, 2500).s (f1_env, 0.5f).s (e2_d, 0.3f).s (e2_s, 0).s (e1_d, 0.35f).s (e1_s, 0).s (e1_r, 0.3f).fx (0, 3, 0.2f, 0.35f, 0.5f, 0.25f));
        L.push_back (Bld ("Hollow Pluck", "Plucks").s (o1_table, 4).s (o1_pos, 0.6f).s (o2_table, 4).s (o2_pos, 0.2f).s (o2_lvl, 0.4f).s (o2_oct, 1).s (f1_type, 1).s (f1_cut, 3000).s (f1_env, 0.6f).s (e2_d, 0.25f).s (e2_s, 0).s (e1_d, 0.4f).s (e1_s, 0).s (e1_r, 0.3f)
            .fx (0, 3, 0.2f, 0.4f, 0.6f, 0.3f).fx (1, 4, 0.55f, 0.8f, 0.7f, 0.25f));
        L.push_back (Bld ("Vocal Pluck", "Plucks").s (o1_table, 12).s (o1_pos, 0.6f).m (0, S_ENV2, D_O1POS, -0.4f).s (e2_d, 0.3f).s (e2_s, 0).s (e1_d, 0.4f).s (e1_s, 0).s (e1_r, 0.3f).s (f1_cut, 9000).fx (0, 3, 0.2f, 0.4f, 0.5f, 0.25f).fx (1, 4, 0.5f, 0.8f, 0.7f, 0.2f));
        L.push_back (Bld ("Sync Pluck", "Plucks").s (o1_table, 0).s (o1_w1, 1).s (o1_w1a, 0.2f).m (0, S_ENV2, D_O1W1, 0.7f).s (e2_d, 0.3f).s (e2_s, 0).s (e1_d, 0.35f).s (e1_s, 0).s (e1_r, 0.25f).s (f1_type, 1).s (f1_cut, 5000).fx (0, 3, 0.2f, 0.35f, 0.5f, 0.25f));
        // ---- more leads
        L.push_back (Bld ("Formant Scream", "Leads").s (o1_table, 14).s (o1_pos, 0.4f).s (o1_w1, 2).s (o1_w1a, 0.3f).s (uni_voices, 4).s (uni_detune, 0.25f).s (f1_type, 6).s (f1_cut, 400).s (f1_res, 0.6f).m (0, S_LFO1, D_F1CUT, 0.35f).s (l1_rate, 0.5f)
            .s (play_mode, 1).s (glide, 0.05f).fx (0, 5, 0.3f, 0.3f, 0.7f, 0.3f).fx (1, 3, 0.3f, 0.4f, 0.6f, 0.22f));
        L.push_back (Bld ("Bend Lead", "Leads").s (o1_table, 5).s (o1_pos, 0.2f).s (o1_w1, 2).s (o1_w1a, 0.4f).m (0, S_LFO1, D_O1W1, 0.4f).s (l1_rate, 0.35f).s (uni_voices, 5).s (uni_detune, 0.3f).s (f1_type, 1).s (f1_cut, 9000)
            .s (e1_s, 0.9f).fx (0, 1, 0.3f, 0.5f, 0.7f, 0.3f).fx (1, 3, 0.35f, 0.4f, 0.6f, 0.22f).fx (2, 4, 0.5f, 0.9f, 0.8f, 0.22f));
        L.push_back (Bld ("Mirror Lead", "Leads").s (o1_table, 6).s (o1_pos, 0.45f).s (o1_w1, 4).s (o1_w1a, 0.5f).s (uni_voices, 4).s (uni_detune, 0.25f).s (f1_type, 1).s (f1_cut, 7000).m (0, S_LFO2, D_O1POS, 0.3f).s (l2_rate, 0.3f)
            .s (e1_s, 0.9f).fx (0, 9, 0.7f, 0.2f, 0.4f, 1.0f).fx (1, 3, 0.35f, 0.4f, 0.6f, 0.2f).fx (2, 4, 0.5f, 0.9f, 0.8f, 0.22f));
        L.push_back (Bld ("Soft Sine Lead", "Leads").s (o1_table, 3).s (o1_pos, 0.05f).s (o2_table, 3).s (o2_pos, 0.05f).s (o2_lvl, 0.4f).s (o2_oct, 1).s (play_mode, 1).s (glide, 0.08f).s (f1_cut, 8000).s (e1_s, 0.95f).m (0, S_MOD, D_PITCH, 0.0f)
            .fx (0, 3, 0.35f, 0.4f, 0.5f, 0.25f).fx (1, 4, 0.5f, 0.9f, 0.8f, 0.25f));
        L.push_back (Bld ("Pulse Sweep Lead", "Leads").s (o1_table, 2).s (o1_pos, 0.3f).m (0, S_LFO1, D_O1POS, 0.5f).s (l1_rate, 0.8f).s (uni_voices, 6).s (uni_detune, 0.3f).s (f1_type, 1).s (f1_cut, 8000).s (e1_s, 0.9f)
            .fx (0, 1, 0.3f, 0.5f, 0.7f, 0.3f).fx (1, 3, 0.35f, 0.4f, 0.6f, 0.22f).fx (2, 4, 0.5f, 0.9f, 0.8f, 0.22f));
        L.push_back (Bld ("Octave Stack Lead", "Leads").s (o1_table, 0).s (o2_table, 0).s (o2_oct, 1).s (o2_lvl, 0.5f).s (uni_voices, 7).s (uni_detune, 0.4f).s (f1_type, 1).s (f1_cut, 8500).s (e1_s, 0.9f).s (e1_r, 0.35f)
            .fx (0, 1, 0.3f, 0.5f, 0.7f, 0.3f).fx (1, 9, 0.7f, 0.2f, 0.4f, 1.0f).fx (2, 4, 0.55f, 0.9f, 0.8f, 0.22f));
        // ---- more basses
        L.push_back (Bld ("Wobble Digital", "Basses").s (o1_table, 4).s (o1_pos, 0.3f).s (o1_oct, -1).s (f1_type, 1).s (f1_cut, 500).s (f1_res, 0.45f).m (0, S_LFO1, D_F1CUT, 0.6f).s (l1_shape, 1).s (l1_sync, 4).s (sub_lvl, 0.4f).s (play_mode, 1)
            .fx (0, 5, 0.25f, 0.3f, 0.6f, 0.3f));
        L.push_back (Bld ("FM Sub Bass", "Basses").s (o1_table, 22).s (o1_pos, 0.2f).s (o1_oct, -1).m (0, S_ENV2, D_O1POS, 0.3f).s (e2_d, 0.25f).s (e2_s, 0.2f).s (sub_lvl, 0.5f).s (f1_type, 1).s (f1_cut, 900).s (play_mode, 1));
        L.push_back (Bld ("Acid Digital", "Basses").s (o1_table, 0).s (o1_pos, 0).s (o1_oct, -1).s (f1_type, 1).s (f1_cut, 600).s (f1_res, 0.75f).s (f1_env, 0.9f).s (f1_drive, 0.3f).s (e2_d, 0.18f).s (e2_s, 0).s (e1_d, 0.2f).s (e1_s, 0.8f).s (e1_r, 0.08f)
            .s (play_mode, 1).s (glide, 0.07f).fx (0, 5, 0.3f, 0.2f, 0.6f, 0.3f));
        L.push_back (Bld ("Sub Pulse", "Basses").s (o1_table, 2).s (o1_pos, 0.2f).s (o1_oct, -2).s (sub_lvl, 0.5f).s (f1_type, 1).s (f1_cut, 400).s (f1_env, 0.3f).s (e2_d, 0.2f).s (e2_s, 0.3f).s (play_mode, 1));
        // ---- more pads
        L.push_back (Bld ("Frozen Glass Pad", "Pads").s (o1_table, 7).s (o1_pos, 0.5f).s (o2_table, 11).s (o2_pos, 0.4f).s (o2_lvl, 0.5f).s (uni_voices, 6).s (uni_detune, 0.3f).s (uni_wtspread, 0.4f).m (0, S_LFO1, D_O1POS, 0.3f).s (l1_rate, 0.1f)
            .s (e1_a, 1.2f).s (e1_s, 1).s (e1_r, 2.5f).s (f1_type, 0).s (f1_cut, 9000).fx (0, 1, 0.25f, 0.6f, 0.9f, 0.4f).fx (1, 4, 0.9f, 0.9f, 1.0f, 0.5f));
        L.push_back (Bld ("Slow Vowel Pad", "Pads").s (o1_table, 12).s (o1_pos, 0.3f).m (0, S_LFO1, D_O1POS, 0.8f).s (l1_rate, 0.07f).s (uni_voices, 4).s (uni_detune, 0.2f).s (e1_a, 1.5f).s (e1_s, 1).s (e1_r, 2.5f).s (f1_type, 0).s (f1_cut, 6000)
            .fx (0, 1, 0.2f, 0.5f, 0.8f, 0.35f).fx (1, 4, 0.85f, 0.7f, 0.8f, 0.45f));
        L.push_back (Bld ("Wide Ocean Pad", "Pads").s (o1_table, 5).s (o1_pos, 0.4f).s (o2_table, 27).s (o2_lvl, 0.6f).s (uni_voices, 8).s (uni_detune, 0.35f).s (uni_spread, 1).s (e1_a, 1.5f).s (e1_s, 1).s (e1_r, 3).s (f1_type, 0).s (f1_cut, 8000)
            .fx (0, 1, 0.2f, 0.7f, 0.9f, 0.4f).fx (1, 9, 0.9f, 0.2f, 0.4f, 1.0f).fx (2, 4, 0.9f, 0.9f, 1.0f, 0.5f));
        L.push_back (Bld ("Organ Pad", "Pads").s (o1_table, 17).s (o1_pos, 0.4f).m (0, S_LFO1, D_O1POS, 0.3f).s (l1_rate, 0.2f).s (uni_voices, 3).s (uni_detune, 0.1f).s (e1_a, 0.3f).s (e1_s, 1).s (e1_r, 0.8f).s (f1_type, 0).s (f1_cut, 7000)
            .fx (0, 1, 0.25f, 0.5f, 0.7f, 0.35f).fx (1, 4, 0.6f, 0.8f, 0.7f, 0.3f));
        L.push_back (Bld ("Bell Pad", "Pads").s (o1_table, 25).s (o1_pos, 0.5f).m (0, S_LFO1, D_O1POS, 0.3f).s (l1_rate, 0.12f).s (uni_voices, 3).s (uni_detune, 0.1f).s (e1_a, 0.8f).s (e1_s, 1).s (e1_r, 2).s (f1_cut, 12000)
            .fx (0, 1, 0.2f, 0.6f, 0.8f, 0.35f).fx (1, 4, 0.85f, 0.9f, 1.0f, 0.45f));
        // ---- more textures
        L.push_back (Bld ("Digital Wind", "Textures").s (o1_table, 28).s (o1_pos, 0.6f).m (0, S_LFO1, D_O1POS, 0.5f).s (l1_rate, 0.1f).s (noise_lvl, 0.2f).s (noise_col, 1).s (f1_type, 3).s (f1_cut, 3000).s (f1_res, 0.35f).m (1, S_LFO2, D_F1CUT, 0.5f).s (l2_rate, 0.07f)
            .s (e1_a, 2).s (e1_s, 1).s (e1_r, 3).fx (0, 3, 0.5f, 0.5f, 0.7f, 0.3f).fx (1, 4, 0.9f, 0.7f, 0.9f, 0.5f));
        L.push_back (Bld ("Stepped Texture", "Textures").s (o1_table, 8).s (o1_pos, 0.5f).m (0, S_LFO4, D_O1POS, 0.8f).s (l4_shape, 4).s (l4_sync, 5).s (uni_voices, 3).s (uni_detune, 0.2f).s (f1_type, 1).s (f1_cut, 5000).s (e1_a, 0.3f).s (e1_s, 1).s (e1_r, 1)
            .fx (0, 3, 0.35f, 0.5f, 0.7f, 0.3f).fx (1, 4, 0.7f, 0.8f, 0.8f, 0.35f));
        L.push_back (Bld ("Metallic Drone", "Textures").s (o1_table, 10).s (o1_pos, 0.3f).s (o1_oct, -1).m (0, S_LFO1, D_O1POS, 0.6f).s (l1_rate, 0.13f).s (o2_table, 10).s (o2_pos, 0.7f).s (o2_lvl, 0.5f).s (o2_fine, 9).s (f1_type, 1).s (f1_cut, 3500)
            .s (e1_a, 2).s (e1_s, 1).s (e1_r, 3).fx (0, 2, 0.15f, 0.6f, 0.5f, 0.35f).fx (1, 4, 0.9f, 0.6f, 0.7f, 0.45f));
        // ---- more arps / sequences
        L.push_back (Bld ("Arp Glass Up", "Arps").s (o1_table, 11).s (o1_pos, 0.4f).s (arp_on, 1).s (arp_mode, 0).s (arp_rate, 2).s (arp_oct, 3).s (arp_gate, 0.45f).s (f1_type, 1).s (f1_cut, 4000).s (f1_env, 0.4f).s (e1_d, 0.2f).s (e1_s, 0.15f).s (e2_d, 0.15f).s (e2_s, 0)
            .fx (0, 3, 0.2f, 0.4f, 0.6f, 0.3f).fx (1, 4, 0.55f, 0.9f, 0.9f, 0.25f));
        L.push_back (Bld ("Seq Pulse Bass", "Arps").s (o1_table, 2).s (o1_pos, 0.25f).s (o1_oct, -1).s (seq_on, 1).s (seq_rate, 1).s (seq_len, 16).s (sub_lvl, 0.4f).s (f1_type, 1).s (f1_cut, 900).s (f1_res, 0.3f).s (f1_env, 0.5f).s (e2_d, 0.15f).s (e2_s, 0.1f).s (e1_d, 0.15f).s (e1_s, 0.5f).s (e1_r, 0.08f));
        L.push_back (Bld ("Arp Vowel", "Arps").s (o1_table, 12).s (o1_pos, 0.5f).m (0, S_ENV2, D_O1POS, 0.5f).s (e2_d, 0.2f).s (e2_s, 0).s (arp_on, 1).s (arp_mode, 2).s (arp_rate, 1).s (arp_oct, 2).s (arp_gate, 0.5f).s (e1_d, 0.25f).s (e1_s, 0.2f).s (f1_cut, 8000)
            .fx (0, 3, 0.2f, 0.4f, 0.6f, 0.25f).fx (1, 4, 0.5f, 0.8f, 0.8f, 0.25f));
        L.push_back (Bld ("Sixteenth Sequence", "Arps").s (o1_table, 6).s (o1_pos, 0.5f).s (seq_on, 1).s (seq_rate, 1).s (seq_len, 8).s (f1_type, 1).s (f1_cut, 3500).s (f1_res, 0.25f).s (f1_env, 0.5f).s (e1_d, 0.18f).s (e1_s, 0.1f).s (e2_d, 0.15f).s (e2_s, 0)
            .fx (0, 3, 0.2f, 0.4f, 0.6f, 0.3f).fx (1, 4, 0.5f, 0.9f, 0.8f, 0.25f));
        // ---- more bells / keys
        L.push_back (Bld ("Crystal Bells", "Bells").s (o1_table, 25).s (o1_pos, 0.6f).s (o2_table, 26).s (o2_pos, 0.5f).s (o2_lvl, 0.4f).s (o2_oct, 1).s (e1_d, 2.5f).s (e1_s, 0).s (e1_r, 1.5f).s (f1_cut, 15000).fx (0, 4, 0.7f, 0.95f, 1.0f, 0.35f));
        L.push_back (Bld ("Tine Keys", "Bells").s (o1_table, 23).s (o1_pos, 0.25f).m (0, S_ENV2, D_O1POS, 0.3f).s (e2_d, 0.5f).s (e2_s, 0).s (e1_d, 1.5f).s (e1_s, 0.05f).s (e1_r, 0.6f).s (f1_vel, 0.5f).s (f1_cut, 9000)
            .fx (0, 1, 0.25f, 0.4f, 0.6f, 0.25f).fx (1, 4, 0.5f, 0.85f, 0.8f, 0.2f));
        L.push_back (Bld ("Organ Keys", "Bells").s (o1_table, 17).s (o1_pos, 0.7f).s (e1_a, 0.01f).s (e1_d, 0.1f).s (e1_s, 1).s (e1_r, 0.1f).s (f1_cut, 8000).fx (0, 1, 0.3f, 0.5f, 0.6f, 0.3f).fx (1, 4, 0.5f, 0.8f, 0.6f, 0.2f));
        // ---- vocal
        L.push_back (Bld ("Choir Lead", "Vocal").s (o1_table, 13).s (o1_pos, 0.45f).m (0, S_LFO1, D_O1POS, 0.3f).s (l1_rate, 0.3f).s (uni_voices, 3).s (uni_detune, 0.15f).s (play_mode, 1).s (glide, 0.06f).s (e1_a, 0.05f).s (e1_s, 1).s (e1_r, 0.4f)
            .s (f1_type, 0).s (f1_cut, 7000).fx (0, 3, 0.35f, 0.4f, 0.6f, 0.22f).fx (1, 4, 0.6f, 0.7f, 0.7f, 0.3f));
        L.push_back (Bld ("Nasal Sweep Lead", "Vocal").s (o1_table, 15).s (o1_pos, 0.3f).m (0, S_LFO1, D_O1POS, 0.6f).s (l1_rate, 0.4f).s (uni_voices, 2).s (uni_detune, 0.15f).s (e1_s, 0.9f).s (f1_type, 1).s (f1_cut, 6500).fx (0, 3, 0.35f, 0.4f, 0.5f, 0.22f).fx (1, 4, 0.5f, 0.8f, 0.7f, 0.2f));        return L;
    }();
    return list;
}
} // namespace sb
