#ifndef UNS1_DECODE_H
#define UNS1_DECODE_H

#include "fmc-aircraft-profile.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

class ProductFMC;

// Shared decoding for aircraft that expose a Universal Avionics UNS-1 style CDU
// (the "text_line_N" / "style_line_N" string + style-byte dataref pairs). These
// helpers hold the parts that are identical across every UNS-1 integration — the
// symbol tables, the per-line screen decode, the special-character glyph mapping,
// and the style-byte colour table — so each aircraft profile stays standalone and
// only supplies its own dataref namespace.
namespace UNS1Decode {
    // Decode `lineCount` text/style dataref pairs under `displayPrefix`
    // (e.g. "FJS/Q4XP/cdu1" or "uns1/cdu1") into `page`, resolving unicode and
    // high-byte symbols to the plugin's placeholder glyphs and writing each cell
    // via the product.
    void updatePage(ProductFMC *product, std::vector<std::vector<char>> &page, const std::string &displayPrefix, int lineCount);

    // Translate a decoded placeholder byte into the UTF-8 glyph bytes the display
    // expects, appending to `buffer`.
    void mapCharacter(std::vector<uint8_t> *buffer, uint8_t character);

    // Style-byte (low nibble, or 0x40 for inverted) -> text colour table.
    const std::map<char, FMCTextColor> &colorMap();
}

#endif // UNS1_DECODE_H
