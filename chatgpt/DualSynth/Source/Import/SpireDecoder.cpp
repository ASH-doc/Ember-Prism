#include "SpireDecoder.h"
#include <juce_core/juce_core.h>
#include <cmath>

namespace aether::import {
namespace {

DecodeResult blocked(const std::string& why) { DecodeResult r; r.ok = false; r.blocker = why; return r; }

std::string groupOf(const std::string& name) {
    const auto underscore = name.find('_');
    if (underscore == std::string::npos || underscore == 0) return "global";
    const std::string prefix = name.substr(0, underscore);
    // Groups are the section prefixes: osc1, flt1, env3, lfo2, slot12, stp1, arp, mix, chr, del, rev, phs, shp, eq ...
    return prefix;
}

bool validName(const juce::String& name) {
    if (name.isEmpty() || size_t(name.length()) > kSpireMaxNameLength) return false;
    for (auto c : name) if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;   // ASCII only
    return true;
}

// The markers a real Spire preset always carries. A JSON file without them is some other document.
const char* const kMarkers[] = { "osc1_wave", "flt1_cut", "env1_att", "mix_osc1" };

} // namespace

bool spireOnEnumLattice(double v) {
    // Values are stored with about six significant digits (2 / 999 is written 0.002002), hence the tolerance.
    if (!std::isfinite(v) || v < -1.0e-9 || v > 1.0 + 1.0e-9) return false;
    const double scaled = v * 999.0;
    return std::abs(scaled - std::round(scaled)) < 0.005;
}

int spireEnumIndex(double storedValue) {
    return spireOnEnumLattice(storedValue) ? static_cast<int>(std::round(storedValue * 999.0)) : -1;
}

DecodeResult decodeSpire(const void* data, size_t size, const std::string& fileName) {
    if (data == nullptr || size == 0) return blocked("The file is empty.");
    if (size > kSpireMaxFileBytes) return blocked("The file is larger than 2 MB, which is far beyond a Spire preset.");

    // Spire files are UTF-8 text; skip a byte-order mark if one is present.
    const char* text = static_cast<const char*>(data);
    size_t length = size;
    if (length >= 3 && static_cast<unsigned char>(text[0]) == 0xEF && static_cast<unsigned char>(text[1]) == 0xBB && static_cast<unsigned char>(text[2]) == 0xBF) { text += 3; length -= 3; }
    for (size_t i = 0; i < length; ++i) if (text[i] == '\0') return blocked("The file contains binary data, not a Spire .spf2 text preset.");

    // The JSON reader recurses per nesting level, so refuse deep nesting before it gets the text (a 2 MB file could
    // otherwise hold a million nested brackets). Real presets are 3 levels deep.
    {
        int depth = 0; bool inString = false, escaped = false;
        for (size_t i = 0; i < length; ++i) {
            const char c = text[i];
            if (inString) { if (escaped) escaped = false; else if (c == '\\') escaped = true; else if (c == '"') inString = false; continue; }
            if (c == '"') inString = true;
            else if (c == '{' || c == '[') { if (++depth > kSpireMaxNesting) return blocked("The file is nested far deeper than any Spire preset."); }
            else if (c == '}' || c == ']') --depth;
        }
    }
    juce::var parsed;
    if (juce::JSON::parse(juce::String::fromUTF8(text, int(length)), parsed).failed()) return blocked("The file is not valid JSON.");
    auto* root = parsed.getDynamicObject();
    if (root == nullptr) return blocked("The file is JSON but not a Spire preset (the top level is not an object).");
    auto* parameters = root->getProperty("parameters").getDynamicObject();
    if (parameters == nullptr) return blocked("The file has no \"parameters\" block, so it is not a Spire .spf2 preset.");
    const auto& properties = parameters->getProperties();
    if (size_t(properties.size()) > kSpireMaxFields) return blocked("The file has more parameter fields than Spire presets have.");
    if (properties.size() < 100) return blocked("The file has too few parameter fields to be a Spire preset.");

    DecodeResult result;
    auto& patch = result.patch;
    patch.format = SourceFormat::SpireSpf2;
    patch.name = fileName;
    patch.fields.reserve(size_t(properties.size()));

    for (const auto& property : properties) {
        const juce::String name = property.name.toString();
        if (!validName(name)) return blocked("A parameter has an invalid name, so the file is not trusted.");
        const juce::var& value = property.value;
        if (!(value.isDouble() || value.isInt() || value.isInt64())) return blocked("Parameter \"" + name.toStdString() + "\" is not a number.");
        const double number = double(value);
        if (!std::isfinite(number)) return blocked("Parameter \"" + name.toStdString() + "\" is not a finite number.");
        // A value far outside 0..1 is kept and flagged (two real files in the local library carry one in the field
        // "soft"); translators must treat a flagged field as having no usable value and say so in the report.
        DecodedField field;
        field.name = name.toStdString();
        field.group = groupOf(field.name);
        field.value = number;
        field.outOfRange = number < -1.0e-3 || number > 1.0 + 1.0e-3;
        if (field.outOfRange) patch.warnings.push_back("Parameter \"" + field.name + "\" is outside 0..1 and was kept as stored.");
        patch.fields.push_back(std::move(field));
    }

    for (const char* marker : kMarkers)
        if (!parameters->hasProperty(juce::Identifier(marker))) return blocked("The file lacks the fields every Spire preset has (\"" + std::string(marker) + "\"), so it is not a Spire preset.");

    // Metadata (all optional, all length-limited; the embedded icon image is ignored).
    auto text255 = [&](const char* key) { return root->getProperty(key).toString().substring(0, 255).toStdString(); };
    patch.author = text255("author"); patch.vendor = text255("vendor"); patch.bank = text255("bank");
    patch.notes = root->getProperty("notes").toString().substring(0, 2000).toStdString();
    const juce::var tags = root->getProperty("tags");
    if (auto* list = tags.getArray()) { for (int i = 0; i < std::min(list->size(), 64); ++i) patch.tags.push_back(list->getReference(i).toString().substring(0, 64).toStdString()); }
    else if (tags.isString()) patch.tags.push_back(tags.toString().substring(0, 64).toStdString());
    if (root->hasProperty("icon")) patch.warnings.push_back("The embedded icon image was ignored.");

    result.ok = true;
    return result;
}

} // namespace aether::import
