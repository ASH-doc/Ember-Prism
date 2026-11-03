// ImportTests.cpp — Decoder tests on synthetic data only (contract v1: no third-party preset content in the repo).
#include "Import/SpireDecoder.h"
#include "Import/SpireTables.h"
#include <juce_core/juce_core.h>
#include <iostream>
using namespace aether::import;
namespace spire = aether::import::spire;
namespace {
int failures = 0, checks = 0;
void require(bool ok, const std::string& what) { ++checks; if (!ok) { ++failures; std::cerr << "FAIL: " << what << "\n"; } }

std::string loadFixture() {
    const auto file = juce::File(__FILE__).getParentDirectory().getChildFile("fixtures/import/synthetic-spire-minimal.spf2");
    require(file.existsAsFile(), "synthetic Spire fixture is present next to the tests");
    return file.loadFileAsString().toStdString();
}
DecodeResult decode(const std::string& text, const std::string& name = "case") { return decodeSpire(text.data(), text.size(), name); }
std::string without(std::string text, const std::string& key) {
    const auto at = text.find("\"" + key + "\"");
    if (at == std::string::npos) return text;
    const auto end = text.find('\n', at);
    text.erase(at, end - at + 1);
    return text;
}
std::string replaceFirst(std::string text, const std::string& from, const std::string& to) {
    const auto at = text.find(from);
    if (at != std::string::npos) text.replace(at, from.size(), to);
    return text;
}
struct Lcg { uint64_t s = 0x9E3779B97F4A7C15ull; uint32_t next() { s = s * 6364136223846793005ull + 1442695040888963407ull; return uint32_t(s >> 33); } };
}

