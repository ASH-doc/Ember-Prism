#include "SpireToEmber.h"
#include "SpireDecoder.h"
#include "SpireTables.h"
#include "sa-parameters.h"
#include <cmath>
#include <map>

namespace aether::import {
namespace {

using juce::String;

const char* const kSpireWaves[49] = { "Sin","Tri","TriSine","Para 1","Para 2","Para 3","Para 4","Bass 1","Bass 2","Bass 3","Bass 4","Piano A1","Piano A2","Piano A3","Piano A4","Violin 1","Violin 2","Violin 3","Oboe","Sitar","Bayan 1","Bayan 2","Guitar 1","Guitar 2","Guitar 3","Organ 1","Organ 2","Organ 3","Brass 1","Brass 2","Vocal 1","Vocal 2","Vocal 3","Vocal 4","Vocal 5","Vocal 6","Robo 1","Robo 2","Robo 3","Synth 1","Synth 2","Synth 3","Inter 1","Inter 2","FM 1","FM 2","FM 3","Alarm 1","Alarm 2" };
const char* const kOscTypes[7] = { "Classic","Noise","FM","AMSync","SawPWM","HardFM","Vowel" };
const char* const kFilterTypes[7] = { "off","Perfecto","Acido","Infecto","Scorpio","Combo","Shaper" };
// Mode lists per filter type (names as in Spire's own tables).
const std::vector<std::vector<const char*>> kFilterModes = { {}, { "LP4","BP4","HP4","Peak" }, { "LP1","LP2","LP3","LP4" }, { "LP2","BP2","HP2","Notch" }, { "RedLP2","RedLP4","BlackLP2","BlackLP4","BlackHP4","BlackBP4" }, { "Mono+","Mono-","Stereo+","Stereo-" }, { "Saturate","Foldback" } };

struct Translator {
    const DecodedPatch& in;
    const EmberCalibration& cal;
    ImportReport report;
    std::map<String, double> out;                       // EMBER parameter id -> value
    juce::Array<juce::var> routes;
    std::map<std::string, double> values;
    std::map<std::string, bool> flagged;

    Translator(const DecodedPatch& d, const EmberCalibration& c) : in(d), cal(c) {
        for (const auto& f : d.fields) { values[f.name] = f.value; flagged[f.name] = f.outOfRange; }
        report.format = SourceFormat::SpireSpf2; report.target = TargetEngine::Ember;
    }
    double v(const std::string& name) const { auto it = values.find(name); return it == values.end() ? 0.0 : it->second; }
    int idx(const std::string& name) const { return spireEnumIndex(v(name)); }
    static double clamp(double x, double lo, double hi) { return std::max(lo, std::min(hi, x)); }
    const SynthA::Param* param(const String& id) const { const int i = SynthA::indexOf(id.toStdString()); return i < 0 ? nullptr : &SynthA::parameters()[size_t(i)]; }
    void set(const String& id, double value) { if (auto* p = param(id)) out[id] = p->limit(float(value)); }
    void setChoice(const String& id, const char* label) { if (auto* p = param(id)) for (size_t i = 0; i < p->choices.size(); ++i) if (p->choices[i] == label) { out[id] = double(i); return; } }
    void row(const String& parameter, const String& source, const String& native, RowStatus status, const String& reason = {}) {
        report.rows.push_back({ parameter.toStdString(), source.toStdString(), native.toStdString(), status, reason.toStdString() });
    }
    static String num(double x, int digits = 2) { return String(x, digits); }

    // ---------------------------------------------------------------------------------------------------------------------
    bool oscActive(int n) const { return v("mix_osc" + std::to_string(n)) > 0.001 && v("osc" + std::to_string(n) + "_mute") < 0.5; }

