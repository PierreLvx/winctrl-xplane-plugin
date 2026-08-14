#ifndef SEGMENT_DISPLAY_H
#define SEGMENT_DISPLAY_H

#include <cstdint>
#include <string>
#include <vector>

namespace SegmentDisplay {

    // Get 7-segment representation for a character
    uint8_t getSegmentRepresentation(char c);

    uint8_t getSegmentMask(char c);

    // Basic 7-segment string encoding (right-aligned)
    std::vector<uint8_t> encodeString(int numSegments, const std::string &str);

    // Swapped nibble encoding (for some displays)
    std::vector<uint8_t> encodeStringSwapped(int numSegments, const std::string &str);

    // EFIS-specific bit mapping
    std::vector<uint8_t> encodeStringEfis(int numSegments, const std::string &str);

    // Fix string length with leading zeros
    std::string fixStringLength(const std::string &value, int length, char fillChar = '0');

    // Swap nibbles in a byte
    uint8_t swapNibbles(uint8_t value);

    // AGP 2-byte per digit encoding (little-endian format)
    std::vector<uint8_t> encodeStringAGP(int numSegments, const std::string &str);

    // Dot/colon placement scheme used when parsing display text
    enum class DotPlacement {
        // Dot attaches to the digit right before the separator (e.g. RMP frequency decimal point)
        PrecedingDigit,
        // Colon renders as two dots around the separator position (AGP clock displays)
        DualDot,
    };

    // Parse display text into fixed-width digits plus a dot-position bitmask.
    // Separators ('.' and ':') are removed from the digit stream and recorded as dot bits.
    // Digits are right-aligned (left-padded with spaces) to expectedLength and appended to outDigits.
    // The local dot bits are shifted by the padding amount and digitOffset before being OR-ed into dotMask.
    // slashReplacement, when not '\0', substitutes '/' characters in the digit stream.
    void parseSegmentText(const std::string &text, int expectedLength, std::string &outDigits, uint16_t &dotMask, int digitOffset, DotPlacement dotPlacement, char slashReplacement = '\0');

    // Encode digits (and optional dots) into a packet using bitplane row offsets.
    // segmentRowOffsets[seg] gives the packet byte offset holding segment bit 'seg' for digit 0;
    // digits beyond the first 8 continue into subsequent bytes (offset + digit / 8, bit digit % 8).
    // dotRowOffset is the base packet byte offset for dot/colon bits; pass -1 to disable dots.
    void encodeBitplane(std::vector<uint8_t> &packet, const std::string &digits, uint16_t dotMask, const int *segmentRowOffsets, int numSegmentRows, int dotRowOffset);

}

#endif
