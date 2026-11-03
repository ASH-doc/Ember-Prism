// FilterProbe.cpp — Measures the low-pass corner of a rendered noise signal (calibration of a source filter's cutoff control).
//   DualFilterProbe <render.wav> [startSeconds=0.3] [lengthSeconds=1.2]
// Averages the power spectrum of the chosen part (Welch, Hann, 8192 points), smooths it in 1/6-octave steps, takes the level
// of the passband as the median of 60 to 200 Hz, and reports the frequencies where the response is 3, 12 and 24 dB below it,
// plus the slope between the -12 and -24 dB points in dB per octave.
#include <juce_audio_formats/juce_audio_formats.h>
#include <complex>
#include <iostream>
#include <vector>
namespace {
void fft(std::vector<std::complex<double>>& a) {
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) { size_t bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap(a[i], a[j]); }
    for (size_t len = 2; len <= n; len <<= 1) {
        const double ang = -2.0 * 3.14159265358979323846 / double(len); const std::complex<double> wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) { std::complex<double> w(1, 0); for (size_t j = 0; j < len / 2; ++j) { auto u = a[i + j], v = a[i + j + len / 2] * w; a[i + j] = u + v; a[i + j + len / 2] = u - v; w *= wl; } }
    }
}
}
// Smoothed spectrum (frequency, dB) of part of a file; shared by the single-file and the ratio modes.
static bool smoothedSpectrum(const juce::File& file, double start, double length, std::vector<std::pair<double, double>>& curve) {
    juce::AudioFormatManager manager; manager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(file));
    if (!reader) return false;
    const double rate = reader->sampleRate; const int from = int(start * rate), count = int(length * rate);
    if (reader->lengthInSamples < from + count) return false;
    juce::AudioBuffer<float> buffer(int(std::max<unsigned>(1, std::min<unsigned>(reader->numChannels, 2))), count);
    reader->read(&buffer, 0, count, from, true, true);
    std::vector<double> mono(size_t(count), 0.0);
    for (int c = 0; c < buffer.getNumChannels(); ++c) for (int i = 0; i < count; ++i) mono[size_t(i)] += buffer.getSample(c, i) / buffer.getNumChannels();
    const size_t N = 8192; std::vector<double> power(N / 2, 0.0); int windows = 0;
    for (size_t at = 0; at + N <= mono.size(); at += N / 2) {
        std::vector<std::complex<double>> a(N); for (size_t i = 0; i < N; ++i) a[i] = mono[at + i] * (0.5 - 0.5 * std::cos(2.0 * 3.14159265358979323846 * double(i) / double(N)));
        fft(a); for (size_t k = 0; k < N / 2; ++k) power[k] += std::norm(a[k]); ++windows;
    }
    curve.clear();
    for (double f = 40.0; f < 20000.0 && f < rate / 2; f *= std::pow(2.0, 1.0 / 6.0)) {
        const double lo = f / std::pow(2.0, 1.0 / 12.0), hi = f * std::pow(2.0, 1.0 / 12.0); double sum = 0; int n = 0;
        for (size_t k = 1; k < N / 2; ++k) { const double hz = double(k) * rate / double(N); if (hz >= lo && hz < hi) { sum += power[k]; ++n; } }
        if (n > 0) curve.push_back({ f, 10.0 * std::log10(std::max(sum / n / windows, 1e-30)) });
    }
    return !curve.empty();
}
// Ratio mode: DualFilterProbe render.wav --ratio open.wav  prints the response of render relative to the open-filter render,
// which cancels the colour of the source (the noise oscillator is not white).
static int ratioMode(const juce::String& renderPath, const juce::String& openPath) {
    std::vector<std::pair<double, double>> a, b;
    if (!smoothedSpectrum(juce::File::getCurrentWorkingDirectory().getChildFile(renderPath), 0.3, 1.2, a) || !smoothedSpectrum(juce::File::getCurrentWorkingDirectory().getChildFile(openPath), 0.3, 1.2, b) || a.size() != b.size()) { std::cerr << "cannot read both files\n"; return 1; }
    std::vector<std::pair<double, double>> h; for (size_t i = 0; i < a.size(); ++i) h.push_back({ a[i].first, a[i].second - b[i].second });
    std::vector<double> plateau; for (const auto& p : h) if (p.first >= 60 && p.first <= 150) plateau.push_back(p.second);
    std::sort(plateau.begin(), plateau.end()); const double level = plateau[plateau.size() / 2];
    auto crossing = [&](double drop) { for (size_t i = 1; i < h.size(); ++i) if (h[i].first > 100 && h[i].second < level - drop && h[i - 1].second >= level - drop) { const double t = (level - drop - h[i - 1].second) / (h[i].second - h[i - 1].second); return std::exp(std::log(h[i - 1].first) + t * (std::log(h[i].first) - std::log(h[i - 1].first))); } return 0.0; };
    const double f3 = crossing(3), f12 = crossing(12), f24 = crossing(24);
    std::cout << "relative to open: passband " << level << " dB, -3 dB at " << f3 << " Hz, -12 dB at " << f12 << " Hz, -24 dB at " << f24 << " Hz";
    if (f12 > 0 && f24 > 0) std::cout << ", slope " << 12.0 / std::log2(f24 / f12) << " dB/oct";
    std::cout << "\n"; return 0;
}
int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: DualFilterProbe render.wav [start] [length]   or   DualFilterProbe render.wav --ratio open.wav\n"; return 2; }
    if (argc >= 4 && juce::String(argv[2]) == "--ratio") return ratioMode(argv[1], argv[3]);
    const double start = argc > 2 ? juce::String(argv[2]).getDoubleValue() : 0.3, length = argc > 3 ? juce::String(argv[3]).getDoubleValue() : 1.2;
    juce::AudioFormatManager manager; manager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(juce::File::getCurrentWorkingDirectory().getChildFile(argv[1])));
    if (!reader) { std::cerr << "cannot read file\n"; return 1; }
    const double rate = reader->sampleRate; const int from = int(start * rate), count = int(length * rate);
    if (reader->lengthInSamples < from + count) { std::cerr << "file too short (" << double(reader->lengthInSamples) / rate << " s)\n"; return 1; }
    juce::AudioBuffer<float> buffer(int(std::max<unsigned>(1, std::min<unsigned>(reader->numChannels, 2))), count);
    reader->read(&buffer, 0, count, from, true, true);
    std::vector<double> mono(size_t(count), 0.0);
    for (int c = 0; c < buffer.getNumChannels(); ++c) for (int i = 0; i < count; ++i) mono[size_t(i)] += buffer.getSample(c, i) / buffer.getNumChannels();
    const size_t N = 8192; std::vector<double> power(N / 2, 0.0); int windows = 0;
    for (size_t at = 0; at + N <= mono.size(); at += N / 2) {
        std::vector<std::complex<double>> a(N); for (size_t i = 0; i < N; ++i) a[i] = mono[at + i] * (0.5 - 0.5 * std::cos(2.0 * 3.14159265358979323846 * double(i) / double(N)));
        fft(a); for (size_t k = 0; k < N / 2; ++k) power[k] += std::norm(a[k]); ++windows;
    }
    // 1/6 octave smoothing from 40 Hz to 20 kHz
    std::vector<std::pair<double, double>> curve;   // frequency, dB
    for (double f = 40.0; f < 20000.0 && f < rate / 2; f *= std::pow(2.0, 1.0 / 6.0)) {
        const double lo = f / std::pow(2.0, 1.0 / 12.0), hi = f * std::pow(2.0, 1.0 / 12.0); double sum = 0; int n = 0;
        for (size_t k = 1; k < N / 2; ++k) { const double hz = double(k) * rate / double(N); if (hz >= lo && hz < hi) { sum += power[k]; ++n; } }
        if (n > 0) curve.push_back({ f, 10.0 * std::log10(std::max(sum / n / windows, 1e-30)) });
    }
    if (argc > 4) { for (const auto& p : curve) std::cout << int(p.first) << " Hz: " << p.second << " dB\n"; }
    std::vector<double> plateau; for (const auto& p : curve) if (p.first >= 60 && p.first <= 200) plateau.push_back(p.second);
    if (plateau.empty()) { std::cerr << "no plateau\n"; return 1; }
    std::sort(plateau.begin(), plateau.end()); const double level = plateau[plateau.size() / 2];
    auto crossing = [&](double drop) { for (size_t i = 1; i < curve.size(); ++i) if (curve[i].first > 200 && curve[i].second < level - drop && curve[i - 1].second >= level - drop) { const double t = (level - drop - curve[i - 1].second) / (curve[i].second - curve[i - 1].second); return std::exp(std::log(curve[i - 1].first) + t * (std::log(curve[i].first) - std::log(curve[i - 1].first))); } return 0.0; };
    const double f3 = crossing(3), f12 = crossing(12), f24 = crossing(24);
    std::cout << "plateau " << level << " dB (relative), windows " << windows << "\n-3 dB at " << f3 << " Hz\n-12 dB at " << f12 << " Hz\n-24 dB at " << f24 << " Hz\n";
    if (f12 > 0 && f24 > 0) std::cout << "slope between -12 and -24 dB: " << 12.0 / std::log2(f24 / f12) << " dB/octave\n";
    return 0;
}
