#include "sloked/core/URI.h"
#include "sloked/core/Locale.h"
#include <sstream>
#include <iomanip>

namespace sloked {

    constexpr char DigitToHex(uint8_t digit) {
        if (digit < 10) {
            return '0' + digit;
        } else switch (digit) {
            case 10:
                return 'A';
            
            case 11:
                return 'B';

            case 12:
                return 'C';

            case 13:
                return 'D';

            case 14:
                return 'E';

            case 15:
                return 'F';

            default:
                return '.';
        }
    }

    std::string SlokedUri::encodeComponent(std::string_view input) {
        std::stringstream ss;
        SlokedLocale::SystemEncoding().IterateCodepoints(input, [&](auto position, auto length, auto chr) {
            auto content = input.substr(position, length);
            switch (chr) {
                // Unreserved uppercase Ucharacters
                case U'A': case U'B': case U'C': case U'D':
                case U'E': case U'F': case U'G': case U'H':
                case U'I': case U'J': case U'K': case U'L':
                case U'M': case U'N': case U'O': case U'P':
                case U'Q': case U'R': case U'S': case U'T':
                case U'U': case U'V': case U'W': case U'X':
                case U'Y': case U'Z':
                // Unreserved lowercase Ucharacters
                case U'a': case U'b': case U'c': case U'd':
                case U'e': case U'f': case U'g': case U'h':
                case U'i': case U'j': case U'k': case U'l':
                case U'm': case U'n': case U'o': case U'p':
                case U'q': case U'r': case U's': case U't':
                case U'u': case U'v': case U'w': case U'x':
                case U'y': case U'z':
                // Numbers
                case U'0': case U'1': case U'2': case U'3':
                case U'4': case U'5': case U'6': case U'7':
                case U'8': case U'9':
                // Other
                case U'-': case U'_': case U'.': case U'~':
                    ss << content;
                    break;

                default:
                    for (auto b : content) {
                        ss << '%' << DigitToHex((b & 0xf0) >> 4) << DigitToHex(b & 0xf);
                    }
                    break;
            }
            return true;
        });
        return ss.str();
    }
}