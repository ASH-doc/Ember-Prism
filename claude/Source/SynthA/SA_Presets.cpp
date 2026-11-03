#include "SA_Presets.h"

namespace sa
{
namespace
{
struct Bld
{
    Preset p;
    Bld (const char* n, const char* c) { p.name = n; p.category = c; }
    Bld& s (int idx, float v) { p.values.push_back ({ idx, v }); return *this; }
    Bld& m (int slot, int src, int dst, float amt) { return s (m1_src + 3 * slot, (float) src).s (m1_dst + 3 * slot, (float) dst).s (m1_amt + 3 * slot, amt); }
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

        // ------------------------------------------------------------------ BASSES (Moog-ladder warmth)
        L.push_back (Bld ("Moog Sub Bass", "Basses").s (o1_wave, 1).s (o1_oct, -1).s (o1_pw, 0.5f).s (o1_lvl, 0.9f).s (o2_wave, 0).s (o2_oct, -1).s (o2_fine, 5).s (o2_lvl, 0.65f)
            .s (sub_lvl, 0.5f).s (f1_cut, 380).s (f1_res, 0.35f).s (f1_drive, 0.35f).s (f1_env, 0.55f).s (f1_key, 0.3f).s (e2_d, 0.22f).s (e2_s, 0.15f).s (e1_d, 0.3f).s (e1_s, 0.85f).s (e1_r, 0.12f)
            .s (play_mode, 1).s (glide, 0.04f).s (warmth, 0.65f).s (age, 0.3f));
        L.push_back (Bld ("Ladder Rolling Bass", "Basses").s (o1_wave, 0).s (o1_oct, -1).s (o2_wave, 1).s (o2_oct, -1).s (o2_pw, 0.4f).s (o2_fine, 12).s (o2_lvl, 0.7f).s (sub_lvl, 0.35f)
            .s (f1_cut, 520).s (f1_res, 0.3f).s (f1_env, 0.6f).s (e2_d, 0.18f).s (e2_s, 0.1f).s (e1_d, 0.2f).s (e1_s, 0.6f).s (e1_r, 0.1f).s (fx_pump_on, 1).s (fx_pump_depth, 0.75f).s (warmth, 0.6f));
        L.push_back (Bld ("Acid Squelch", "Basses").s (o1_lvl, 1.0f).s (o2_lvl, 0).s (sub_lvl, 0).s (f1_cut, 600).s (f1_res, 0.82f).s (f1_drive, 0.5f).s (f1_env, 0.9f).s (f1_key, 0.6f)
            .s (e2_d, 0.18f).s (e2_s, 0).s (e1_d, 0.2f).s (e1_s, 0.8f).s (e1_r, 0.08f).s (play_mode, 1).s (glide, 0.08f).s (warmth, 0.5f).s (fx_drv_on, 1).s (fx_drv_type, 0).s (fx_drv_amt, 0.3f));
        L.push_back (Bld ("Soft Sine Sub", "Basses").s (o1_wave, 3).s (o1_oct, -1).s (o1_lvl, 1).s (o2_lvl, 0).s (sub_lvl, 0.4f).s (f1_cut, 700).s (f1_res, 0).s (f1_drive, 0).s (f1_env, 0.1f).s (play_mode, 1).s (warmth, 0.35f).s (age, 0.1f));
        L.push_back (Bld ("Pulse Pump Bass", "Basses").s (o1_wave, 1).s (o1_oct, -1).s (o1_pw, 0.3f).s (o2_wave, 0).s (o2_oct, -1).s (o2_lvl, 0.5f).s (f1_cut, 450).s (f1_res, 0.45f).s (f1_env, 0.5f)
            .s (e2_d, 0.2f).s (e2_s, 0.2f).s (fx_pump_on, 1).s (fx_pump_depth, 0.85f).s (fx_pump_rel, 0.5f).s (warmth, 0.55f).s (m1_src, S_LFO1).s (m1_dst, D_O1PW).s (m1_amt, 0.2f).s (l1_rate, 0.5f));
        L.push_back (Bld ("Dark Reese", "Basses").s (o1_wave, 0).s (o1_oct, -1).s (o2_wave, 0).s (o2_oct, -1).s (o2_fine, 18).s (o3_wave, 0).s (o3_oct, -2).s (o3_lvl, 0.5f).s (f1_cut, 420).s (f1_res, 0.25f).s (f1_drive, 0.4f)
            .s (f1_env, 0.3f).s (age, 0.5f).s (warmth, 0.7f).m (0, S_LFO1, D_F1CUT, 0.2f).s (l1_rate, 0.35f));

