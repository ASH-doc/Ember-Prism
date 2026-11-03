#pragma once
// Objective "does it sound alike" features used to compare a preset rendered by the Claude engines with the same preset
// converted and rendered by the ChatGPT engines. Plain C++ (no JUCE). This file is identical in both projects.
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <vector>

namespace match {

constexpr int kBands = 31;                 // 1/3-octave bands, 40 Hz .. 16 kHz
constexpr double kPi = 3.14159265358979323846;

struct Features {
    float rmsDb = -120.f, peakDb = -120.f, attackMs = 0.f, releaseDb = 0.f;
    std::array<float, kBands> band {};     // dB, relative to the loudest band
};

inline void fft (std::vector<std::complex<double>>& a)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) { size_t bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap (a[i], a[j]); }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = -2.0 * kPi / (double) len; const std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len) { std::complex<double> w (1, 0); for (size_t j = 0; j < len / 2; ++j) { auto u = a[i + j], v = a[i + j + len / 2] * w; a[i + j] = u + v; a[i + j + len / 2] = u - v; w *= wl; } }
    }
}

inline float dbOf (double x) { return (float) (20.0 * std::log10 (std::max (x, 1e-9))); }

// mono = left+right averaged, sampleRate in Hz. The note is held for holdSeconds then released.
inline Features analyse (const std::vector<float>& mono, double sr, double holdSeconds)
{
    Features f;
    const size_t n = mono.size(); if (n < 1024) return f;
    auto rmsOver = [&] (double t0, double t1) { size_t a = (size_t) (t0 * sr), b = std::min (n, (size_t) (t1 * sr)); if (b <= a) return 0.0; double s = 0; for (size_t i = a; i < b; ++i) s += (double) mono[i] * mono[i]; return std::sqrt (s / (double) (b - a)); };
    double peak = 0; for (float x : mono) peak = std::max (peak, (double) std::fabs (x));
    f.peakDb = dbOf (peak);
    f.rmsDb = dbOf (rmsOver (0.0, holdSeconds));
    // attack: first 5 ms window whose rms reaches 90% of the loudest 5 ms window before the release
    const size_t win = (size_t) (0.005 * sr); double best = 0; std::vector<double> env;
    for (size_t i = 0; i + win < (size_t) (holdSeconds * sr) && i + win < n; i += win) { env.push_back (rmsOver ((double) i / sr, (double) (i + win) / sr)); best = std::max (best, env.back()); }
    for (size_t k = 0; k < env.size(); ++k) if (env[k] >= 0.9 * best) { f.attackMs = (float) (k * 5.0); break; }
    // release: level 300 ms after note-off relative to the level just before it
    const double before = rmsOver (holdSeconds - 0.05, holdSeconds), after = rmsOver (holdSeconds + 0.3, holdSeconds + 0.35);
    f.releaseDb = dbOf (after) - dbOf (before);
    // spectrum of the sustain (0.2 s .. 0.2 s + 16384 samples), Hann window
    const size_t N = 16384, start = (size_t) (0.2 * sr); if (start + N > n) return f;
    std::vector<std::complex<double>> a (N); for (size_t i = 0; i < N; ++i) a[i] = (double) mono[start + i] * (0.5 - 0.5 * std::cos (2.0 * kPi * (double) i / (double) N));
    fft (a);
    std::array<double, kBands> e {};
    for (size_t k = 1; k < N / 2; ++k)
    {
        const double hz = (double) k * sr / (double) N; if (hz < 35.0 || hz > 18000.0) continue;
        const int b = (int) std::floor (3.0 * std::log2 (hz / 40.0) + 0.5); if (b < 0 || b >= kBands) continue;
        e[(size_t) b] += std::norm (a[k]);
    }
    double mx = 1e-30; for (double v : e) mx = std::max (mx, v);
    for (int b = 0; b < kBands; ++b) f.band[(size_t) b] = (float) (10.0 * std::log10 (std::max (e[(size_t) b], mx * 1e-9) / mx));
    return f;
}

// 0 = identical .. larger = further apart.  Spectral part: mean absolute dB difference over the bands, weighted by band energy.
// Level part: loudness difference in dB. Envelope part: attack time difference in ms (capped) and release difference in dB.
struct Distance { float spectrumDb = 0, levelDb = 0, attackMs = 0, releaseDb = 0, score = 0; };

inline Distance compare (const Features& a, const Features& b)
{
    Distance d;
    double wsum = 0, acc = 0;
    for (int i = 0; i < kBands; ++i) { const double w = std::pow (10.0, std::max (a.band[(size_t) i], b.band[(size_t) i]) / 20.0); wsum += w; acc += w * std::fabs ((double) a.band[(size_t) i] - (double) b.band[(size_t) i]); }
    d.spectrumDb = (float) (acc / std::max (wsum, 1e-9));
    d.levelDb = std::fabs (a.rmsDb - b.rmsDb);
    d.attackMs = std::fabs (a.attackMs - b.attackMs);
    d.releaseDb = std::fabs (std::max (-60.f, a.releaseDb) - std::max (-60.f, b.releaseDb));   // below -60 dB both are "silent"
    // one number for sorting: spectrum matters most, then loudness, then envelope
    d.score = d.spectrumDb + 0.5f * d.levelDb + 0.02f * std::min (d.attackMs, 200.f) + 0.2f * std::min (d.releaseDb, 20.f);
    return d;
}

} // namespace match
