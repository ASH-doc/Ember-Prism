// Offline self-test: renders both synths (alone and through the host) with OfflineAudio-style loops and checks the results.
//   DuoSelfTest.exe [outDir]      -> prints PASS/FAIL lines, writes demo WAVs into outDir, exit code 0 = all passed
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <cstdio>
#include <chrono>
#include "SynthA/SA_Engine.h"
#include "SynthA/SA_Presets.h"
#include "SynthB/SB_Engine.h"
#include "SynthB/SB_Presets.h"
#include "Host/HostEngine.h"
#include <functional>

static int failures = 0;
static juce::AudioBuffer<float> lastRender;
static void check (bool ok, const juce::String& what)
{
    std::printf ("%s  %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
    if (! ok) ++failures;
}

struct Stats { float peak = 0, rms = 0; bool bad = false; };
static Stats analyse (const juce::AudioBuffer<float>& b, int from = 0)
{
    Stats s; double sum = 0; int cnt = 0;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = from; i < b.getNumSamples(); ++i)
        {
            const float x = b.getSample (c, i);
            if (! std::isfinite (x)) s.bad = true;
            s.peak = std::max (s.peak, std::fabs (x)); sum += (double) x * x; ++cnt;
        }
    s.rms = cnt ? (float) std::sqrt (sum / cnt) : 0.0f;
    return s;
}

static void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::FileOutputStream> (f.createOutputStream()))
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0)))
        {
            os.release();
            w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
        }
}

// magnitude spectrum (Hann window, 32768-point) of channel 0 between t0 and t1 seconds
static std::vector<float> spectrum (const juce::AudioBuffer<float>& b, double sr, double t0, double /*t1*/)
{
    const int order = 15, N = 1 << order;
    juce::dsp::FFT fft (order);
    std::vector<float> data ((size_t) N * 2, 0.0f);
    const int start = (int) (t0 * sr);
    for (int i = 0; i < N; ++i)
    {
        const int idx = start + i;
        const float w = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) N);
        data[(size_t) i] = idx < b.getNumSamples() ? (b.getSample (0, idx) + b.getSample (1, idx)) * 0.5f * w : 0.0f;
    }
    fft.performFrequencyOnlyForwardTransform (data.data());
    data.resize ((size_t) N / 2);
    return data;
}
static float peakFrequency (const juce::AudioBuffer<float>& b, double sr, double t0, double t1)
{
    auto sp = spectrum (b, sr, t0, t1);
    size_t best = 1; for (size_t i = 2; i < sp.size(); ++i) if (sp[i] > sp[best]) best = i;
    return (float) ((double) best * sr / 32768.0);
}
// level of everything that is not within +-3 bins of a multiple of the fundamental (below 20 kHz), relative to the fundamental peak
static float inharmonicDb (const juce::AudioBuffer<float>& b, double sr, double t0, double t1, float f0)
{
    auto sp = spectrum (b, sr, t0, t1);
    const double binHz = sr / 32768.0;
    float fund = 0; for (size_t i = 1; i < sp.size(); ++i) if (std::fabs ((double) i * binHz - (double) f0) < 40.0) fund = std::max (fund, sp[i]);
    double junk = 0;
    for (size_t i = 3; i < sp.size() && (double) i * binHz < 20000.0; ++i)
    {
        const double f = (double) i * binHz, h = std::round (f / f0), d = std::fabs (f - h * f0);
        if (d > 3.0 * binHz + 5.0) junk = std::max (junk, (double) sp[i]);
    }
    return 20.0f * std::log10 ((float) junk / std::max (fund, 1e-9f));
}

template <class Eng>
static juce::AudioBuffer<float> renderChord (Eng& e, const std::vector<float>& params, double sr, double seconds, const std::vector<int>& notes, double holdSec, int block = 256)
{
    e.prepare (sr, block);
    e.setParamSnapshot (params.data());
    const int total = (int) (seconds * sr);
    juce::AudioBuffer<float> out (2, total);
    duo::TransportInfo t; t.bpm = 138.0;
    bool on = false, off = false;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        if (! on) { for (int k : notes) e.noteOn (k, 0.9f, 1); on = true; }
        if (! off && pos >= holdSec * sr) { for (int k : notes) e.noteOff (k, 1); off = true; }
        e.process (out.getWritePointer (0) + pos, out.getWritePointer (1) + pos, n, t);
        t.ppq += n * t.bpm / 60.0 / sr;
    }
    return out;
}