        // ------------------------------------------------------------------ LEADS
        L.push_back (Bld ("Hypersaw Anthem", "Leads").s (o1_wave, 4).s (o1_lvl, 0.8f).s (o2_wave, 4).s (o2_lvl, 0.55f).s (o2_fine, 9).s (uni_detune, 0.55f).s (uni_spread, 0.85f).s (uni_blend, 0.6f)
            .s (o3_wave, 0).s (o3_oct, -1).s (o3_lvl, 0.3f).s (sub_lvl, 0.2f).s (f1_cut, 5200).s (f1_res, 0.15f).s (f1_drive, 0.15f).s (f1_env, 0.25f).s (e2_d, 0.6f).s (e2_s, 0.5f)
            .s (e1_a, 0.005f).s (e1_s, 0.85f).s (e1_r, 0.35f).s (warmth, 0.35f).s (age, 0.3f).s (fx_ch_on, 1).s (fx_ch_mix, 0.3f).s (fx_dl_on, 1).s (fx_dl_time, 1).s (fx_dl_mix, 0.22f)
            .s (fx_rv_on, 1).s (fx_rv_size, 0.6f).s (fx_rv_mix, 0.22f));
        L.push_back (Bld ("Warm Supersaw", "Leads").s (o1_wave, 4).s (o2_wave, 4).s (o2_lvl, 0.5f).s (o2_fine, 6).s (uni_detune, 0.45f).s (f1_model, 0).s (f1_cut, 3200).s (f1_res, 0.22f).s (f1_drive, 0.35f)
            .s (f1_env, 0.2f).s (warmth, 0.6f).s (age, 0.35f).s (sub_lvl, 0.25f).s (e1_s, 0.9f).s (e1_r, 0.4f).s (fx_dl_on, 1).s (fx_dl_time, 2).s (fx_dl_mix, 0.2f).s (fx_rv_on, 1).s (fx_rv_mix, 0.2f));
        L.push_back (Bld ("Mono Glide Lead", "Leads").s (o1_wave, 0).s (o2_wave, 1).s (o2_pw, 0.4f).s (o2_fine, 6).s (o2_lvl, 0.5f).s (f1_cut, 3000).s (f1_res, 0.32f).s (f1_env, 0.4f)
            .s (play_mode, 1).s (glide, 0.12f).s (warmth, 0.5f).s (fx_dl_on, 1).s (fx_dl_time, 1).s (fx_dl_mix, 0.25f).s (fx_rv_on, 1).s (fx_rv_mix, 0.15f).m (0, S_MOD, D_PITCH, 0.0f));
        L.push_back (Bld ("Sync Sweep Lead", "Leads").s (o1_wave, 0).s (o2_wave, 0).s (o2_sync, 1).s (o2_oct, 1).s (o2_lvl, 0.8f).s (o2_fine, 0).s (f1_cut, 4200).s (f1_res, 0.2f).s (f1_env, 0.2f)
            .m (0, S_LFO2, D_O2PITCH, 0.3f).s (l2_shape, 1).s (l2_rate, 0.4f).s (warmth, 0.4f).s (play_mode, 1).s (glide, 0.05f).s (fx_dl_on, 1).s (fx_dl_mix, 0.2f));
        L.push_back (Bld ("Screamer Lead", "Leads").s (o1_wave, 0).s (o2_wave, 1).s (o2_oct, 0).s (o2_fine, 8).s (f1_model, 2).s (f1_cut, 2400).s (f1_res, 0.55f).s (f1_drive, 0.6f).s (f1_env, 0.35f)
            .s (e2_d, 0.4f).s (e2_s, 0.4f).s (warmth, 0.4f).s (fx_drv_on, 1).s (fx_drv_type, 2).s (fx_drv_amt, 0.3f).s (fx_dl_on, 1).s (fx_dl_mix, 0.18f));

        // ------------------------------------------------------------------ PLUCKS
        L.push_back (Bld ("Trance Pluck", "Plucks").s (o1_lvl, 0.9f).s (o2_fine, 7).s (o2_lvl, 0.7f).s (sub_lvl, 0.2f).s (f1_cut, 700).s (f1_res, 0.25f).s (f1_env, 0.85f)
            .s (e2_a, 0.001f).s (e2_d, 0.22f).s (e2_s, 0).s (e2_r, 0.2f).s (e1_a, 0.002f).s (e1_d, 0.35f).s (e1_s, 0).s (e1_r, 0.18f).s (warmth, 0.4f)
            .s (fx_dl_on, 1).s (fx_dl_time, 1).s (fx_dl_fb, 0.4f).s (fx_dl_mix, 0.3f).s (fx_rv_on, 1).s (fx_rv_mix, 0.2f));
        L.push_back (Bld ("Afterglow Pluck", "Plucks").s (o1_wave, 1).s (o1_pw, 0.35f).s (o2_wave, 0).s (o2_fine, 9).s (f1_cut, 900).s (f1_res, 0.35f).s (f1_env, 0.7f).s (e1_d, 0.5f).s (e1_s, 0).s (e1_r, 0.35f)
            .s (e2_d, 0.3f).s (e2_s, 0).s (fx_ch_on, 1).s (fx_ch_mix, 0.3f).s (fx_rv_on, 1).s (fx_rv_size, 0.7f).s (fx_rv_mix, 0.3f));
        L.push_back (Bld ("Hypersaw Pluck", "Plucks").s (o1_wave, 4).s (o2_wave, 4).s (o2_lvl, 0.5f).s (uni_detune, 0.4f).s (f1_cut, 900).s (f1_res, 0.2f).s (f1_env, 0.9f).s (e2_d, 0.28f).s (e2_s, 0).s (e1_d, 0.4f).s (e1_s, 0)
            .s (e1_r, 0.2f).s (fx_dl_on, 1).s (fx_dl_time, 1).s (fx_dl_mix, 0.28f).s (fx_rv_on, 1).s (fx_rv_mix, 0.22f).s (warmth, 0.5f));

