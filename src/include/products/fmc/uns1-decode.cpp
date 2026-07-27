#include "uns1-decode.h"

#include "dataref.h"
#include "product-fmc.h"

#include <utility>

void UNS1Decode::updatePage(ProductFMC *product, std::vector<std::vector<char>> &page, const std::string &displayPrefix, int lineCount) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();

    // Replace unicode symbols with single-byte placeholders
    const std::vector<std::pair<std::string, unsigned char>> symbols = {
        {"←", '<'},  {"→", '>'},  {"↑", 30},  {"↓", 31},
        {"☐", '#'},  {"°", '`'},  {"Δ", '^'},
        {"↔", '<'},  {"↖", '<'},  {"↗", '>'},  {"↘", '>'},  {"↙", '<'},
        {"⇦", '<'},  {"⇨", '>'},  {"⇧", 30},  {"⇩", 31},
        {"─", '-'},  {"│", '|'},  {"┌", '+'}, {"┐", '+'}, {"└", '+'}, {"┘", '+'},
        {"├", '|'},  {"┤", '|'},  {"┬", '+'}, {"┴", '+'}, {"┼", '+'},
        {"═", '='},  {"║", '|'},  {"╔", '+'}, {"╗", '+'}, {"╚", '+'}, {"╝", '+'},
        {"╠", '+'},  {"╣", '+'},  {"╦", '+'}, {"╩", '+'}, {"╬", '+'},
        {"╭", '|'},  {"╮", '|'},  {"╯", '|'},  {"╰", '|'},
        {"⎡", '+'},  {"⎤", '+'},  {"⎧", '{'},  {"⎫", '}'},
        {"⟦", '['},  {"⟧", ']'},
    };

    for (int lineNum = 0; lineNum < lineCount; ++lineNum) {
        std::string textDataref = displayPrefix + "/text_line_" + std::to_string(lineNum);
        std::string styleDataref = displayPrefix + "/style_line_" + std::to_string(lineNum);

        std::string text = datarefManager->getCached<std::string>(textDataref.c_str());
        if (text.empty()) {
            continue;
        }

        std::vector<unsigned char> styleBytes = datarefManager->getCached<std::vector<unsigned char>>(styleDataref.c_str());

        for (const auto &symbol : symbols) {
            size_t pos = 0;
            while ((pos = text.find(symbol.first, pos)) != std::string::npos) {
                text.replace(pos, symbol.first.length(), std::string(1, static_cast<char>(symbol.second)));
                pos += 1;
            }
        }

        // Map remaining high-byte characters to ASCII equivalents
        for (size_t i = 0; i < text.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(text[i]);
            if (c <= 127) { continue; }
            switch (c) {
                case 0xB0: case 0xB1: case 0xB2: text[i] = ' '; break;
                case 0xB3: case 0xDD: case 0xDE: text[i] = '|'; break;
                case 0xC4: case 0xDC: case 0xDF: text[i] = '-'; break;
                case 0xBA:                     text[i] = '|'; break;
                case 0xCD:                     text[i] = '='; break;
                case 0xBF: case 0xC0: case 0xD9: case 0xDA:
                case 0xB4: case 0xC3: case 0xC1: case 0xC2: case 0xC5:
                case 0xB5: case 0xB6: case 0xB7: case 0xB8: case 0xB9:
                case 0xBB: case 0xBC: case 0xBD: case 0xBE:
                case 0xC6: case 0xC7: case 0xC8: case 0xC9:
                case 0xCA: case 0xCB: case 0xCC: case 0xCE: case 0xCF:
                case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4:
                case 0xD5: case 0xD6: case 0xD7: case 0xD8:
                    text[i] = '+'; break;
                case 0xDB: text[i] = '#'; break;
                default:   text[i] = ' '; break;
            }
        }

        int displayLine = lineNum;
        if (displayLine >= ProductFMC::PageLines) {
            break;
        }

        for (int i = 0; i < text.size() && i < ProductFMC::PageCharsPerLine; ++i) {
            char c = text[i];
            if (c == 0x00) {
                continue;
            }

            bool fontSmall = false;
            unsigned char styleByte = (i < styleBytes.size()) ? styleBytes[i] : 0x00;
            fontSmall = (styleByte & 0xF0) == 0x00;

            bool isInverted = (styleByte & 0x40);
            if (c == ' ' && !isInverted) {
                continue;
            }

            unsigned char colorIdx = styleByte & 0x0F;
            if (styleByte & 0x40) {
                colorIdx = 0x40;  // inverted/reverse video
            }

            product->writeLineToPage(page, displayLine, i, std::string(1, c), colorIdx, fontSmall);
        }
    }
}

void UNS1Decode::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character) {
    switch (character) {
        case '#':
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case '<':
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_LEFT.begin(), FMCSpecialCharacter::ARROW_LEFT.end());
            break;

        case '>':
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_RIGHT.begin(), FMCSpecialCharacter::ARROW_RIGHT.end());
            break;

        case 30:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_UP.begin(), FMCSpecialCharacter::ARROW_UP.end());
            break;

        case 31:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_DOWN.begin(), FMCSpecialCharacter::ARROW_DOWN.end());
            break;

        case '`':
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        case '^':
            buffer->insert(buffer->end(), FMCSpecialCharacter::TRIANGLE.begin(), FMCSpecialCharacter::TRIANGLE.end());
            break;

        default:
            buffer->push_back(character);
            break;
    }
}

const std::map<char, FMCTextColor> &UNS1Decode::colorMap() {
    static const std::map<char, FMCTextColor> colMap = {
        {0x00, FMCTextColor::COLOR_WHITE},
        {0x01, FMCTextColor::COLOR_WHITE},
        {0x02, FMCTextColor::COLOR_GREEN},
        {0x03, FMCTextColor::COLOR_YELLOW},
        {0x04, FMCTextColor::COLOR_GREEN},
        {0x05, FMCTextColor::COLOR_MAGENTA},
        {0x06, FMCTextColor::COLOR_GREEN},
        {0x07, FMCTextColor::COLOR_CYAN},
        {0x0B, FMCTextColor::COLOR_GREEN},
        {0x40, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_WHITE)},  // inverted (ACCEPT)
    };
    return colMap;
}
