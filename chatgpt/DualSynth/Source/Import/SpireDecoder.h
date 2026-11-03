#pragma once
// SpireDecoder.h — Reads a Spire 1.5 `.spf2` preset into a DecodedPatch. The file is plain JSON; see docs/import/.
#include "ImportTypes.h"

namespace aether::import {

// Hard limits, so a hostile or damaged file cannot make the reader allocate without bound.
constexpr size_t kSpireMaxFileBytes = 2u * 1024u * 1024u;   // real files are about 62 KB
constexpr size_t kSpireMaxFields = 2000;                    // real files hold 722
constexpr size_t kSpireMaxNameLength = 64;
constexpr int kSpireMaxNesting = 16;                        // real files nest 3 levels

// `fileName` (without folder) is used as the patch name because the file itself carries none.
DecodeResult decodeSpire(const void* data, size_t size, const std::string& fileName);

// Spire stores list choices as index / 999 (observed in every list-like field of the local library, 72,084 files).
inline int spireEnumIndex(double storedValue) { return static_cast<int>(storedValue * 999.0 + 0.5); }
// True when the stored value sits on the index / 999 lattice (a quick check that a field is a list choice).
bool spireOnEnumLattice(double storedValue);

} // namespace aether::import