        // ------------------------------------------------------------------ GATED
        L.push_back (Bld ("Gated Supersaw", "Gated").s (o1_wave, 4).s (o2_wave, 4).s (o2_lvl, 0.5f).s (uni_detune, 0.5f).s (f1_cut, 4200).s (f1_res, 0.15f).s (gate_on, 1).s (gate_rate, 1).s (gate_depth, 0.95f)
            .s (gate_smooth, 0.2f).s (e1_s, 0.9f).s (e1_r, 0.15f).s (warmth, 0.4f).s (fx_rv_on, 1).s (fx_rv_size, 0.65f).s (fx_rv_mix, 0.25f).s (fx_dl_on, 1).s (fx_dl_time, 2).s (fx_dl_mix, 0.18f));
        L.push_back (Bld ("Gated Warm Pad", "Gated").s (uni_voices, 4).s (uni_detune, 0.4f).s (o2_fine, 9).s (f1_cut, 1800).s (f1_res, 0.15f).s (gate_on, 1).s (gate_rate, 0).s (gate_depth, 0.8f).s (gate_smooth, 0.5f)
            .s (e1_a, 0.08f).s (e1_s, 1).s (e1_r, 0.6f).s (fx_rv_on, 1).s (fx_rv_mix, 0.3f).s (warmth, 0.55f).s (age, 0.4f));
        L.push_back (Bld ("Pumping Chord Stab", "Gated").s (o1_wave, 0).s (o2_wave, 0).s (o2_fine, 8).s (uni_voices, 3).s (uni_detune, 0.3f).s (f1_cut, 2600).s (f1_env, 0.3f).s (e1_d, 0.35f).s (e1_s, 0.4f).s (e1_r, 0.15f)
            .s (fx_pump_on, 1).s (fx_pump_depth, 0.7f).s (fx_pump_rel, 0.45f).s (fx_dl_on, 1).s (fx_dl_time, 2).s (fx_dl_mix, 0.25f).s (fx_rv_on, 1).s (fx_rv_mix, 0.2f));

        // ------------------------------------------------------------------ PADS
        L.push_back (Bld ("Warm Analog Pad", "Pads").s (uni_voices, 3).s (uni_detune, 0.3f).s (o2_fine, 9).s (f1_cut, 1100).s (f1_res, 0.15f).s (f1_env, 0.2f).s (e1_a, 0.6f).s (e1_d, 1).s (e1_s, 0.85f).s (e1_r, 1.4f)
            .s (e2_a, 0.8f).s (e2_d, 1.5f).s (e2_s, 0.5f).m (0, S_LFO1, D_F1CUT, 0.15f).s (l1_rate, 0.3f).s (fx_ch_on, 1).s (fx_ch_mix, 0.35f).s (fx_rv_on, 1).s (fx_rv_size, 0.75f).s (fx_rv_mix, 0.3f).s (age, 0.5f).s (warmth, 0.6f));
        L.push_back (Bld ("Cathedral Pad", "Pads").s (uni_voices, 5).s (uni_detune, 0.35f).s (o2_fine, 12).s (sub_lvl, 0.2f).s (f1_cut, 900).s (f1_res, 0.1f).s (e1_a, 1.2f).s (e1_s, 1).s (e1_r, 2.2f)
            .s (fx_rv_on, 1).s (fx_rv_size, 0.92f).s (fx_rv_damp, 0.5f).s (fx_rv_mix, 0.45f).s (fx_ch_on, 1).s (fx_ch_mix, 0.3f).s (warmth, 0.5f).s (age, 0.45f));
        L.push_back (Bld ("String Ensemble", "Pads").s (o1_wave, 0).s (o2_wave, 1).s (o2_pw, 0.45f).s (o2_fine, 5).s (uni_voices, 3).s (uni_detune, 0.25f).s (f1_cut, 3000).s (f1_key, 0.7f).s (f1_env, 0.1f)
            .s (e1_a, 0.25f).s (e1_s, 1).s (e1_r, 0.8f).s (fx_ch_on, 1).s (fx_ch_mix, 0.45f).s (fx_ph_on, 1).s (fx_ph_mix, 0.2f).s (fx_ph_depth, 0.4f).s (fx_rv_on, 1).s (fx_rv_mix, 0.25f));
        L.push_back (Bld ("Dark Drone", "Pads").s (o1_oct, -2).s (o2_oct, -2).s (o2_fine, 14).s (uni_voices, 3).s (uni_detune, 0.4f).s (f1_cut, 500).s (f1_res, 0.3f).s (f1_drive, 0.4f).m (0, S_LFO1, D_F1CUT, 0.3f).s (l1_rate, 0.12f)
            .s (e1_a, 1.8f).s (e1_s, 1).s (e1_r, 3).s (age, 0.8f).s (warmth, 0.7f).s (fx_rv_on, 1).s (fx_rv_size, 0.9f).s (fx_rv_mix, 0.35f));