int main (int argc, char** argv)
{
    const juce::File outDir = argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]) : juce::File::getCurrentWorkingDirectory().getChildFile ("test-output");
    outDir.createDirectory();
    const double sr = 48000.0;
    const std::vector<int> chord { 48, 55, 60, 64, 67, 72 };
    {
        sb::generateWavetables();
        for (int tb : { 0, 12, 22 }) for (int m : { 0, 3, 7 })
        {
            const auto& wt = sb::wavetable (tb); const int N = sb::kBaseN >> m; double sq = 0; float pk = 0;
            for (int i = 0; i < N; ++i) { const float x = wt.mip[m][(size_t) (5 * N + i)]; sq += x * x; pk = std::max (pk, std::fabs (x)); }
            std::printf ("DEBUG table %d mip %d frame5: rms %.3f peak %.3f\n", tb, m, std::sqrt (sq / N), pk);
        }
    }

    // ------------------------------------------------------------------ EMBER (Synth A)
    {
        sa::Engine a;
        const auto init = sa::presetValues (0);
        auto b = renderChord (a, init, sr, 3.0, chord, 2.0);
        auto st = analyse (b);
        check (! st.bad, "EMBER init: no NaN / Inf");
        check (st.rms > 0.01f, "EMBER init: not silent (rms " + juce::String (st.rms, 4) + ")");
        check (st.peak < 1.0f, "EMBER init: peak below 1.0 (" + juce::String (st.peak, 3) + ")");
        writeWav (outDir.getChildFile ("ember-init-chord.wav"), b, sr);

        // brutal settings: max resonance, drive, every filter model must stay finite and bounded
        for (int model = 0; model < 4; ++model)
        {
            auto p = init;
            p[sa::f1_model] = (float) model; p[sa::f1_res] = 1.0f; p[sa::f1_drive] = 1.0f; p[sa::f1_cut] = 400.0f; p[sa::f1_env] = 1.0f;
            p[sa::uni_voices] = 8; p[sa::o1_wave] = 4;
            sa::Engine e;
            auto bb = renderChord (e, p, sr, 2.0, chord, 1.5);
            auto s2 = analyse (bb);
            check (! s2.bad && s2.peak < 2.0f, "EMBER filter model " + juce::String (model) + " extreme settings finite (peak " + juce::String (s2.peak, 2) + ")");
        }

        // tuning: A4 must come out at 440 Hz (single saw, no unison, no drift)
        {
            auto p = init; p[sa::age] = 0; p[sa::o2_lvl] = 0; p[sa::sub_lvl] = 0; p[sa::o1_lvl] = 1; p[sa::f1_cut] = 20000; p[sa::f1_env] = 0; p[sa::f1_key] = 0; p[sa::f1_model] = 1;
            sa::Engine e;
            auto bb = renderChord (e, p, sr, 1.0, { 69 }, 0.9);
            check (std::fabs (peakFrequency (bb, sr, 0.2, 0.8) - 440.0f) < 2.0f, "EMBER tuning A4 = 440 Hz (measured " + juce::String (peakFrequency (bb, sr, 0.2, 0.8), 1) + ")");
        }
        // aliasing: band-limited saws on very high notes must not leave strong non-harmonic junk
        for (int note : { 96, 108 })
        {
            auto p = init; p[sa::age] = 0; p[sa::o2_lvl] = 0; p[sa::sub_lvl] = 0; p[sa::o1_lvl] = 1; p[sa::f1_cut] = 20000; p[sa::f1_env] = 0; p[sa::f1_key] = 0; p[sa::f1_model] = 1; p[sa::warmth] = 0; p[sa::f1_drive] = 0;
            sa::Engine e;
            auto bb = renderChord (e, p, sr, 1.0, { note }, 0.9);
            const float f0 = 440.0f * std::pow (2.0f, (note - 69) / 12.0f);
            const float db = inharmonicDb (bb, sr, 0.2, 0.8, f0);
            const float limit = note == 96 ? -32.0f : -25.0f;
            check (db < limit, "EMBER saw note " + juce::String (note) + " alias junk " + juce::String (db, 1) + " dB re fundamental (needs < " + juce::String (limit, 0) + ")");
        }
        // self-oscillating Moog ladder: pure tone near the keyboard-tracked pitch with no oscillator input
        {
            auto p = init; p[sa::o1_lvl] = 0; p[sa::o2_lvl] = 0; p[sa::sub_lvl] = 0; p[sa::age] = 0; p[sa::f1_model] = 0; p[sa::f1_res] = 1.0f; p[sa::f1_cut] = 440.0f; p[sa::f1_key] = 1.0f; p[sa::f1_env] = 0;
            p[sa::noise_lvl] = 0.02f; p[sa::e1_s] = 1; p[sa::f1_drive] = 0; p[sa::f1_vel] = 0;
            sa::Engine e;
            auto bb = renderChord (e, p, sr, 1.5, { 60 }, 1.4);
            const float hz = peakFrequency (bb, sr, 0.6, 1.3);
            check (hz > 420.0f && hz < 460.0f && analyse (bb, (int) (0.6 * sr)).rms > 0.001f, "EMBER ladder self-oscillates at " + juce::String (hz, 1) + " Hz (cutoff 440 @ C4 key-tracked)");
        }
        // ---- "Moog warmth" measurements: ladder slope and even-harmonic saturation
        {
            auto levelAt = [&] (const juce::AudioBuffer<float>& b, float hz)
            {
                auto sp = spectrum (b, sr, 0.3, 0.9); const int c = (int) std::lround ((double) hz * 32768.0 / sr); float m = 1e-9f;
                for (int i = std::max (1, c - 3); i <= std::min ((int) sp.size() - 1, c + 3); ++i) m = std::max (m, sp[(size_t) i]);
                return juce::Decibels::gainToDecibels (m);
            };
            auto base = init; base[sa::age] = 0; base[sa::o2_lvl] = 0; base[sa::sub_lvl] = 0; base[sa::o1_lvl] = 1; base[sa::f1_env] = 0; base[sa::f1_vel] = 0; base[sa::f1_key] = 0; base[sa::f1_res] = 0; base[sa::f1_drive] = 0; base[sa::warmth] = 0; base[sa::o1_oct] = -1;
            const float f0 = 440.0f * std::pow (2.0f, (36 - 69) / 12.0f);     // C2 = 65.4 Hz
            for (int model : { 0, 1 })
            {
                auto p2 = base; p2[sa::f1_model] = (float) model; p2[sa::f1_cut] = 1000;
                sa::Engine e; auto bb = renderChord (e, p2, sr, 1.2, { 48 }, 1.1);   // note 48 with o1_oct -1 = C2
                const float h1 = levelAt (bb, std::round (4000.0f / f0) * f0), h2 = levelAt (bb, std::round (8000.0f / f0) * f0);
                const float slope = h1 - h2;      // dB drop per octave above the cutoff, including the saw's own 6 dB/oct
                std::printf ("INFO  EMBER %s: %.1f dB per octave above cutoff (saw 6 dB + filter)\n", model == 0 ? "Moog ladder" : "Multimode 12", slope);
                check (model == 0 ? (slope > 26.0f && slope < 36.0f) : (slope > 14.0f && slope < 22.0f), juce::String (model == 0 ? "EMBER Moog ladder is a true 24 dB/oct filter" : "EMBER Multimode is a 12 dB/oct filter") + " (" + juce::String (slope, 1) + " dB/oct total)");
            }
            // even harmonics from the warmth stage: a pure sine in, count the 2nd harmonic out
            auto sine = base; sine[sa::o1_wave] = 3; sine[sa::f1_cut] = 20000; sine[sa::f1_model] = 1; sine[sa::o1_oct] = 0;
            float h2cold = 0, h2warm = 0;
            for (float w : { 0.0f, 1.0f })
            {
                auto p2 = sine; p2[sa::warmth] = w; sa::Engine e; auto bb = renderChord (e, p2, sr, 1.2, { 57 }, 1.1);    // A3 = 220 Hz
                const float fund = levelAt (bb, 220.0f), second = levelAt (bb, 440.0f);
                (w == 0.0f ? h2cold : h2warm) = second - fund;
            }
            std::printf ("INFO  EMBER warmth: 2nd harmonic %.1f dB (warmth 0) -> %.1f dB (warmth 1) relative to the fundamental\n", h2cold, h2warm);
            check (h2warm > h2cold + 15.0f && h2warm > -30.0f, "EMBER warmth adds even harmonics (2nd harmonic rises by " + juce::String (h2warm - h2cold, 1) + " dB)");
        }    }

    // ------------------------------------------------------------------ PRISM (Synth B)
    {
        sb::generateWavetables();
        std::vector<float> init (sb::COUNT);
        for (int i = 0; i < sb::COUNT; ++i) init[(size_t) i] = sb::defs()[i].def;
        {
            sb::Engine e;
            auto b = renderChord (e, init, sr, 3.0, chord, 2.0);
            auto st = analyse (b);
            check (! st.bad, "PRISM init: no NaN / Inf");
            check (st.rms > 0.01f, "PRISM init: not silent (rms " + juce::String (st.rms, 4) + ")");
            check (st.peak < 1.0f, "PRISM init: peak below 1.0 (" + juce::String (st.peak, 3) + ")");
            writeWav (outDir.getChildFile ("prism-init-chord.wav"), b, sr);
        }
        // every wavetable plays across the keyboard without NaN and with sane level
        {
            bool allOk = true; float worst = 0;
            for (int t = 0; t < sb::kTables; ++t)
                for (float pos : { 0.0f, 0.37f, 1.0f })
                {
                    auto p = init; p[sb::o1_table] = (float) t; p[sb::o1_pos] = pos; p[sb::f1_cut] = 20000;
                    sb::Engine e;
                    auto bb = renderChord (e, p, sr, 0.5, { 36, 72, 96 }, 0.4);
                    auto s2 = analyse (bb);
                    worst = std::max (worst, s2.peak);
                    if (s2.bad || s2.rms < 0.003f || s2.peak > 1.5f) { allOk = false; std::printf ("   table %d pos %.2f -> rms %.4f peak %.3f bad %d\n", t, pos, s2.rms, s2.peak, (int) s2.bad); }
                }
            check (allOk, "PRISM all 30 wavetables x 3 positions: finite, audible, bounded (worst peak " + juce::String (worst, 2) + ")");
        }
        // every warp mode and every filter type at extreme settings
        {
            bool ok = true; float worst = 0;
            for (int w = 0; w <= 10; ++w)
            {
                auto p = init; p[sb::o1_w1] = (float) w; p[sb::o1_w1a] = 1; p[sb::o1_w2] = (float) ((w + 3) % 11); p[sb::o1_w2a] = 0.8f;
                p[sb::o2_lvl] = 0.6f; p[sb::o2_w1] = (float) w; p[sb::o2_w1a] = 1; p[sb::uni_voices] = 16; p[sb::uni_detune] = 1;
                sb::Engine e; auto bb = renderChord (e, p, sr, 0.8, chord, 0.6); auto s2 = analyse (bb);
                worst = std::max (worst, s2.peak); if (s2.bad || s2.peak > 3.0f) ok = false;
            }
            check (ok, "PRISM all 11 warp modes at max with 16-voice unison: finite (worst peak " + juce::String (worst, 2) + ")");
            ok = true; worst = 0;
            for (int f = 0; f < 8; ++f)
            {
                auto p = init; p[sb::f1_type] = (float) f; p[sb::f1_res] = 1; p[sb::f1_cut] = 300; p[sb::f1_drive] = 1; p[sb::f2_type] = (float) ((f + 1) % 8); p[sb::f2_res] = 1; p[sb::f2_cut] = 2000;
                sb::Engine e; auto bb = renderChord (e, p, sr, 0.8, chord, 0.6); auto s2 = analyse (bb);
                worst = std::max (worst, s2.peak); if (s2.bad || s2.peak > 3.0f) { ok = false; std::printf ("   filter type %d peak %.2f\n", f, s2.peak); }
            }
            check (ok, "PRISM all 8 filter types at max resonance + drive: finite (worst peak " + juce::String (worst, 2) + ")");
        }
        // every FX rack module on its own
        {
            bool ok = true; float worst = 0;
            for (int t = 1; t <= 10; ++t)
            {
                auto p = init; p[sb::r1_type] = (float) t; p[sb::r1_mix] = 1; p[sb::r1_a] = 0.7f; p[sb::r1_b] = 0.8f; p[sb::r1_c] = 0.6f;
                sb::Engine e; auto bb = renderChord (e, p, sr, 1.5, chord, 0.8); auto s2 = analyse (bb);
                worst = std::max (worst, s2.peak); if (s2.bad || s2.peak > 3.0f || s2.rms < 0.0005f) { ok = false; std::printf ("   fx %d rms %.5f peak %.3f\n", t, s2.rms, s2.peak); }
            }
            check (ok, "PRISM 10 rack modules: finite and audible (worst peak " + juce::String (worst, 2) + ")");
        }
        // tuning + aliasing with the pure saw table
        {
            auto p = init; p[sb::o1_table] = 0; p[sb::o1_pos] = 0; p[sb::f1_cut] = 20000; p[sb::f1_type] = 0; p[sb::f1_env] = 0; p[sb::f1_key] = 0; p[sb::f1_vel] = 0;
            { sb::Engine e; auto bb = renderChord (e, p, sr, 1.0, { 69 }, 0.9);
              check (std::fabs (peakFrequency (bb, sr, 0.2, 0.8) - 440.0f) < 2.0f, "PRISM tuning A4 = 440 Hz (measured " + juce::String (peakFrequency (bb, sr, 0.2, 0.8), 1) + ")"); }
            for (int note : { 96, 108 })
            {
                sb::Engine e; auto bb = renderChord (e, p, sr, 1.0, { note }, 0.9);
                const float f0 = 440.0f * std::pow (2.0f, (note - 69) / 12.0f);
                const float db = inharmonicDb (bb, sr, 0.2, 0.8, f0);
                check (db < -50.0f, "PRISM wavetable saw note " + juce::String (note) + " alias junk " + juce::String (db, 1) + " dB re fundamental (needs < -50)");
            }
        }
    }

    // ------------------------------------------------------------------ every preset of both synths
    {
        sb::generateWavetables();
        int bad = 0; float worstPeak = 0; juce::String listing;
        for (int i = 0; i < (int) sa::presets().size(); ++i)
        {
            sa::Engine e; auto b = renderChord (e, sa::presetValues (i), sr, 3.5, chord, 2.5); auto s = analyse (b);
            worstPeak = std::max (worstPeak, s.peak);
            if (s.peak > 0.75f) std::printf ("INFO  hot EMBER preset %d '%s' peak %.2f\n", i, sa::presets()[(size_t) i].name.c_str(), s.peak);
            if (s.bad || s.rms < 0.002f || s.peak > 1.5f) { ++bad; std::printf ("   EMBER preset %d '%s': rms %.4f peak %.3f bad %d\n", i, sa::presets()[(size_t) i].name.c_str(), s.rms, s.peak, (int) s.bad); }
            if (i == 1 || i == 7 || i == 12 || i == 17 || i == 22) writeWav (outDir.getChildFile ("ember-" + juce::String (i) + "-" + juce::String (sa::presets()[(size_t) i].name).retainCharacters ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") + ".wav"), b, sr);
        }
        check (bad == 0, "EMBER all " + juce::String ((int) sa::presets().size()) + " presets: finite, audible, bounded (worst peak " + juce::String (worstPeak, 2) + ")");
        bad = 0; worstPeak = 0;
        for (int i = 0; i < (int) sb::presets().size(); ++i)
        {
            auto ep = std::make_unique<sb::Engine>(); auto b = renderChord (*ep, sb::presetValues (i), sr, 3.5, chord, 2.5); auto s = analyse (b);
            worstPeak = std::max (worstPeak, s.peak);
            if (s.bad || s.rms < 0.002f || s.peak > 1.5f) { ++bad; std::printf ("   PRISM preset %d '%s': rms %.4f peak %.3f bad %d\n", i, sb::presets()[(size_t) i].name.c_str(), s.rms, s.peak, (int) s.bad); }
            if (i == 1 || i == 5 || i == 9 || i == 14 || i == 20) writeWav (outDir.getChildFile ("prism-" + juce::String (i) + "-" + juce::String (sb::presets()[(size_t) i].name).retainCharacters ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") + ".wav"), b, sr);
        }
        check (bad == 0, "PRISM all " + juce::String ((int) sb::presets().size()) + " presets: finite, audible, bounded (worst peak " + juce::String (worstPeak, 2) + ")");
    }
    // ------------------------------------------------------------------ the host: routing, mixer, master
    {
        sb::generateWavetables();
        auto A = std::make_unique<sa::Engine>(); auto B = std::make_unique<sb::Engine>();
        duo::HostEngine host;
        std::vector<float> pa = sa::presetValues (0), pb (sb::COUNT);
        for (int i = 0; i < sb::COUNT; ++i) pb[(size_t) i] = sb::defs()[i].def;
        host.prepare (sr, 512, A.get(), B.get());
        A->setParamSnapshot (pa.data()); B->setParamSnapshot (pb.data());
        duo::HostParams hp;
        duo::TransportInfo tr; tr.bpm = 138;
        const int blk = 512;
        juce::AudioBuffer<float> blkOut (2, blk);

        // renders `seconds`, injecting the given (timeSeconds, message) events; returns stats of the whole render
        struct Ev { double t; juce::MidiMessage m; };
        auto run = [&] (double seconds, std::vector<Ev> evs, const std::function<void (double)>& perBlock = {}) -> Stats
        {
            juce::AudioBuffer<float> all (2, (int) (seconds * sr));
            std::sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t; });
            size_t ei = 0; int pos = 0;
            while (pos < all.getNumSamples())
            {
                const int n = std::min (blk, all.getNumSamples() - pos);
                juce::MidiBuffer mb;
                while (ei < evs.size() && evs[ei].t * sr < pos + n) { mb.addEvent (evs[ei].m, std::max (0, (int) (evs[ei].t * sr) - pos)); ++ei; }
                if (perBlock) perBlock ((double) pos / sr);
                host.processBlock (all.getWritePointer (0) + pos, all.getWritePointer (1) + pos, n, mb, tr, hp);
                tr.ppq += n * tr.bpm / 60.0 / sr; pos += n;
            }
            lastRender = all;
            return analyse (all);
        };
        auto on = [] (int ch, int n) { return juce::MidiMessage::noteOn (ch, n, 0.9f); };
        auto off = [] (int ch, int n) { return juce::MidiMessage::noteOff (ch, n); };
        auto tailRms = [&] (double fromSec) { return analyse (lastRender, (int) (fromSec * sr)).rms; };

        // -- both synths through the host
        {
            std::vector<Ev> evs; for (int k : chord) { evs.push_back ({ 0.0, on (1, k) }); evs.push_back ({ 2.0, off (1, k) }); }
            auto st = run (4.0, evs);
            check (! st.bad && st.rms > 0.01f, "HOST layer: both synths audible and finite (rms " + juce::String (st.rms, 3) + ")");
            check (st.peak <= 0.9662f, "HOST layer: peak after limiter " + juce::String (st.peak, 4) + " <= -0.3 dB (0.9661)");
            check (host.heldCount (0) == 0 && host.heldCount (1) == 0, "HOST layer: nothing held after note-offs");
            writeWav (outDir.getChildFile ("host-layer-chord.wav"), lastRender, sr);
        }
        // -- limiter with absurd gain
        {
            hp.gainA = 12.0f; hp.gainB = 12.0f;
            std::vector<Ev> evs; for (int k : chord) { evs.push_back ({ 0.0, on (1, k) }); evs.push_back ({ 1.5, off (1, k) }); }
            auto st = run (2.5, evs);
            check (! st.bad && st.peak <= 0.9662f, "HOST limiter: +21 dB overdrive stays below -0.3 dBFS (peak " + juce::String (st.peak, 4) + ")");
            hp.gainA = hp.gainB = 1.0f;
        }
        // -- mute / solo
        {
            host.panic(); run (0.2, { });
            hp.muteA = true; hp.muteB = true;
            std::vector<Ev> evs; for (int k : chord) evs.push_back ({ 0.0, on (1, k) });
            run (2.0, evs);
            check (tailRms (1.0) < 1.0e-4f, "HOST mute both: output silent (rms " + juce::String (tailRms (1.0), 6) + ")");
            hp.muteA = hp.muteB = false;
            run (2.0, { });   // let everything settle
            hp.soloA = true; evs.clear(); for (int k : chord) evs.push_back ({ 0.0, on (1, k) });
            run (2.0, evs);
            const float soloRms = tailRms (1.0);
            check (host.heldCount (1) == 0 && host.heldCount (0) > 0, "HOST solo A: B holds no notes, A holds " + juce::String (host.heldCount (0)));
            check (soloRms > 0.005f, "HOST solo A: audible");
            hp.soloA = false;
            run (1.0, { });
        }
        // -- stuck notes: mute while holding, change routing mode while holding, panic
        {
            host.panic(); hp = duo::HostParams();
            std::vector<Ev> evs; for (int k : chord) evs.push_back ({ 0.0, on (1, k) });
            run (1.0, evs);
            hp.muteA = true;
            run (2.5, { });
            check (host.heldCount (0) == 0, "HOST stuck-note: muting A releases A's notes");
            check (A->activeVoices() == 0, "HOST stuck-note: A has no sounding voices 2.5 s after mute (" + juce::String (A->activeVoices()) + ")");
            hp.muteA = false;
            run (0.2, { });
            host.panic();
            evs.clear(); for (int k : chord) evs.push_back ({ 0.0, on (1, k) });
            run (1.0, evs);
            hp.mode = duo::Split;
            run (3.0, { });
            check (host.heldCount (0) == 0 && host.heldCount (1) == 0, "HOST stuck-note: mode change Layer->Split clears every held note");
            check (A->activeVoices() == 0 && B->activeVoices() == 0, "HOST stuck-note: no voices left after mode change (A " + juce::String (A->activeVoices()) + ", B " + juce::String (B->activeVoices()) + ")");
            hp.mode = duo::Layer; run (0.2, { });
            evs.clear(); for (int k : chord) evs.push_back ({ 0.0, on (1, k) });
            run (1.0, evs);
            host.panic();
            run (0.3, { });
            check (tailRms (0.2) < 1.0e-4f && host.heldCount (0) == 0 && host.heldCount (1) == 0, "HOST panic: instant silence, nothing held (rms " + juce::String (tailRms (0.2), 6) + ")");
        }
        // -- routing modes
        {
            host.panic(); hp = duo::HostParams(); hp.mode = duo::Split; hp.splitNote = 60;
            run (0.1, { });
            std::vector<Ev> evs { { 0.0, on (1, 48) }, { 0.0, on (1, 72) } };
            run (0.5, evs);
            const bool splitOk = host.heldCount (0) == 1 && host.heldCount (1) == 1;
            check (splitOk, "HOST split: note 48 -> A only, note 72 -> B only");
            host.panic(); hp.splitSwap = true; evs = { { 0.0, on (1, 48) }, { 0.0, on (1, 72) } };
            run (0.5, evs);
            check (host.heldCount (0) == 1 && host.heldCount (1) == 1, "HOST split swapped: still one note each");
            // which one? 48 should now sound on B: release 48 and see B empty
            run (0.2, { { 0.0, off (1, 48) } });
            check (host.heldCount (1) == 0 && host.heldCount (0) == 1, "HOST split swapped: note 48 belonged to B");
            host.panic(); hp.splitSwap = false;

            hp.mode = duo::Channel; hp.channelA = 1; hp.channelB = 2;
            run (0.1, { });
            run (0.5, { { 0.0, on (1, 60) }, { 0.0, on (2, 64) } });
            check (host.heldCount (0) == 1 && host.heldCount (1) == 1, "HOST channel: ch1 -> A, ch2 -> B");
            run (0.2, { { 0.0, off (2, 64) } });
            check (host.heldCount (1) == 0 && host.heldCount (0) == 1, "HOST channel: note-off on ch2 released B only");
            host.panic();
            hp.channelA = 0; hp.channelB = 3;   // A omni
            run (0.5, { { 0.0, on (5, 60) }, { 0.0, on (3, 62) } });
            check (host.heldCount (0) == 2 && host.heldCount (1) == 1, "HOST channel: omni A takes every channel, B only ch3");
            host.panic(); hp = duo::HostParams();
            run (0.5, { });
        }
        // -- CPU
        {
            host.panic(); hp = duo::HostParams();
            std::vector<float> a2 = sa::presetValues (0); a2[sa::o1_wave] = 4; a2[sa::o2_wave] = 4; a2[sa::uni_detune] = 0.5f; a2[sa::fx_ch_on] = 1; a2[sa::fx_dl_on] = 1; a2[sa::fx_rv_on] = 1; a2[sa::f1_model] = 0;
            std::vector<float> b2 (pb); b2[sb::o2_lvl] = 0.8f; b2[sb::uni_voices] = 8; b2[sb::o1_w1] = 1; b2[sb::o1_w1a] = 0.3f; b2[sb::r1_type] = sb::FX_CHORUS; b2[sb::r2_type] = sb::FX_DELAY; b2[sb::r3_type] = sb::FX_REVERB;
            A->setParamSnapshot (a2.data()); B->setParamSnapshot (b2.data());
            std::vector<Ev> evs; for (int k : { 36, 43, 48, 52, 55, 59, 60, 64, 67, 71, 74, 79 }) evs.push_back ({ 0.0, on (1, k) });
            const auto t0 = std::chrono::steady_clock::now();
            run (4.0, evs);
            const double el = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
            std::printf ("INFO  CPU: 12-note chord on BOTH synths (A hypersaw x7 x2 osc + FX, B 8-voice unison x2 osc + warp + rack): %.2f s to render 4 s -> %.1f%% of one core, %d + %d voices\n", el, 100.0 * el / 4.0, A->activeVoices(), B->activeVoices());
            check (el < 4.0, "HOST CPU: renders faster than real time");
            // glitch onset: keep adding B unison until it no longer runs in real time
            A->setParamSnapshot (sa::presetValues (0).data());
            for (int uni : { 4, 8, 12, 16 })
            {
                std::vector<float> b3 (pb); b3[sb::o2_lvl] = 0.8f; b3[sb::uni_voices] = (float) uni; b3[sb::o1_w1] = 1; b3[sb::o1_w1a] = 0.3f;
                B->setParamSnapshot (b3.data()); host.panic(); run (0.1, { });
                std::vector<Ev> e2; for (int k = 0; k < 16; ++k) e2.push_back ({ 0.0, on (1, 40 + 3 * k) });
                const auto t1 = std::chrono::steady_clock::now();
                run (2.0, e2);
                const double el2 = std::chrono::duration<double> (std::chrono::steady_clock::now() - t1).count();
                std::printf ("INFO  CPU: PRISM 16 voices x %2d unison x 2 osc (+ warp): %.1f%% of one core\n", uni, 100.0 * el2 / 2.0);
            }
        }
    }

    std::printf ("\n%s (%d failure%s)\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}








