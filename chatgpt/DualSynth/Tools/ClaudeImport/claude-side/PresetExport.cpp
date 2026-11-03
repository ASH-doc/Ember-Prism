// Exports every EMBER and PRISM factory preset of the Claude engines (full parameter sets, real units), the harmonic
// spectra of the 30 PRISM wavetables, and objective sound features of each preset (note C3, velocity 0.8, 1 s hold),
// as one JSON file for the ChatGPT-side importer.   Usage: DuoPresetExport <output.json>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "SynthA/SA_Engine.h"
#include "SynthA/SA_Presets.h"
#include "SynthB/SB_Engine.h"
#include "SynthB/SB_Presets.h"
#include "SynthB/SB_Wavetables.h"
#include "MatchFeatures.h"
#include <cstdio>

static constexpr double kRate = 48000.0, kHold = 1.0, kTotal = 2.0;

template <class Eng>
static match::Features renderFeatures (Eng& e, const std::vector<float>& params)
{
    const int block = 256, total = (int) (kTotal * kRate);
    e.prepare (kRate, block);
    e.setParamSnapshot (params.data());
    std::vector<float> L ((size_t) total), R ((size_t) total), mono ((size_t) total);
    duo::TransportInfo t; t.bpm = 138.0;
    bool on = false, off = false;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        if (! on) { e.noteOn (60, 0.8f, 1); on = true; }
        if (! off && (double) pos >= kHold * kRate) { e.noteOff (60, 1); off = true; }
        e.process (L.data() + pos, R.data() + pos, n, t);
        t.ppq += n * t.bpm / 60.0 / kRate;
    }
    for (int i = 0; i < total; ++i) mono[(size_t) i] = 0.5f * (L[(size_t) i] + R[(size_t) i]);
    return match::analyse (mono, kRate, kHold);
}

static juce::var featuresToVar (const match::Features& f)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("rmsDb", f.rmsDb); o->setProperty ("peakDb", f.peakDb); o->setProperty ("attackMs", f.attackMs); o->setProperty ("releaseDb", f.releaseDb);
    juce::Array<juce::var> bands; for (float b : f.band) bands.add (b);
    o->setProperty ("band", bands);
    return juce::var (o);
}

template <class Defs>
static juce::var paramsToVar (const Defs* defs, int count, const std::vector<float>& values)
{
    auto* o = new juce::DynamicObject();
    for (int i = 0; i < count; ++i) o->setProperty (juce::Identifier (defs[i].id), (double) values[(size_t) i]);
    return juce::var (o);
}