        // ------------------------------------------------------------------ ARPS
        L.push_back (Bld ("Trance Arp 138", "Arps").s (arp_on, 1).s (arp_mode, 0).s (arp_rate, 2).s (arp_oct, 2).s (arp_gate, 0.5f).s (f1_cut, 1500).s (f1_res, 0.3f).s (f1_env, 0.6f).s (e2_d, 0.18f).s (e2_s, 0.1f)
            .s (e1_d, 0.22f).s (e1_s, 0.4f).s (e1_r, 0.15f).s (fx_dl_on, 1).s (fx_dl_time, 1).s (fx_dl_fb, 0.4f).s (fx_dl_mix, 0.3f).s (fx_rv_on, 1).s (fx_rv_mix, 0.2f));
        L.push_back (Bld ("Acid Arp", "Arps").s (arp_on, 1).s (arp_mode, 2).s (arp_rate, 2).s (arp_oct, 2).s (arp_gate, 0.45f).s (o2_lvl, 0).s (f1_cut, 700).s (f1_res, 0.7f).s (f1_env, 0.8f).s (e2_d, 0.16f).s (e2_s, 0)
            .s (e1_d, 0.2f).s (e1_s, 0.3f).s (warmth, 0.5f).s (fx_drv_on, 1).s (fx_drv_type, 0).s (fx_drv_amt, 0.25f).s (fx_dl_on, 1).s (fx_dl_mix, 0.22f));
        L.push_back (Bld ("Chord Stab Arp", "Arps").s (arp_on, 1).s (arp_mode, 5).s (arp_rate, 1).s (arp_gate, 0.3f).s (uni_voices, 3).s (uni_detune, 0.3f).s (f1_cut, 2400).s (f1_env, 0.4f).s (e1_d, 0.3f).s (e1_s, 0.3f)
            .s (fx_dl_on, 1).s (fx_dl_time, 2).s (fx_dl_mix, 0.3f).s (fx_rv_on, 1).s (fx_rv_mix, 0.2f));

        // ------------------------------------------------------------------ CLASSIC POLYS / KEYS
        L.push_back (Bld ("Juno Brass", "Classic").s (f1_model, 1).s (f1_cut, 2200).s (f1_res, 0.2f).s (f1_env, 0.6f).s (e2_a, 0.06f).s (e2_d, 0.5f).s (e2_s, 0.5f).s (o2_fine, 5).s (e1_a, 0.03f).s (e1_s, 0.9f).s (e1_r, 0.25f)
            .s (fx_ch_on, 1).s (fx_ch_mix, 0.4f).s (warmth, 0.5f));
        L.push_back (Bld ("Vintage Poly PWM", "Classic").s (o1_wave, 1).s (o2_wave, 1).s (o2_pw, 0.4f).s (o2_fine, 6).s (f1_model, 1).s (f1_cut, 2000).s (f1_res, 0.15f).m (0, S_LFO3, D_O1PW, 0.3f).s (l3_shape, 0).s (l3_rate, 0.6f)
            .s (e1_a, 0.05f).s (e1_s, 0.85f).s (e1_r, 0.5f).s (fx_ch_on, 1).s (fx_ch_mix, 0.45f).s (warmth, 0.5f).s (age, 0.4f));
        L.push_back (Bld ("Electric Keys", "Classic").s (o1_wave, 2).s (o2_wave, 1).s (o2_pw, 0.4f).s (o2_lvl, 0.4f).s (f1_cut, 3500).s (f1_vel, 0.6f).s (f1_env, 0.3f).s (e1_d, 0.8f).s (e1_s, 0.3f).s (e1_r, 0.4f)
            .s (fx_ch_on, 1).s (fx_ch_mix, 0.3f).s (fx_rv_on, 1).s (fx_rv_mix, 0.15f));
        L.push_back (Bld ("Warm Organ", "Classic").s (o1_wave, 3).s (o2_wave, 3).s (o2_oct, 1).s (o2_fine, 0).s (o2_lvl, 0.5f).s (o3_wave, 3).s (o3_oct, 2).s (o3_fine, 0).s (o3_lvl, 0.25f).s (sub_lvl, 0.3f).s (f1_cut, 9000).s (f1_env, 0).s (f1_drive, 0.3f)
            .s (e1_a, 0.01f).s (e1_d, 0.1f).s (e1_s, 1).s (e1_r, 0.08f).s (fx_ch_on, 1).s (fx_ch_mix, 0.4f).s (fx_ph_on, 1).s (fx_ph_mix, 0.2f).s (warmth, 0.55f));
        L.push_back (Bld ("Ring Bell", "Classic").s (o1_wave, 3).s (o2_wave, 3).s (o2_oct, 1).s (o2_semi, 7).s (o2_ring, 0.7f).s (o2_lvl, 0.6f).s (f1_cut, 8000).s (f1_env, 0).s (e1_d, 1.6f).s (e1_s, 0).s (e1_r, 1.2f)
            .s (fx_rv_on, 1).s (fx_rv_size, 0.7f).s (fx_rv_mix, 0.3f).s (warmth, 0.2f));