    void oscillators() {
        for (int n = 1; n <= 4; ++n) {
            const std::string p = "osc" + std::to_string(n) + "_";
            const bool active = oscActive(n);
            if (n == 4) { if (active) row("Oscillator 4", "mix " + num(v("mix_osc4")), "", RowStatus::Omitted, "EMBER has three oscillators; the fourth layer is not carried over"); continue; }
            const String o = "osc" + String(n) + ".";
            if (!active) { set(o + "level", 0); continue; }
            const int type = std::clamp(idx(p + "type"), 0, 6), wave = std::clamp(idx(p + "wave"), 0, 48);
            const double level = v("mix_osc" + std::to_string(n));
            if (type == 1) {   // noise oscillator
                set(o + "level", 0); set("mix.noise", std::max(out.count("mix.noise") ? out["mix.noise"] : 0.0, level));
                row("Oscillator " + String(n) + " type", "Noise", "EMBER noise mixer", RowStatus::Approximated, "Spire's noise oscillator is carried by EMBER's single noise source; its colour controls are not mapped");
                continue;
            }
            // wave
            if (type == 4) { setChoice(o + "wave", "Pulse"); set(o + "pw", 0.05 + 0.9 * v(p + "ctrlA")); row("Oscillator " + String(n) + " wave", "SawPWM", "Pulse", RowStatus::Approximated, "Spire's saw with pulse-width modulation is approximated by a pulse wave"); }
            else if (wave == 0) { setChoice(o + "wave", "Sine"); row("Oscillator " + String(n) + " wave", "Sin", "Sine", RowStatus::Mapped); }
            else if (wave == 1) { setChoice(o + "wave", "Triangle"); row("Oscillator " + String(n) + " wave", "Tri", "Triangle", RowStatus::Mapped); }
            else { setChoice(o + "wave", wave == 2 ? "Triangle" : "Saw"); row("Oscillator " + String(n) + " wave", kSpireWaves[wave], wave == 2 ? "Triangle" : "Saw", RowStatus::Approximated, "Spire's \"" + String(kSpireWaves[wave]) + "\" is a sampled or parametric waveform; the closest basic EMBER wave is used"); }
            if (type != 0 && type != 4) row("Oscillator " + String(n) + " type", kOscTypes[type], "plain wave", RowStatus::Approximated, String(kOscTypes[type]) + " mode (modulator, sync or vowel behaviour) is not available; the plain waveform is used");
            // level, pitch, unison
            set(o + "level", level); row("Oscillator " + String(n) + " level", num(level), num(level), RowStatus::Approximated, "level curve not calibrated; overall loudness is matched to the original");
            const double octave = std::round((v(p + "oct") - 0.5) * 8.0); set(o + "octave", clamp(octave, -3, 3));
            if (std::abs(octave) > 3) row("Oscillator " + String(n) + " octave", num(octave, 0), "limited to +-3", RowStatus::Approximated, "EMBER's octave range is three");
            const double semis = (v(p + "note") - 0.5) * 48.0, cents = (v(p + "fine") - 0.5) * 200.0;
            if (std::abs(semis) > 0.01 || std::abs(cents) > 0.01) { set(o + "coarse", std::round(semis)); set(o + "fine", cents); row("Oscillator " + String(n) + " tune", "note " + num(v(p + "note"), 3) + ", fine " + num(v(p + "fine"), 3), num(std::round(semis), 0) + " st, " + num(cents, 0) + " ct", RowStatus::Approximated, "the stored note and fine ranges are assumed (+-24 semitones, +-100 cents), not calibrated"); }
            const int voices = std::clamp(idx(p + "ucnt") + 1, 1, 9);
            set(o + "uni.voices", std::min(voices, 8)); set(o + "uni.detune", clamp(v(p + "udet"), 0, 1)); set(o + "uni.spread", clamp(v(p + "uwid"), 0, 1)); set(o + "uni.blend", 0.7);
            if (voices > 1) row("Oscillator " + String(n) + " unison", String(voices) + " voices, detune " + num(v(p + "udet")), String(std::min(voices, 8)) + " voices", voices > 8 ? RowStatus::Approximated : RowStatus::Approximated, voices > 8 ? "EMBER allows eight unison voices; detune and width scales are not calibrated" : "detune and width scales are not calibrated; Spire's density control has no equivalent");
        }
    }