int main() {
    const std::string good = loadFixture();

    // ---- A clean file decodes, with metadata and groups ------------------------------------------------------------------
    {
        const auto r = decode(good, "Synthetic Bass 01");
        require(r.ok, "synthetic fixture decodes");
        require(r.patch.format == SourceFormat::SpireSpf2, "format is recorded");
        require(r.patch.fields.size() == 137, "all 137 fields are read");
        require(r.patch.name == "Synthetic Bass 01", "the patch is named after the file");
        require(r.patch.vendor == "AEther tests" && r.patch.author == "Synthetic fixture", "metadata is read");
        require(r.patch.tags.size() == 2, "tags are read");
        bool sawGroups = false, sawGlobal = false;
        for (const auto& f : r.patch.fields) { if (f.name == "osc1_wave") sawGroups = f.group == "osc1"; if (f.name == "volume") sawGlobal = f.group == "global"; }
        require(sawGroups && sawGlobal, "groups come from the name prefix");
        require(!r.patch.warnings.empty(), "the ignored icon is mentioned as a warning");
        require(spireEnumIndex(0.002002) == 2 && spireEnumIndex(0.017017) == 17 && spireEnumIndex(0) == 0, "list choices decode as index / 999");
        require(spireOnEnumLattice(0.007007) && !spireOnEnumLattice(0.5) && !spireOnEnumLattice(0.47) && !spireOnEnumLattice(1.5), "lattice check");
    }
    // The menu tables agree with every list position that was confirmed from presets saved by the real Spire (S01 to S05).
    {
        auto source = [](const char* name) { for (const auto& e : spire::kModulationSources) if (std::string(e.name) == name) return e.id; return -1; };
        auto target = [](const char* name) { for (const auto& e : spire::kModulationTargets) if (std::string(e.name) == name) return e.id; return -1; };
        require(spire::kModulationSourceCount == 33 && spire::kModulationTargetCount == 230, "table sizes: 33 sources, 230 targets");
        require(source("Env1") == 5 && source("LFO1") == 9 && source("Velocity") == 13 && source("KeyFollow") == 14 && source("Random_Bipolar") == 15, "source ids confirmed by saved presets (1)");
        require(source("PitchWheel") == 16 && source("ModWheel") == 17 && source("Stepper1") == 23 && source("Stepper2") == 24 && source("ArpStep") == 25, "source ids confirmed by saved presets (2)");
        require(source("Mod1") == 26 && source("Mod4") == 29 && source("Random_Unipolar") == 30 && source("Constant_10%") == 31 && source("Constant_50%") == 32, "source ids confirmed by saved presets (3)");
        bool unique = true; for (size_t i = 0; i < spire::kModulationTargetCount; ++i) for (size_t j = i + 1; j < spire::kModulationTargetCount; ++j) if (spire::kModulationTargets[i].id == spire::kModulationTargets[j].id) unique = false;
        require(unique, "every target id is unique");
        require(target("Cutoff_1") == 45 && target("Reso_1") == 46 && target("Cutoff_2") == 47 && target("Osc1_Pitch") == 5 && target("Rev_DryWet") == 143 && target("Main_Volume") == 145, "target ids match the most used targets in the library");
    }
    // A byte-order mark is accepted.
    require(decode(std::string("\xEF\xBB\xBF") + good).ok, "a UTF-8 byte-order mark is accepted");
    // A value outside 0..1 is kept and flagged, not silently clamped.
    {
        const auto r = decode(replaceFirst(good, "\"osc1_ctrlA\": 0.1", "\"osc1_ctrlA\": -5"));
        bool flagged = false; for (const auto& f : r.patch.fields) if (f.name == "osc1_ctrlA") flagged = f.outOfRange && f.value == -5;
        require(r.ok && flagged, "an out-of-range value is kept as stored and flagged");
    }

    // An absurdly large (but finite) value is kept and flagged, so one odd field does not block an otherwise good preset.
    {
        const auto r = decode(replaceFirst(good, "\"osc1_ctrlA\": 0.1", "\"osc1_ctrlA\": 1e9"));
        bool flagged = false; for (const auto& f : r.patch.fields) if (f.name == "osc1_ctrlA") flagged = f.outOfRange && f.value == 1e9;
        require(r.ok && flagged, "a huge finite value is kept and flagged");
    }

    // ---- Malformed and hostile input is blocked, never accepted, never a crash -------------------------------------------
    struct Case { std::string name; std::string text; };
    std::vector<Case> bad;
    bad.push_back({ "empty", "" });
    bad.push_back({ "whitespace", "   \n\t  " });
    bad.push_back({ "null literal", "null" });
    bad.push_back({ "array", "[]" });
    bad.push_back({ "empty object", "{}" });
    bad.push_back({ "empty parameters", "{\"parameters\":{}}" });
    bad.push_back({ "parameters is an array", "{\"parameters\":[1,2,3]}" });
    bad.push_back({ "parameters is a string", "{\"parameters\":\"abc\"}" });
    bad.push_back({ "plain text", "This is not a preset." });
    bad.push_back({ "binary with NUL", std::string("{\"parameters\":{\"a\":1}}\0\0garbage", 30) });
    bad.push_back({ "truncated in half", good.substr(0, good.size() / 2) });
    bad.push_back({ "truncated to ten bytes", good.substr(0, 10) });
    bad.push_back({ "missing closing brace", good.substr(0, good.rfind('}')) });
    bad.push_back({ "value is a string", replaceFirst(good, "\"osc1_ctrlA\": 0.1", "\"osc1_ctrlA\": \"loud\"") });
    bad.push_back({ "value is null", replaceFirst(good, "\"osc1_ctrlA\": 0.1", "\"osc1_ctrlA\": null") });
    bad.push_back({ "value is an array", replaceFirst(good, "\"osc1_ctrlA\": 0.1", "\"osc1_ctrlA\": [0.1]") });
    bad.push_back({ "value is an object", replaceFirst(good, "\"osc1_ctrlA\": 0.1", "\"osc1_ctrlA\": {\"x\":1}") });
    bad.push_back({ "value is a boolean", replaceFirst(good, "\"osc1_ctrlA\": 0.1", "\"osc1_ctrlA\": true") });
    bad.push_back({ "value overflows to infinity", replaceFirst(good, "\"osc1_ctrlA\": 0.1", "\"osc1_ctrlA\": 1e999") });
    bad.push_back({ "NaN token", replaceFirst(good, "\"osc1_ctrlA\": 0.1", "\"osc1_ctrlA\": NaN") });
    bad.push_back({ "name with a space", replaceFirst(good, "\"osc1_ctrlA\"", "\"osc1 ctrlA\"") });
    bad.push_back({ "name with a slash", replaceFirst(good, "\"osc1_ctrlA\"", "\"../osc1\"") });
    bad.push_back({ "name with a non-ASCII letter", replaceFirst(good, "\"osc1_ctrlA\"", "\"osc1_ctrl\xC3\x80\"") });
    bad.push_back({ "name longer than 64 characters", replaceFirst(good, "\"osc1_ctrlA\"", "\"" + std::string(65, 'a') + "\"") });
    bad.push_back({ "empty name", replaceFirst(good, "\"osc1_ctrlA\"", "\"\"") });
    for (const char* marker : { "osc1_wave", "flt1_cut", "env1_att", "mix_osc1" }) bad.push_back({ std::string("missing marker ") + marker, without(good, marker) });
    {   // fewer than 100 fields
        std::string small = "{\"parameters\":{\"osc1_wave\":0,\"flt1_cut\":0,\"env1_att\":0,\"mix_osc1\":0}}"; bad.push_back({ "too few fields", small });
    }
    {   // more than the field limit
        std::string many = "{\"parameters\":{\"osc1_wave\":0,\"flt1_cut\":0,\"env1_att\":0,\"mix_osc1\":0";
        for (int i = 0; i < 2100; ++i) many += ",\"f" + std::to_string(i) + "\":0.5";
        bad.push_back({ "more fields than Spire has", many + "}}" });
    }
    {   // oversized file
        std::string big = replaceFirst(good, "\"notes\": \"", "\"notes\": \"" + std::string(3 * 1024 * 1024, 'x'));
        bad.push_back({ "file larger than 2 MB", big });
    }
    bad.push_back({ "nesting bomb (arrays)", std::string(200000, '[') + std::string(200000, ']') });
    bad.push_back({ "nesting bomb (objects)", [] { std::string s; for (int i = 0; i < 100000; ++i) s += "{\"a\":"; return s + "1" + std::string(100000, '}'); }() });
    for (const auto& c : bad) { const auto r = decode(c.text); require(!r.ok && !r.blocker.empty(), "blocked with a reason: " + c.name); }
    std::cout << "INFO: " << bad.size() << " malformed or hostile inputs, each blocked with a stated reason\n";
    require(bad.size() >= 30, "at least 30 malformed cases are covered");

    // ---- Random damage never crashes the decoder -------------------------------------------------------------------------
    {
        Lcg rng; int accepted = 0, rejected = 0;
        for (int i = 0; i < 3000; ++i) {
            std::string damaged = good;
            const int edits = 1 + int(rng.next() % 6);
            for (int e = 0; e < edits; ++e) {
                const size_t at = rng.next() % damaged.size();
                switch (rng.next() % 4) {
                    case 0: damaged[at] = char(rng.next() & 0xFF); break;
                    case 1: damaged.erase(at, 1 + rng.next() % 20); break;
                    case 2: damaged.insert(at, 1, char(rng.next() & 0xFF)); break;
                    default: damaged.resize(at + 1); break;
                }
                if (damaged.empty()) damaged = "x";
            }
            const auto r = decode(damaged);
            (r.ok ? accepted : rejected)++;
        }
        std::cout << "INFO: 3000 randomly damaged copies of the fixture: " << rejected << " blocked, " << accepted << " still decoded (damage hit a harmless spot), no crash\n";
        require(rejected > 0, "random damage is usually detected");
    }

    std::cout << (failures == 0 ? "PASS" : "FAIL") << ": Spire decoder, " << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
