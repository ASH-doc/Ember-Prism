// CompareWav.cpp — The agreed objective score (contract v1) between two rendered WAV files.
//   DualCompareWav <reference.wav> <candidate.wav> [holdSeconds=2.0]
// Both files must use the same sample rate. The note is held for `holdSeconds` and then released; each file must be at
// least holdSeconds + 0.4 s long. The formula is the one in Tools/ClaudeImport/MatchFeatures.h (documented in
// docs/import/evaluation-notes.md): a score of at most 4 is "close", at most 9 "approximate", above 9 "different".
#include <juce_audio_formats/juce_audio_formats.h>
#include "MatchFeatures.h"
#include <iostream>
namespace {
bool loadMono(const juce::File& file, std::vector<float>& mono, double& rate) {
    juce::AudioFormatManager manager; manager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(file));
    if (!reader || reader->lengthInSamples < 1024 || reader->lengthInSamples > 48000 * 120) return false;
    rate = reader->sampleRate; const int n = int(reader->lengthInSamples);
    juce::AudioBuffer<float> buffer(int(std::max<unsigned>(1, std::min<unsigned>(reader->numChannels, 2))), n);
    if (!reader->read(&buffer, 0, n, 0, true, true)) return false;
    mono.assign(size_t(n), 0.f);
    for (int c = 0; c < buffer.getNumChannels(); ++c) for (int i = 0; i < n; ++i) mono[size_t(i)] += buffer.getSample(c, i) / float(buffer.getNumChannels());
    return true;
}
}
int main(int argc, char** argv) {
    if (argc < 3) { std::cerr << "usage: DualCompareWav reference.wav candidate.wav [holdSeconds]\n"; return 2; }
    const double hold = argc > 3 ? juce::String(argv[3]).getDoubleValue() : 2.0;
    std::vector<float> a, b; double rateA = 0, rateB = 0;
    if (!loadMono(juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]), a, rateA)) { std::cerr << "cannot read reference\n"; return 1; }
    if (!loadMono(juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]), b, rateB)) { std::cerr << "cannot read candidate\n"; return 1; }
    if (std::abs(rateA - rateB) > 0.5) { std::cerr << "sample rates differ (" << rateA << " vs " << rateB << ")\n"; return 1; }
    if (double(a.size()) / rateA < hold + 0.4 || double(b.size()) / rateB < hold + 0.4) { std::cerr << "files are shorter than hold + 0.4 s\n"; return 1; }
    const auto fa = match::analyse(a, rateA, hold), fb = match::analyse(b, rateB, hold);
    const auto d = match::compare(fa, fb);
    std::cout << "reference: rms " << fa.rmsDb << " dB, attack " << fa.attackMs << " ms, release " << fa.releaseDb << " dB\n"
              << "candidate: rms " << fb.rmsDb << " dB, attack " << fb.attackMs << " ms, release " << fb.releaseDb << " dB\n"
              << "spectrum " << d.spectrumDb << " dB, level " << d.levelDb << " dB, attack " << d.attackMs << " ms, release " << d.releaseDb << " dB\n"
              << "score " << d.score << " => " << (d.score <= 4.f ? "close" : d.score <= 9.f ? "approximate" : "different") << "\n";
    return 0;
}