// harmonic magnitudes (dB below the strongest) of one PRISM table at nine positions, from the mip-0 frames
static juce::var tableSpectra (int index)
{
    const auto& wt = sb::wavetable (index);
    juce::Array<juce::var> positions;
    for (int p = 0; p < 9; ++p)
    {
        const double fp = (double) p / 8.0 * (sb::kFrames - 1); int f0 = std::min ((int) fp, sb::kFrames - 2); const double ff = fp - f0;
        std::vector<std::complex<double>> a ((size_t) sb::kBaseN);
        for (int i = 0; i < sb::kBaseN; ++i) a[(size_t) i] = (1.0 - ff) * wt.mip[0][(size_t) (f0 * sb::kBaseN + i)] + ff * wt.mip[0][(size_t) ((f0 + 1) * sb::kBaseN + i)];
        match::fft (a);
        double mx = 1e-30; for (int h = 1; h <= 48; ++h) mx = std::max (mx, std::abs (a[(size_t) h]));
        juce::Array<juce::var> hs; for (int h = 1; h <= 48; ++h) hs.add ((double) match::dbOf (std::abs (a[(size_t) h]) / mx));
        positions.add (hs);
    }
    return juce::var (positions);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 2) { std::fprintf (stderr, "usage: DuoPresetExport <output.json>\n"); return 2; }
    sb::generateWavetables();
    auto* root = new juce::DynamicObject();
    root->setProperty ("format", "claude-export"); root->setProperty ("version", 1);
    root->setProperty ("note", "C3 velocity 0.8, 48 kHz, 1.0 s hold, 2.0 s render, mono mix");

    {   // EMBER
        auto* section = new juce::DynamicObject(); juce::Array<juce::var> list;
        const auto& presets = sa::presets();
        for (size_t i = 0; i < presets.size(); ++i)
        {
            const auto values = sa::presetValues ((int) i);
            auto* p = new juce::DynamicObject();
            p->setProperty ("index", (int) i); p->setProperty ("name", juce::String (presets[i].name)); p->setProperty ("category", juce::String (presets[i].category));
            p->setProperty ("params", paramsToVar (sa::defs(), sa::COUNT, values));
            sa::Engine e; p->setProperty ("features", featuresToVar (renderFeatures (e, values)));
            list.add (juce::var (p));
            std::printf ("EMBER %zu/%zu %s\n", i + 1, presets.size(), presets[i].name.c_str());
        }
        section->setProperty ("presets", list);
        juce::Array<juce::var> choices; for (int i = 0; i < sa::COUNT; ++i) choices.add (juce::String (sa::defs()[i].choices));
        section->setProperty ("choices", choices);
        root->setProperty ("ember", juce::var (section));
    }
    {   // PRISM
        auto* section = new juce::DynamicObject(); juce::Array<juce::var> list;
        const auto& presets = sb::presets();
        for (size_t i = 0; i < presets.size(); ++i)
        {
            const auto values = sb::presetValues ((int) i);
            auto* p = new juce::DynamicObject();
            p->setProperty ("index", (int) i); p->setProperty ("name", juce::String (presets[i].name)); p->setProperty ("category", juce::String (presets[i].category));
            p->setProperty ("params", paramsToVar (sb::defs(), sb::COUNT, values));
            juce::Array<juce::var> shapes;
            for (const auto& s : presets[i].shapes) { auto* so = new juce::DynamicObject(); so->setProperty ("lfo", s.first); juce::Array<juce::var> pts; for (float v : s.second) pts.add (v); so->setProperty ("points", pts); shapes.add (juce::var (so)); }
            p->setProperty ("shapes", shapes);
            sb::Engine e;
            for (const auto& s : presets[i].shapes) for (size_t k = 0; k < s.second.size() && k < 16; ++k) e.setCustomShape (s.first, (int) k, s.second[k]);
            p->setProperty ("features", featuresToVar (renderFeatures (e, values)));
            list.add (juce::var (p));
            std::printf ("PRISM %zu/%zu %s\n", i + 1, presets.size(), presets[i].name.c_str());
        }
        section->setProperty ("presets", list);
        juce::Array<juce::var> choices; for (int i = 0; i < sb::COUNT; ++i) choices.add (juce::String (sb::defs()[i].choices));
        section->setProperty ("choices", choices);
        juce::Array<juce::var> tables;
        for (int t = 0; t < sb::kTables; ++t) { auto* o = new juce::DynamicObject(); o->setProperty ("index", t); o->setProperty ("name", juce::String (sb::wavetableName (t))); o->setProperty ("spectra", tableSpectra (t)); tables.add (juce::var (o)); }
        section->setProperty ("tables", tables);
        root->setProperty ("prism", juce::var (section));
    }
    if (argc >= 3)   // the 30 wavetables themselves (32 frames x 2048 samples, float32 little-endian, standard base64) for exact reuse
    {
        juce::Array<juce::var> tables;
        for (int t = 0; t < sb::kTables; ++t)
        {
            const auto& wt = sb::wavetable (t);
            juce::MemoryOutputStream encoded;
            juce::Base64::convertToBase64 (encoded, wt.mip[0].data(), (size_t) (sb::kFrames * sb::kBaseN) * sizeof (float));
            auto* o = new juce::DynamicObject();
            o->setProperty ("name", juce::String (sb::wavetableName (t))); o->setProperty ("frames", sb::kFrames);
            o->setProperty ("encoding", "float32-le-base64"); o->setProperty ("data", encoded.toString());
            tables.add (juce::var (o));
        }
        const auto tablesFile = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        if (! tablesFile.replaceWithText (juce::JSON::toString (juce::var (tables), false))) { std::fprintf (stderr, "could not write %s\n", tablesFile.getFullPathName().toRawUTF8()); return 1; }
        std::printf ("written %s (%lld bytes)\n", tablesFile.getFullPathName().toRawUTF8(), (long long) tablesFile.getSize());
    }
    const auto out = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
    if (! out.replaceWithText (juce::JSON::toString (juce::var (root), false))) { std::fprintf (stderr, "could not write %s\n", out.getFullPathName().toRawUTF8()); return 1; }
    std::printf ("written %s (%lld bytes)\n", out.getFullPathName().toRawUTF8(), (long long) out.getSize());
    return 0;
}