    // ---------------------------------------------------------------------------------------------------------------------
    static const char* emberModelFor(int type) { return type == 1 ? "Ember 24" : type == 2 ? "Forge SVF" : type == 3 ? "Silk 12" : "Snarl 12"; }
    void filters() {
        const double offset = cal.cutoffShiftOctaves;
        for (int n = 1; n <= 2; ++n) {
            const std::string p = "flt" + std::to_string(n) + "_";
            const String f = "flt" + String(n) + ".";
            const int type = std::clamp(idx(p + "type"), 0, 6);
            if (type == 0) { set(f + "on", n == 1 ? 1.0 : 0.0); if (n == 1) { set(f + "cutoff", 20000); set(f + "res", 0); row("Filter 1", "off", "wide open", RowStatus::Mapped); } continue; }
            const auto& modes = kFilterModes[size_t(type)];
            const int mode = std::clamp(idx(p + "mode"), 0, int(modes.size()) - 1);
            const String modeName = modes[size_t(mode)];
            set(f + "on", 1);
            if (type == 5 || type == 6) {   // combo and shaper filters have no equivalent
                set(f + "on", 0); row("Filter " + String(n) + " type", kFilterTypes[type] + String(" / ") + modeName, "bypassed", RowStatus::Omitted, "EMBER has no comb or shaper filter; this filter is bypassed");
                continue;
            }
            setChoice(f + "model", emberModelFor(type));
            const char* response = modeName.startsWith("BP") || modeName.startsWith("BlackBP") ? "Band pass" : modeName.startsWith("HP") || modeName.startsWith("BlackHP") ? "High pass" : modeName == "Notch" ? "Notch" : "Low pass";
            setChoice(f + "mode", response);
            const double cutoff = 20.0 * std::pow(1000.0, clamp(v(p + "cut"), 0, 1)) * std::pow(2.0, offset);
            set(f + "cutoff", clamp(cutoff, 20, 20000)); set(f + "res", clamp(v(p + "res"), 0, 1));
            row("Filter " + String(n) + " type", String(kFilterTypes[type]) + " / " + modeName, String(emberModelFor(type)) + " / " + response, RowStatus::Approximated, modeName == "Peak" ? "Peak mode has no equivalent; a low pass is used" : "the closest EMBER filter model and slope; the sound of the two filters differs");
            row("Filter " + String(n) + " cutoff", num(v(p + "cut"), 3), num(clamp(cutoff, 20, 20000), 0) + " Hz", RowStatus::Approximated, "cutoff curve not calibrated (assumed 20 Hz to 20 kHz exponential); corrected against the original's render");
            set(f + "key", clamp((v("flt_keytrack") - 0.5) * 2.0, 0, 1));
        }
        if (v("flt_link") > 0.5) row("Filter link", "on", "", RowStatus::Omitted, "Spire's linked filter cutoffs have no switch in EMBER; both cutoffs are set independently");
        if (idx("flt_rout") != 0 && std::abs(v("flt_rout")) > 0.0005) row("Filter routing", "stored " + num(v("flt_rout"), 3), "serial", RowStatus::Approximated, "routing between the two filters is not mapped; EMBER's serial routing is used");
    }

    // ---------------------------------------------------------------------------------------------------------------------
    double seconds(double stored) const { return 0.001 * std::pow(10.0, 4.0 * clamp(stored, 0, 1)) * std::pow(2.0, cal.timeScaleOctaves); }
    void envelopes() {
        for (int n = 1; n <= 3; ++n) {
            const std::string p = "env" + std::to_string(n) + "_";
            const String e = "env" + String(n) + ".";
            set(e + "attack", seconds(v(p + "att"))); set(e + "decay", seconds(v(p + "dec"))); set(e + "sustain", clamp(v(p + "sus"), 0, 1)); set(e + "release", seconds(v(p + "rel")));
            if (n <= 2) row("Envelope " + String(n) + (n == 1 ? " (amplitude)" : " (filter)"), "A " + num(v(p + "att"), 3) + " D " + num(v(p + "dec"), 3) + " S " + num(v(p + "sus"), 2) + " R " + num(v(p + "rel"), 3), "A " + num(seconds(v(p + "att")), 3) + " s, D " + num(seconds(v(p + "dec")), 3) + " s, S " + num(v(p + "sus"), 2) + ", R " + num(seconds(v(p + "rel")), 3) + " s", RowStatus::Approximated, "time curve not calibrated (assumed 1 ms to 10 s exponential); times are corrected against the original's render; slope and curve stages are not mapped");
        }
        if (v("env4_att") > 0 || v("env4_dec") > 0 || v("env4_rel") > 0) { bool used = false; for (int t = 1; t <= 2; ++t) used = used || idx("env4_target" + std::to_string(t)) != 0; if (used) row("Envelope 4", "in use", "", RowStatus::Omitted, "EMBER has three envelopes"); }
    }

