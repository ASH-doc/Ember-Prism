#pragma once
// ImportTypes.h — The shared data contract of the Serum / Spire import (contract v1, reviews/serum-spire-contract-v1.md).
//
//   preset file --decoder--> DecodedPatch --translator--> native patch + ImportReport --UI--> preview / apply
//
// Plain C++17, no JUCE types, so the same structures can be used by the decoders, the translators, the preview UI,
// the tests and the macOS build. Nothing in this header touches the audio thread or the live patch.
#include <cstdint>
#include <string>
#include <vector>

namespace aether::import {

enum class SourceFormat { Unknown, SpireSpf2, Serum1Fxp, Serum2Preset };
enum class TargetEngine { Ember, Prism };

// One row of the import report. `Mapped` means a verified native representation exists (it says nothing about how the
// patch sounds). `Approximated` is a named substitution. `Omitted` is an active feature that could not be carried over.
enum class RowStatus { Mapped, Approximated, Omitted };

// File-level result. `Blocked` keeps Apply disabled (unknown version, malformed data, unresolved required asset, or no
// minimum playable voice). `Approximation` is playable but needs the explicit Apply Approximation action.
enum class FileResult { Opened, Approximation, Blocked };

// A single named value read from a source file, exactly as stored (no unit conversion).
struct DecodedField {
    std::string name;    // as stored in the file, for example "flt1_cut"
    std::string group;   // derived from the name prefix: "osc1", "flt1", "env3", "slot12", "global", ...
    double value = 0;    // normalised value as stored
    bool outOfRange = false;  // stored value is outside the normalised range 0..1 (kept, flagged)
};

struct DecodedPatch {
    SourceFormat format = SourceFormat::Unknown;
    std::string formatVersion;           // from the file when it carries one, otherwise empty
    std::string name;                    // supplied by the caller (file name without extension) when the file has none
    std::string author, vendor, bank, notes;
    std::vector<std::string> tags;
    std::vector<DecodedField> fields;    // file order
    std::vector<std::string> warnings;   // non-fatal findings (ignored keys, flagged values)
};

// Result of a decoder. When `ok` is false, `blocker` says why and `patch` must not be used.
struct DecodeResult {
    bool ok = false;
    std::string blocker;
    DecodedPatch patch;
};

struct ReportRow {
    std::string parameter;     // native or source parameter the row is about
    std::string sourceValue;   // as the user would read it in the source plug-in (with unit) or the raw stored value
    std::string nativeValue;   // the AEther value that was written, or empty when omitted
    RowStatus status = RowStatus::Mapped;
    std::string reason;        // optional: why a value was substituted, omitted or is missing a dependency
};

struct ImportReport {
    FileResult result = FileResult::Blocked;
    SourceFormat format = SourceFormat::Unknown;
    TargetEngine target = TargetEngine::Ember;
    std::string headline;      // one line for the top of the preview
    std::string blocker;       // set when result == Blocked
    std::vector<ReportRow> rows;
    int mapped() const { int n = 0; for (const auto& r : rows) n += r.status == RowStatus::Mapped; return n; }
    int approximated() const { int n = 0; for (const auto& r : rows) n += r.status == RowStatus::Approximated; return n; }
    int omitted() const { int n = 0; for (const auto& r : rows) n += r.status == RowStatus::Omitted; return n; }
};

inline const char* toString(RowStatus s) { return s == RowStatus::Mapped ? "Mapped" : s == RowStatus::Approximated ? "Approximated" : "Omitted"; }
inline const char* toString(FileResult r) { return r == FileResult::Opened ? "Opened" : r == FileResult::Approximation ? "Approximation" : "Blocked"; }
inline const char* toString(SourceFormat f) { return f == SourceFormat::SpireSpf2 ? "Spire .spf2" : f == SourceFormat::Serum1Fxp ? "Serum 1 .fxp" : f == SourceFormat::Serum2Preset ? "Serum 2 .SerumPreset" : "Unknown"; }

} // namespace aether::import