        // ------------------------------------------------------------------ FX
        L.push_back (Bld ("Noise Riser", "FX").s (o1_lvl, 0).s (o2_lvl, 0).s (sub_lvl, 0).s (noise_lvl, 1).s (noise_col, 0).s (f1_model, 1).s (f1_type, 1).s (f1_cut, 200).s (f1_res, 0.5f).s (f1_env, 1).s (f1_key, 0)
            .s (e2_a, 6).s (e2_d, 0.1f).s (e2_s, 1).s (e1_a, 4).s (e1_s, 1).s (e1_r, 2).s (fx_rv_on, 1).s (fx_rv_size, 0.8f).s (fx_rv_mix, 0.3f).s (fx_ph_on, 1).s (fx_ph_mix, 0.4f));
        L.push_back (Bld ("Sub Impact", "FX").s (o1_lvl, 0).s (o2_lvl, 0).s (sub_lvl, 0.9f).s (noise_lvl, 0.5f).s (noise_col, 1).s (f1_cut, 600).s (f1_env, 0.5f).s (e2_d, 0.4f).s (e2_s, 0).s (e1_d, 1.2f).s (e1_s, 0).s (e1_r, 1.5f)
            .s (fx_drv_on, 1).s (fx_drv_amt, 0.3f).s (fx_rv_on, 1).s (fx_rv_size, 0.9f).s (fx_rv_mix, 0.4f));
        // ================================================================== batch 2
        // ---- more basses
        L.push_back (Bld ("Offbeat Trance Bass", "Basses").s (o1_wave, 0).s (o1_oct, -1).s (o2_wave, 1).s (o2_oct, -1).s (o2_pw, 0.45f).s (o2_fine, 8).s (o2_lvl, 0.6f).s (sub_lvl, 0.3f).s (f1_cut, 600).s (f1_res, 0.3f).s (f1_env, 0.5f)
            .s (e2_d, 0.15f).s (e2_s, 0.2f).s (e1_d, 0.15f).s (e1_s, 0.55f).s (e1_r, 0.08f).s (fx_pump_on, 1).s (fx_pump_depth, 0.8f).s (fx_pump_rel, 0.5f).s (warmth, 0.6f));
        L.push_back (Bld ("Moog Wobble", "Basses").s (o1_wave, 0).s (o1_oct, -1).s (o2_wave, 0).s (o2_oct, -1).s (o2_fine, 6).s (sub_lvl, 0.4f).s (f1_cut, 500).s (f1_res, 0.45f).s (f1_drive, 0.3f).s (f1_env, 0.1f)
            .m (0, S_LFO1, D_F1CUT, 0.55f).s (l1_shape, 1).s (l1_sync, 4).s (play_mode, 1).s (warmth, 0.65f));
        L.push_back (Bld ("Growl Bass", "Basses").s (o1_wave, 0).s (o1_oct, -1).s (o2_wave, 1).s (o2_oct, -1).s (o2_pw, 0.3f).s (f1_model, 2).s (f1_cut, 700).s (f1_res, 0.6f).s (f1_drive, 0.6f).s (f1_env, 0.3f)
            .m (0, S_LFO2, D_F1CUT, 0.4f).s (l2_shape, 0).s (l2_rate, 3).m (1, S_LFO3, D_O2PW, 0.3f).s (l3_shape, 1).s (l3_rate, 0.8f).s (fx_drv_on, 1).s (fx_drv_type, 2).s (fx_drv_amt, 0.35f).s (play_mode, 1).s (warmth, 0.5f));
        L.push_back (Bld ("Deep Sub Pulse", "Basses").s (o1_wave, 1).s (o1_pw, 0.5f).s (o1_oct, -2).s (o1_lvl, 0.9f).s (o2_lvl, 0).s (sub_lvl, 0.6f).s (f1_cut, 300).s (f1_res, 0.2f).s (f1_env, 0.3f).s (e1_d, 0.4f).s (e1_s, 0.8f).s (play_mode, 1).s (warmth, 0.6f));
        L.push_back (Bld ("Saw Stack Bass", "Basses").s (o1_wave, 4).s (o1_oct, -1).s (o2_wave, 4).s (o2_oct, -1).s (uni_detune, 0.35f).s (sub_lvl, 0.4f).s (f1_cut, 800).s (f1_res, 0.25f).s (f1_env, 0.5f).s (e2_d, 0.25f).s (e2_s, 0.25f).s (e1_s, 0.8f).s (warmth, 0.55f));
        // ---- more leads
        L.push_back (Bld ("Trance Anthem Stack", "Leads").s (o1_wave, 4).s (o2_wave, 4).s (o2_fine, 11).s (o2_lvl, 0.6f).s (o3_wave, 0).s (o3_oct, 1).s (o3_lvl, 0.25f).s (uni_detune, 0.6f).s (uni_spread, 0.9f).s (f1_cut, 6500).s (f1_res, 0.12f).s (f1_env, 0.2f)
            .s (e1_s, 0.9f).s (e1_r, 0.4f).s (fx_ch_on, 1).s (fx_ch_mix, 0.3f).s (fx_dl_on, 1).s (fx_dl_time, 2).s (fx_dl_mix, 0.25f).s (fx_rv_on, 1).s (fx_rv_size, 0.7f).s (fx_rv_mix, 0.25f).s (warmth, 0.35f));
        L.push_back (Bld ("Lush Wide Saw", "Leads").s (master, 0.55f).s (o1_wave, 4).s (o2_wave, 0).s (uni_voices, 8).s (o2_fine, 5).s (o2_lvl, 0.5f).s (uni_detune, 0.45f).s (uni_spread, 1).s (f1_model, 1).s (f1_cut, 4800).s (f1_res, 0.1f).s (e1_a, 0.02f).s (e1_s, 0.9f).s (e1_r, 0.5f)
            .s (fx_ch_on, 1).s (fx_ch_mix, 0.4f).s (fx_rv_on, 1).s (fx_rv_size, 0.8f).s (fx_rv_mix, 0.3f).s (warmth, 0.4f));
        L.push_back (Bld ("PWM Pulse Lead", "Leads").s (o1_wave, 1).s (o2_wave, 1).s (o2_fine, 7).s (o2_lvl, 0.6f).s (f1_cut, 3500).s (f1_res, 0.2f).s (f1_env, 0.3f).m (0, S_LFO1, D_O1PW, 0.35f).m (1, S_LFO2, D_O2PW, -0.3f).s (l1_rate, 0.7f).s (l2_rate, 0.9f)
            .s (play_mode, 1).s (glide, 0.05f).s (fx_dl_on, 1).s (fx_dl_mix, 0.22f).s (warmth, 0.5f));
        L.push_back (Bld ("Vintage Mono Solo", "Leads").s (o1_wave, 0).s (o2_wave, 2).s (o2_fine, 4).s (o2_lvl, 0.5f).s (f1_cut, 2400).s (f1_res, 0.35f).s (f1_drive, 0.3f).s (f1_env, 0.35f).s (play_mode, 2).s (glide, 0.1f)
            .m (0, S_MOD, D_F1CUT, 0.3f).s (warmth, 0.6f).s (age, 0.4f).s (fx_dl_on, 1).s (fx_dl_mix, 0.2f));
        L.push_back (Bld ("Euphoric Climax", "Leads").s (master, 0.6f).s (o1_wave, 4).s (o2_wave, 4).s (o2_lvl, 0.6f).s (o2_fine, 10).s (uni_detune, 0.65f).s (sub_lvl, 0.2f).s (f1_cut, 8000).s (f1_env, 0.15f).s (e1_a, 0.01f).s (e1_s, 1).s (e1_r, 0.6f)
            .s (fx_ch_on, 1).s (fx_dl_on, 1).s (fx_dl_time, 3).s (fx_dl_fb, 0.5f).s (fx_dl_mix, 0.3f).s (fx_rv_on, 1).s (fx_rv_size, 0.9f).s (fx_rv_mix, 0.35f));
        L.push_back (Bld ("Bright Sync Lead", "Leads").s (o2_sync, 1).s (o2_oct, 1).s (o2_semi, 5).s (o2_lvl, 0.9f).s (o1_lvl, 0.4f).s (f1_cut, 5500).s (f1_res, 0.15f).m (0, S_ENV3, D_O2PITCH, 0.4f).s (e3_d, 0.5f).s (e3_s, 0.3f)
            .s (play_mode, 1).s (glide, 0.04f).s (fx_dl_on, 1).s (fx_dl_mix, 0.2f));
        // ---- more plucks
        L.push_back (Bld ("Short Pluck", "Plucks").s (o1_lvl, 0.9f).s (o2_fine, 6).s (f1_cut, 600).s (f1_res, 0.3f).s (f1_env, 0.9f).s (e2_d, 0.14f).s (e2_s, 0).s (e1_d, 0.2f).s (e1_s, 0).s (e1_r, 0.1f).s (fx_dl_on, 1).s (fx_dl_time, 1).s (fx_dl_mix, 0.25f));
        L.push_back (Bld ("Rubber Pluck", "Plucks").s (o1_wave, 1).s (o1_pw, 0.4f).s (o2_wave, 0).s (o2_oct, -1).s (f1_cut, 500).s (f1_res, 0.5f).s (f1_env, 0.9f).s (e2_d, 0.18f).s (e2_s, 0).s (e1_d, 0.3f).s (e1_s, 0).s (e1_r, 0.15f)
            .m (0, S_ENV2, D_PITCH, 0.04f).s (warmth, 0.55f));
        L.push_back (Bld ("Pulse Pluck Echo", "Plucks").s (o1_wave, 1).s (o1_pw, 0.35f).s (o2_wave, 1).s (o2_pw, 0.6f).s (o2_fine, 9).s (f1_model, 1).s (f1_cut, 1500).s (f1_env, 0.6f).s (e2_d, 0.25f).s (e2_s, 0).s (e1_d, 0.4f).s (e1_s, 0)
            .s (fx_dl_on, 1).s (fx_dl_time, 2).s (fx_dl_fb, 0.5f).s (fx_dl_mix, 0.35f).s (fx_rv_on, 1).s (fx_rv_mix, 0.2f));
        L.push_back (Bld ("Supersaw Pluck Long", "Plucks").s (o1_wave, 4).s (o2_wave, 4).s (o2_lvl, 0.5f).s (uni_detune, 0.45f).s (f1_cut, 1100).s (f1_env, 0.8f).s (e2_d, 0.45f).s (e2_s, 0.05f).s (e1_d, 0.7f).s (e1_s, 0).s (e1_r, 0.4f)
            .s (fx_ch_on, 1).s (fx_rv_on, 1).s (fx_rv_size, 0.75f).s (fx_rv_mix, 0.3f).s (fx_dl_on, 1).s (fx_dl_mix, 0.25f));
        L.push_back (Bld ("Acid Pluck", "Plucks").s (o1_lvl, 1).s (o2_lvl, 0).s (f1_cut, 450).s (f1_res, 0.75f).s (f1_drive, 0.4f).s (f1_env, 0.95f).s (e2_d, 0.15f).s (e2_s, 0).s (e1_d, 0.25f).s (e1_s, 0).s (e1_r, 0.1f).s (warmth, 0.5f));
        // ---- more gated
        L.push_back (Bld ("Gated Strings", "Gated").s (o1_wave, 0).s (o2_wave, 0).s (uni_voices, 4).s (uni_detune, 0.3f).s (o2_fine, 7).s (f1_cut, 3200).s (f1_env, 0.1f).s (gate_on, 1).s (gate_rate, 1).s (gate_depth, 0.9f).s (gate_smooth, 0.35f)
            .s (e1_a, 0.15f).s (e1_s, 1).s (e1_r, 0.5f).s (fx_ch_on, 1).s (fx_rv_on, 1).s (fx_rv_mix, 0.25f));
        L.push_back (Bld ("Choppy Stab", "Gated").s (o1_wave, 4).s (o2_wave, 4).s (o2_lvl, 0.5f).s (f1_cut, 3800).s (f1_res, 0.2f).s (gate_on, 1).s (gate_rate, 2).s (gate_depth, 1).s (gate_smooth, 0.05f).s (e1_s, 1).s (e1_r, 0.1f)
            .s (fx_dl_on, 1).s (fx_dl_time, 2).s (fx_dl_mix, 0.2f));
        L.push_back (Bld ("Gated Dark Pad", "Gated").s (uni_voices, 4).s (uni_detune, 0.4f).s (f1_cut, 1000).s (f1_res, 0.2f).s (f1_drive, 0.3f).s (gate_on, 1).s (gate_rate, 0).s (gate_depth, 0.85f).s (gate_smooth, 0.6f).s (e1_a, 0.1f).s (e1_s, 1).s (e1_r, 0.7f)
            .s (age, 0.5f).s (warmth, 0.65f).s (fx_rv_on, 1).s (fx_rv_size, 0.8f).s (fx_rv_mix, 0.3f).s (fx_pump_on, 1).s (fx_pump_depth, 0.4f));
        // ---- more pads
        L.push_back (Bld ("Evolving Warm Pad", "Pads").s (uni_voices, 4).s (uni_detune, 0.3f).s (o2_wave, 1).s (o2_pw, 0.5f).s (o2_fine, 8).s (f1_cut, 1300).s (f1_res, 0.2f).s (f1_env, 0.2f).m (0, S_LFO1, D_F1CUT, 0.25f).s (l1_rate, 0.15f)
            .m (1, S_LFO2, D_O2PW, 0.3f).s (l2_rate, 0.11f).s (e1_a, 1).s (e1_s, 1).s (e1_r, 2).s (e2_a, 1.5f).s (e2_s, 0.6f).s (fx_ch_on, 1).s (fx_rv_on, 1).s (fx_rv_size, 0.8f).s (fx_rv_mix, 0.3f).s (age, 0.5f).s (warmth, 0.6f));
        L.push_back (Bld ("Slow Strings", "Pads").s (o1_wave, 0).s (o2_wave, 0).s (uni_voices, 3).s (uni_detune, 0.2f).s (o2_fine, 6).s (f1_cut, 2600).s (f1_key, 0.7f).s (e1_a, 0.8f).s (e1_s, 1).s (e1_r, 1.5f).s (fx_ch_on, 1).s (fx_ch_mix, 0.5f).s (fx_rv_on, 1).s (fx_rv_mix, 0.3f));
        L.push_back (Bld ("Ambient Wash", "Pads").s (uni_voices, 5).s (uni_detune, 0.45f).s (sub_lvl, 0.2f).s (f1_cut, 800).s (f1_res, 0.15f).m (0, S_LFO1, D_F1CUT, 0.3f).s (l1_rate, 0.08f).s (e1_a, 2.5f).s (e1_s, 1).s (e1_r, 4)
            .s (fx_ch_on, 1).s (fx_ph_on, 1).s (fx_ph_rate, 0.15f).s (fx_ph_mix, 0.3f).s (fx_dl_on, 1).s (fx_dl_time, 4).s (fx_dl_mix, 0.3f).s (fx_rv_on, 1).s (fx_rv_size, 0.95f).s (fx_rv_mix, 0.5f).s (warmth, 0.5f));
        L.push_back (Bld ("Jupiter Pad", "Pads").s (f1_model, 1).s (f1_cut, 2200).s (f1_res, 0.15f).s (o1_wave, 0).s (o2_wave, 0).s (o2_fine, 7).s (uni_voices, 2).s (uni_detune, 0.2f).s (e1_a, 0.35f).s (e1_s, 0.9f).s (e1_r, 1).s (fx_ch_on, 1).s (fx_ch_mix, 0.45f).s (fx_rv_on, 1).s (fx_rv_mix, 0.25f).s (warmth, 0.5f));
        // ---- more arps
        L.push_back (Bld ("Rolling Arp Up", "Arps").s (arp_on, 1).s (arp_mode, 0).s (arp_rate, 2).s (arp_oct, 3).s (arp_gate, 0.55f).s (o1_wave, 1).s (o1_pw, 0.4f).s (f1_cut, 1800).s (f1_res, 0.3f).s (f1_env, 0.5f).s (e2_d, 0.2f).s (e2_s, 0.1f)
            .s (e1_d, 0.2f).s (e1_s, 0.3f).s (fx_dl_on, 1).s (fx_dl_time, 2).s (fx_dl_mix, 0.3f).s (fx_rv_on, 1).s (fx_rv_mix, 0.2f));
        L.push_back (Bld ("Bright Down Arp", "Arps").s (arp_on, 1).s (arp_mode, 1).s (arp_rate, 2).s (arp_oct, 2).s (arp_gate, 0.4f).s (f1_cut, 3500).s (f1_res, 0.2f).s (f1_env, 0.4f).s (e1_d, 0.15f).s (e1_s, 0.2f).s (fx_dl_on, 1).s (fx_dl_time, 1).s (fx_dl_mix, 0.3f));
        L.push_back (Bld ("Random Pluck Arp", "Arps").s (arp_on, 1).s (arp_mode, 3).s (arp_rate, 2).s (arp_oct, 2).s (arp_gate, 0.35f).s (f1_cut, 1200).s (f1_res, 0.35f).s (f1_env, 0.7f).s (e2_d, 0.15f).s (e2_s, 0).s (e1_d, 0.18f).s (e1_s, 0)
            .s (fx_dl_on, 1).s (fx_dl_time, 2).s (fx_dl_mix, 0.3f).s (fx_rv_on, 1).s (fx_rv_mix, 0.2f));
        L.push_back (Bld ("Swing Arp", "Arps").s (arp_on, 1).s (arp_mode, 2).s (arp_rate, 1).s (arp_oct, 2).s (arp_swing, 0.35f).s (arp_gate, 0.5f).s (f1_cut, 2200).s (f1_env, 0.4f).s (e1_d, 0.25f).s (e1_s, 0.3f).s (fx_ch_on, 1).s (fx_dl_on, 1).s (fx_dl_mix, 0.25f));
        // ---- keys / classic
        L.push_back (Bld ("Mellow Keys", "Classic").s (o1_wave, 3).s (o2_wave, 2).s (o2_lvl, 0.4f).s (f1_cut, 3000).s (f1_vel, 0.5f).s (f1_env, 0.2f).s (e1_d, 1).s (e1_s, 0.25f).s (e1_r, 0.5f).s (fx_ch_on, 1).s (fx_ch_mix, 0.3f).s (fx_rv_on, 1).s (fx_rv_mix, 0.2f).s (warmth, 0.4f));
        L.push_back (Bld ("Pulse Clav", "Classic").s (o1_wave, 1).s (o1_pw, 0.25f).s (o2_lvl, 0).s (f1_model, 1).s (f1_type, 2).s (f1_cut, 900).s (f1_res, 0.2f).s (f1_env, 0.5f).s (e2_d, 0.15f).s (e2_s, 0).s (e1_d, 0.25f).s (e1_s, 0).s (e1_r, 0.1f));
        L.push_back (Bld ("Strings And Brass", "Classic").s (o1_wave, 0).s (o2_wave, 1).s (o2_pw, 0.4f).s (o2_fine, 6).s (uni_voices, 2).s (uni_detune, 0.2f).s (f1_model, 1).s (f1_cut, 2400).s (f1_res, 0.18f).s (f1_env, 0.4f).s (e2_a, 0.08f).s (e2_d, 0.6f).s (e2_s, 0.6f)
            .s (e1_a, 0.06f).s (e1_s, 0.9f).s (e1_r, 0.4f).s (fx_ch_on, 1).s (fx_ch_mix, 0.4f).s (warmth, 0.5f).s (age, 0.35f));
        // ---- more FX
        L.push_back (Bld ("Downlifter", "FX").s (o1_lvl, 0.6f).s (o2_lvl, 0.5f).s (noise_lvl, 0.7f).s (f1_cut, 9000).s (f1_res, 0.4f).s (f1_env, -1).s (f1_key, 0).s (e2_a, 0.01f).s (e2_d, 3.5f).s (e2_s, 0).s (e1_a, 0.01f).s (e1_d, 3.5f).s (e1_s, 0).s (e1_r, 2)
            .s (fx_rv_on, 1).s (fx_rv_size, 0.85f).s (fx_rv_mix, 0.35f));
        L.push_back (Bld ("Laser Zap", "FX").s (o1_wave, 3).s (o1_lvl, 0.9f).s (o2_lvl, 0).s (f1_cut, 8000).s (f1_env, 0).m (0, S_ENV3, D_PITCH, 0.7f).s (e3_a, 0.001f).s (e3_d, 0.22f).s (e3_s, 0).s (e1_d, 0.35f).s (e1_s, 0).s (e1_r, 0.2f)
            .s (fx_dl_on, 1).s (fx_dl_time, 1).s (fx_dl_mix, 0.3f));
        L.push_back (Bld ("Dark Impact", "FX").s (o1_lvl, 0).s (o2_lvl, 0).s (sub_lvl, 1).s (sub_oct, 1).s (noise_lvl, 0.6f).s (noise_col, 1).s (f1_cut, 400).s (f1_env, 0.5f).s (e2_d, 0.8f).s (e2_s, 0).s (e1_d, 2).s (e1_s, 0).s (e1_r, 2)
            .s (fx_drv_on, 1).s (fx_drv_amt, 0.4f).s (fx_rv_on, 1).s (fx_rv_size, 1).s (fx_rv_mix, 0.5f));
        L.push_back (Bld ("Sweep Up", "FX").s (o1_lvl, 0.5f).s (o2_lvl, 0.5f).s (o2_fine, 15).s (noise_lvl, 0.5f).s (f1_cut, 250).s (f1_res, 0.55f).s (f1_env, 1).s (f1_key, 0).s (e2_a, 5).s (e2_d, 0.1f).s (e2_s, 1).s (e1_a, 3).s (e1_s, 1).s (e1_r, 1.5f)
            .s (fx_ph_on, 1).s (fx_ph_mix, 0.4f).s (fx_rv_on, 1).s (fx_rv_mix, 0.3f));        return L;
    }();
    return list;
}
} // namespace sa