    // ---------------------------------------------------------------------------------------------------------------------
    void effects() {
        const struct { const char* prefix; const char* module; } list[] = { { "chr", "chorus" }, { "del", "delay" }, { "rev", "reverb" }, { "phs", "phaser" }, { "shp", "drive" } };
        for (const auto& fx : list) {
            const std::string p = std::string(fx.prefix) + "_";
            const bool muted = v(p + "mute") > 0.5; const double wet = v(p + "wet");
            if (muted || wet < 0.001) continue;
            const String m = String("fx.") + fx.module + ".";
            set(m + "on", 1); set(m + "mix", clamp(wet, 0, 1));
            if (std::string(fx.module) == "delay") { set("fx.delay.feedback", clamp((v("del_fback") - 0.5) * 2.0 * 0.8, 0, 0.88)); set("fx.delay.sync", v("del_sync") > 0.5 ? 1 : 0); }
            if (std::string(fx.module) == "reverb") { set("fx.reverb.size", clamp(0.3 + 0.7 * v("rev_decay"), 0, 1)); set("fx.reverb.damp", clamp(v("rev_damp"), 0, 1)); }
            if (std::string(fx.module) == "chorus") { set("fx.chorus.rate", 0.1 + 2.0 * v("chr_rate")); set("fx.chorus.depth", clamp(v("chr_depth"), 0, 1)); }
            if (std::string(fx.module) == "phaser") { set("fx.phaser.rate", 0.1 + 2.0 * v("phs_rate")); set("fx.phaser.depth", clamp(v("phs_depth"), 0, 1)); }
            if (std::string(fx.module) == "drive") { set("fx.drive.amount", clamp(v("shp_drive"), 0, 1)); set("fx.drive.mix", clamp(v("shp_wet"), 0, 1)); }
            row(String(fx.module) + " effect", "wet " + num(wet), String(fx.module) + " mix " + num(clamp(wet, 0, 1)), RowStatus::Approximated, "the effect's other settings are mapped by meaning only and are not calibrated");
        }
        if (v("eq_power") > 0.5) row("EQ", "on", "", RowStatus::Omitted, "Spire's master EQ is not mapped in this version");
    }

