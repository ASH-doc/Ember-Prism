// SpireImport.cpp — Translates Spire .spf2 presets to EMBER patches and scores them against renders of the originals.
//   DualSpireImport translate <preset.spf2 or folder> <output folder> [holdSeconds=1.846]
//        translate, render EMBER, write <name>.ember.json, <name>.report.md and <name>.ember.wav
//   DualSpireImport batch <presets folder> <reference renders folder> <output folder> [holdSeconds=1.846]
//        for every preset with a matching <name>.wav render of the original (made in FL Studio from the fixed reference note),
//        translate, then correct loudness, filter cutoff and envelope times against the render and print the score before and after
// Contract v1: the score is the agreed objective measurement (Tools/ClaudeImport/MatchFeatures.h). Third-party presets and renders
// are read in place and the outputs stay on the local machine; only aggregate numbers go in the repository.
#include <JuceHeader.h>
#include "Import/SpireDecoder.h"
#include "Import/SpireToEmber.h"
#include "sa-processor.h"
#include "MatchFeatures.h"
#include <cstdio>

using namespace juce;
using namespace aether::import;

namespace {

struct Render { std::vector<float> mono; double rate = 48000; };

Render renderEmber(const var& patch, double rate, double hold) {
    Render r; r.rate = rate;
    auto processor = std::make_unique<SynthAProcessor>();
    if (auto result = processor->setPatch(patch); result.failed()) { std::fprintf(stderr, "EMBER rejected the patch: %s\n", result.getErrorMessage().toRawUTF8()); return r; }
    const int block = 256, total = int((hold + 0.8) * rate);
    processor->setRateAndBufferSizeDetails(rate, block); processor->prepareToPlay(rate, block);
    r.mono.assign(size_t(total), 0.f); AudioBuffer<float> audio(2, block); bool on = false, off = false;
    for (int at = 0; at < total; at += block) {
        MidiBuffer midi; if (!on) { midi.addEvent(MidiMessage::noteOn(1, 60, .8f), 0); on = true; }
        if (!off && double(at) >= hold * rate) { midi.addEvent(MidiMessage::noteOff(1, 60), 0); off = true; }
        processor->processBlock(audio, midi);
        for (int i = 0; i < block && at + i < total; ++i) r.mono[size_t(at + i)] = .5f * (audio.getSample(0, i) + audio.getSample(1, i));
    }
    return r;
}

bool loadWav(const File& file, Render& out) {
    AudioFormatManager manager; manager.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> reader(manager.createReaderFor(file));
    if (!reader || reader->lengthInSamples < 2048) return false;
    const int n = int(std::min<int64>(reader->lengthInSamples, int64(reader->sampleRate * 20)));
    AudioBuffer<float> buffer(int(std::max<unsigned>(1, std::min<unsigned>(reader->numChannels, 2))), n);
    if (!reader->read(&buffer, 0, n, 0, true, true)) return false;
    out.rate = reader->sampleRate; out.mono.assign(size_t(n), 0.f);
    for (int c = 0; c < buffer.getNumChannels(); ++c) for (int i = 0; i < n; ++i) out.mono[size_t(i)] += buffer.getSample(c, i) / float(buffer.getNumChannels());
    return true;
}

void writeWav(const File& file, const Render& r) {
    file.deleteFile(); WavAudioFormat format; std::unique_ptr<FileOutputStream> stream(file.createOutputStream()); if (!stream) return;
    std::unique_ptr<AudioFormatWriter> writer(format.createWriterFor(stream.get(), r.rate, 1, 24, {}, 0)); if (!writer) return; stream.release();
    AudioBuffer<float> buffer(1, int(r.mono.size())); buffer.copyFrom(0, 0, r.mono.data(), int(r.mono.size())); writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

const char* band(float score) { return score <= 4.f ? "close" : score <= 9.f ? "approximate" : "different"; }

struct Outcome { match::Features features; match::Distance distance; EmberCalibration calibration; EmberTranslation translation; };

Outcome evaluate(const DecodedPatch& decoded, const EmberCalibration& calibration, const match::Features& reference, double rate, double hold) {
    Outcome o; o.calibration = calibration; o.translation = translateSpireToEmber(decoded, calibration);
    if (o.translation.report.result == FileResult::Blocked) { o.distance.score = 99; return o; }
    const auto render = renderEmber(o.translation.patch, rate, hold);
    o.features = match::analyse(render.mono, rate, hold); o.distance = match::compare(reference, o.features);
    return o;
}

// Loudness first (two passes), then a small search over cutoff shift and envelope time scale, then loudness again.
Outcome calibrate(const DecodedPatch& decoded, const match::Features& reference, double rate, double hold) {
    EmberCalibration c; Outcome best = evaluate(decoded, c, reference, rate, hold);
    auto trim = [&](Outcome& o) { for (int pass = 0; pass < 2; ++pass) { if (o.translation.report.result == FileResult::Blocked || o.features.rmsDb < -100.f) return; o.calibration.masterVolumeDb = jlimit(-60.0, 6.0, o.calibration.masterVolumeDb + double(reference.rmsDb - o.features.rmsDb)); o = evaluate(decoded, o.calibration, reference, rate, hold); } };
    trim(best);
    if (best.translation.report.result == FileResult::Blocked) return best;
    for (int round = 0; round < 2; ++round) {
        for (double shift : { -2.0, -1.0, -0.5, 0.5, 1.0, 2.0 }) { auto c2 = best.calibration; c2.cutoffShiftOctaves += shift; auto o = evaluate(decoded, c2, reference, rate, hold); trim(o); if (o.distance.score < best.distance.score - 0.05f) best = o; }
        for (double scale : { -3.0, -2.0, -1.0, 1.0, 2.0, 3.0 }) { auto c2 = best.calibration; c2.timeScaleOctaves += scale; auto o = evaluate(decoded, c2, reference, rate, hold); trim(o); if (o.distance.score < best.distance.score - 0.05f) best = o; }
    }
    return best;
}

String reportText(const DecodedPatch& decoded, const ImportReport& report) {
    String md = "# " + String(decoded.name) + "\n\nResult: **" + String(toString(report.result)) + "** - " + String(report.headline) + "\n\n";
    if (!report.blocker.empty()) md << "Blocker: " << String(report.blocker) << "\n\n";
    md << "| Parameter | Source value | Native value | Status | Reason |\n|---|---|---|---|---|\n";
    for (const auto& r : report.rows) md << "| " << String(r.parameter) << " | " << String(r.sourceValue) << " | " << String(r.nativeValue) << " | " << String(toString(r.status)) << " | " << String(r.reason) << " |\n";
    return md;
}

bool decodeFile(const File& file, DecodedPatch& out) {
    MemoryBlock data; if (!file.loadFileAsData(data)) return false;
    auto result = decodeSpire(data.getData(), data.getSize(), file.getFileNameWithoutExtension().toStdString());
    if (!result.ok) { std::fprintf(stderr, "%s: %s\n", file.getFileName().toRawUTF8(), result.blocker.c_str()); return false; }
    out = std::move(result.patch); return true;
}

} // namespace

int main(int argc, char** argv) {
    ScopedJuceInitialiser_GUI gui;
    if (argc < 4) { std::fprintf(stderr, "usage: DualSpireImport translate <preset or folder> <out folder> [hold]\n       DualSpireImport batch <presets folder> <reference folder> <out folder> [hold]\n"); return 2; }
    const String mode(argv[1]);
    if (mode == "translate") {
        const File input = File::getCurrentWorkingDirectory().getChildFile(argv[2]), out = File::getCurrentWorkingDirectory().getChildFile(argv[3]); out.createDirectory();
        const double hold = argc > 4 ? String(argv[4]).getDoubleValue() : 1.846154;
        Array<File> files; if (input.isDirectory()) files = input.findChildFiles(File::findFiles, false, "*.spf2"); else files.add(input);
        for (const auto& file : files) {
            DecodedPatch decoded; if (!decodeFile(file, decoded)) continue;
            const auto t = translateSpireToEmber(decoded);
            out.getChildFile(file.getFileNameWithoutExtension() + ".report.md").replaceWithText(reportText(decoded, t.report));
            if (t.report.result != FileResult::Blocked) { out.getChildFile(file.getFileNameWithoutExtension() + ".ember.json").replaceWithText(JSON::toString(t.patch, false)); writeWav(out.getChildFile(file.getFileNameWithoutExtension() + ".ember.wav"), renderEmber(t.patch, 48000, hold)); }
            std::printf("%s: %s (%d mapped, %d approximated, %d omitted)\n", file.getFileName().toRawUTF8(), toString(t.report.result), t.report.mapped(), t.report.approximated(), t.report.omitted());
        }
        return 0;
    }
    if (mode == "batch" && argc >= 5) {
        const File presets = File::getCurrentWorkingDirectory().getChildFile(argv[2]), refs = File::getCurrentWorkingDirectory().getChildFile(argv[3]), out = File::getCurrentWorkingDirectory().getChildFile(argv[4]); out.createDirectory();
        const double hold = argc > 5 ? String(argv[5]).getDoubleValue() : 1.846154;
        int counts[3] = { 0, 0, 0 }, blocked = 0, total = 0; String summary = "| preset | import | score before | score after | band after | level dB | cutoff shift (oct) | time scale (oct) |\n|---|---|---|---|---|---|---|---|\n";
        auto files = presets.findChildFiles(File::findFiles, false, "*.spf2"); files.sort();
        for (const auto& file : files) {
            const File refFile = refs.getChildFile(file.getFileNameWithoutExtension() + ".wav"); if (!refFile.existsAsFile()) continue;
            DecodedPatch decoded; if (!decodeFile(file, decoded)) continue; Render ref; if (!loadWav(refFile, ref)) continue; ++total;
            const auto reference = match::analyse(ref.mono, ref.rate, hold);
            const auto before = evaluate(decoded, EmberCalibration{}, reference, ref.rate, hold); const auto after = calibrate(decoded, reference, ref.rate, hold);
            if (after.translation.report.result == FileResult::Blocked) { ++blocked; summary << "| " << file.getFileNameWithoutExtension() << " | Blocked | | | | | | |\n"; continue; }
            ++counts[after.distance.score <= 4.f ? 0 : after.distance.score <= 9.f ? 1 : 2];
            out.getChildFile(file.getFileNameWithoutExtension() + ".ember.json").replaceWithText(JSON::toString(after.translation.patch, false));
            out.getChildFile(file.getFileNameWithoutExtension() + ".report.md").replaceWithText(reportText(decoded, after.translation.report));
            summary << "| " << file.getFileNameWithoutExtension() << " | " << toString(after.translation.report.result) << " | " << String(before.distance.score, 1) << " | " << String(after.distance.score, 1) << " | " << band(after.distance.score) << " | " << String(after.calibration.masterVolumeDb, 1) << " | " << String(after.calibration.cutoffShiftOctaves, 1) << " | " << String(after.calibration.timeScaleOctaves, 1) << " |\n";
            std::printf("%s: score %.1f -> %.1f (%s)\n", file.getFileNameWithoutExtension().toRawUTF8(), before.distance.score, after.distance.score, band(after.distance.score));
        }
        summary << "\nCompared: " << total << ", blocked " << blocked << ", close " << counts[0] << ", approximate " << counts[1] << ", different " << counts[2] << "\n";
        out.getChildFile("summary.md").replaceWithText(summary); std::printf("%s", summary.toRawUTF8()); return 0;
    }
    return 2;
}