    // ---------------------------------------------------------------------------------------------------------------------
    void arpeggiator() {
        if (v("arp_on") < 0.5) return;
        static const char* spireModes[10] = { "Up","Down","Up/Down","Up/Down2","Down/Up","Down/Up2","Random","Ordered","Step","Chord" };
        static const char* emberModes[10] = { "Up","Down","Up-down","Up-down","Up-down","Up-down","Random","As played","Up","Chord" };
        static const bool exactModes[10] = { true,true,true,false,false,false,true,false,false,true };
        static const char* spireDivisions[21] = { "16/1","8/1","4/1","2/1","1/1","3/4","2/3","1/2","3/8","1/3","1/4","3/16","1/6","1/8","3/32","1/12","1/16","3/64","1/24","1/32","1/64" };
        static const char* emberDivisions[21] = { "2 bars","2 bars","2 bars","2 bars","1 bar","1 bar","1/2","1/2","1/4","1/4","1/4","1/8 D","1/8","1/8","1/8 T","1/8 T","1/16","1/16","1/16 T","1/32","1/32" };
        static const bool exactDivisions[21] = { false,false,false,true,true,false,false,true,false,false,true,true,false,true,false,true,true,false,true,true,false };
        const int mode = std::clamp(idx("arp_mode"), 0, 9), speed = std::clamp(idx("arp_speed"), 0, 20);
        set("arp.on", 1); setChoice("arp.mode", emberModes[mode]); setChoice("arp.rate", emberDivisions[speed]);
        set("arp.octaves", std::clamp(idx("arp_octave") + 1, 1, 4)); set("arp.gate", clamp(v("arp_length"), 0.05, 0.95)); set("arp.hold", 0);
        row("Arpeggiator mode", spireModes[mode], emberModes[mode], exactModes[mode] ? RowStatus::Mapped : RowStatus::Approximated, exactModes[mode] ? String() : "EMBER has no variant of this pattern; the nearest one is used");
        row("Arpeggiator rate", spireDivisions[speed], emberDivisions[speed], exactDivisions[speed] ? RowStatus::Mapped : RowStatus::Approximated, exactDivisions[speed] ? String() : "no tempo division of this length in EMBER; the nearest is used");
        row("Arpeggiator steps", "step pattern", "", RowStatus::Omitted, "Spire's per-step arpeggiator pattern is not mapped; the plain pattern plays");
    }
    // ---------------------------------------------------------------------------------------------------------------------
    // Modulation matrix and the direct envelope / LFO targets.
    static const char* emberSource(int sourceId) {
        switch (sourceId) { case 5: return "Env 1"; case 6: return "Env 2"; case 7: return "Env 3"; case 9: return "LFO 1"; case 10: return "LFO 2"; case 11: return "LFO 3"; case 13: return "Velocity"; case 14: return "Key"; case 15: case 30: return "Random"; case 17: return "Mod wheel"; case 18: return "Pressure"; case 26: return "Macro 1"; case 27: return "Macro 2"; case 28: return "Macro 3"; case 29: return "Macro 4"; default: return nullptr; }
    }
    static String sourceName(int id) { for (const auto& e : spire::kModulationSources) if (e.id == id) return e.name; return "source " + String(id); }
    static String targetName(int id) { for (const auto& e : spire::kModulationTargets) if (e.id == id) return e.name; return "target " + String(id); }
    static const char* emberTarget(int id) {
        switch (id) { case 45: return "flt1.cutoff"; case 46: return "flt1.res"; case 47: return "flt2.cutoff"; case 48: return "flt2.res"; case 1: return "osc1.level"; case 2: return "osc2.level"; case 3: return "osc3.level"; case 5: return "osc1.fine"; case 15: return "osc2.fine"; case 25: return "osc3.fine"; case 143: return "fx.reverb.mix"; case 137: return "fx.delay.mix"; case 129: return "fx.chorus.mix"; default: return nullptr; }
    }
    void addRoute(const char* source, const char* destination, double amount) {
        if (std::abs(amount) < 0.01) return;
        if (routes.size() >= 12) { row("Modulation", "extra route", "", RowStatus::Omitted, "EMBER has twelve modulation routes; the remaining routes were dropped"); return; }
        auto* r = new juce::DynamicObject; r->setProperty("src", source); r->setProperty("dst", destination); r->setProperty("amount", clamp(amount, -1, 1)); routes.add(juce::var(r));
    }
    void route(int sourceId, int targetId, double amountStored, const String& where) {
        if (targetId == 0 || sourceId == 0) return;
        const double depth = (amountStored - 0.5) * 2.0;
        if (std::abs(depth) < 0.01) return;
        const char* src = emberSource(sourceId); const char* dst = emberTarget(targetId);
        const String label = where + ": " + sourceName(sourceId) + " to " + targetName(targetId);
        if (src == nullptr) { row(label, "depth " + num(depth), "", RowStatus::Omitted, "the source " + sourceName(sourceId) + " has no EMBER equivalent"); return; }
        if (dst == nullptr) { row(label, "depth " + num(depth), "", RowStatus::Omitted, "EMBER has no matching destination for " + targetName(targetId)); return; }
        // Envelope 2 into a filter cutoff is EMBER's own filter-envelope amount rather than a matrix route.
        if (std::string(src) == "Env 2" && (std::string(dst) == "flt1.cutoff" || std::string(dst) == "flt2.cutoff")) { set(String(dst).replace("cutoff", "env"), clamp(depth, -1, 1)); row(label, "depth " + num(depth), String(dst).replace("cutoff", "env") + " " + num(depth), RowStatus::Approximated, "depth scale not calibrated"); return; }
        addRoute(src, dst, depth * 0.5); row(label, "depth " + num(depth), "route " + String(src) + " to " + dst + " " + num(depth * 0.5), RowStatus::Approximated, "depth scale not calibrated");
    }
    void modulation() {
        for (int e = 1; e <= 4; ++e) for (int t = 1; t <= 2; ++t) route(4 + e, idx("env" + std::to_string(e) + "_target" + std::to_string(t)), v("env" + std::to_string(e) + "_amt" + std::to_string(t)), "Envelope " + String(e));
        for (int l = 1; l <= 4; ++l) for (int t = 1; t <= 2; ++t) route(8 + l, idx("lfo" + std::to_string(l) + "_target" + std::to_string(t)), v("lfo" + std::to_string(l) + "_amt" + std::to_string(t)), "LFO " + String(l));
        for (int s = 1; s <= 15; ++s) {
            const std::string p = "slot" + std::to_string(s) + "_";
            const int s1 = idx(p + "Src1"), s2 = idx(p + "Src2");
            for (int t = 1; t <= 4; ++t) { const int target = idx(p + "Trg" + std::to_string(t)); if (target == 0) continue; const int src = s1 != 0 ? s1 : s2; route(src, target, v(p + "Amt" + std::to_string(t)), "Matrix slot " + String(s)); if (s1 != 0 && s2 != 0) row("Matrix slot " + String(s), "second source " + sourceName(s2), "", RowStatus::Omitted, "a second source multiplies the first in Spire; EMBER routes have one source"); }
        }
        // LFO shapes for the first three LFOs
        for (int l = 1; l <= 3; ++l) {
            const std::string p = "lfo" + std::to_string(l) + "_"; const int form = std::clamp(idx(p + "form"), 0, 51);
            const char* shape = form == 0 ? "Sine" : form == 1 ? "Triangle" : form == 2 ? "Square" : form == 3 ? "Hold" : form == 4 ? "Smooth random" : "Sine";
            setChoice("lfo" + String(l) + ".shape", shape); set("lfo" + String(l) + ".rate", 0.01 * std::pow(3000.0, clamp(v(p + "rate"), 0, 1)));
            set("lfo" + String(l) + ".sync", v(p + "sync") > 0.5 ? 1 : 0);
        }
    }

    // ---------------------------------------------------------------------------------------------------------------------
    void globals() {
        const int poly = idx("poly");
        set("voice.mode", poly == 0 ? 1 : 0);   // Spire mode list not fully known: poly 0 treated as mono, everything else polyphonic
        set("voice.glide", clamp(v("glide"), 0, 1) * 0.5);
        if (v("glide") > 0.001) row("Glide", num(v("glide"), 3), num(clamp(v("glide"), 0, 1) * 0.5, 2) + " s", RowStatus::Approximated, "glide time curve not calibrated");
        set("master.volume", cal.masterVolumeDb);
        row("Master volume", num(v("volume")), num(cal.masterVolumeDb, 1) + " dB", RowStatus::Approximated, "set so that the loudness matches the original's render");
        if (v("x-comp") > 0.001) row("X-Comp", num(v("x-comp")), "", RowStatus::Omitted, "Spire's master compressor is not mapped");
    }

    EmberTranslation run() {
        EmberTranslation result;
        bool sounding = false; for (int n = 1; n <= 3; ++n) sounding = sounding || oscActive(n) || false;
        const bool noiseOnly = !sounding;
        oscillators(); filters(); envelopes(); effects(); arpeggiator(); modulation(); globals();
        sounding = false; for (int n = 1; n <= 3; ++n) sounding = sounding || (out.count("osc" + String(n) + ".level") && out["osc" + String(n) + ".level"] > 0.001);
        sounding = sounding || (out.count("mix.noise") && out["mix.noise"] > 0.001);
        (void) noiseOnly;
        report.blocker.clear();
        if (!sounding) { report.result = FileResult::Blocked; report.blocker = "No oscillator among the first three is sounding, so EMBER would have no voice (the fourth oscillator alone cannot be carried over)."; report.headline = "Blocked: " + report.blocker; result.report = report; return result; }
        const int approx = report.approximated(), omit = report.omitted();
        report.result = (approx == 0 && omit == 0) ? FileResult::Opened : FileResult::Approximation;
        report.headline = String(report.mapped()).toStdString() + " settings mapped, " + String(approx).toStdString() + " approximated, " + String(omit).toStdString() + " omitted";
        auto* patch = new juce::DynamicObject; patch->setProperty("format", "dualsynth-patch"); patch->setProperty("version", 1); patch->setProperty("engine", "A");
        patch->setProperty("name", String(in.name)); patch->setProperty("category", "Spire import");
        auto* params = new juce::DynamicObject; for (auto& e : out) params->setProperty(juce::Identifier(e.first), e.second); patch->setProperty("params", juce::var(params)); patch->setProperty("mod", routes);
        result.patch = juce::var(patch); result.report = report; return result;
    }
};

} // namespace

EmberTranslation translateSpireToEmber(const DecodedPatch& decoded, const EmberCalibration& calibration) { return Translator(decoded, calibration).run(); }

} // namespace aether::import
