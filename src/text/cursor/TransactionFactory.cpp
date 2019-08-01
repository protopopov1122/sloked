#include "sloked/text/cursor/TransactionFactory.h"
#include "sloked/text/cursor/EditingPrimitives.h"

namespace sloked {

    SlokedCursorTransaction SlokedTransactionFactory::Insert(const TextPosition &position, std::string_view text) {
        return SlokedCursorTransaction {
            SlokedCursorTransaction::Action::Insert,
            SlokedCursorTransaction::Content {
                position,
                std::string {text}
            }
        };
    }

    SlokedCursorTransaction SlokedTransactionFactory::Newline(const TextPosition &position, std::string_view text) {
        return SlokedCursorTransaction {
            SlokedCursorTransaction::Action::Newline,
            SlokedCursorTransaction::Content {
                position,
                std::string {text}
            }
        };
    }

    SlokedCursorTransaction SlokedTransactionFactory::DeleteBackward(const TextBlock &text, const Encoding &encoding, const TextPosition &position) {
        std::string content = "";
        TextPosition::Column width = 0;
        if (position.column > 0) {
            std::string_view view = text.GetLine(position.line);
            auto pos = encoding.GetCodepoint(view, position.column - 1);
            content = view.substr(pos.first, pos.second);
        } else {
            width = encoding.CodepointCount(text.GetLine(position.line - 1));
        }
        return SlokedCursorTransaction {
            SlokedCursorTransaction::Action::DeleteBackward,
            SlokedCursorTransaction::DeletePosition {
                position,
                content,
                width
            }
        };
    }

    SlokedCursorTransaction SlokedTransactionFactory::DeleteForward(const TextBlock &text, const Encoding &encoding, const TextPosition &position) {
        std::string content = "";
        TextPosition::Column width = encoding.CodepointCount(text.GetLine(position.line));
        if (position.column < width) {
            std::string_view view = text.GetLine(position.line);
            auto pos = encoding.GetCodepoint(view, position.column);
            content = view.substr(pos.first, pos.second);
        }
        return SlokedCursorTransaction {
            SlokedCursorTransaction::Action::DeleteForward,
            SlokedCursorTransaction::DeletePosition {
                position,
                content,
                width
            }
        };
    }

    static TextPosition ClampPosition(const TextBlock &text, const Encoding &encoding, const TextPosition &position) {
        TextPosition res;
        res.line = std::min(position.line, static_cast<TextPosition::Line>(text.GetLastLine()));
        res.column = std::min(position.column, static_cast<TextPosition::Column>(encoding.CodepointCount(text.GetLine(res.line))));
        return res;
    }

    SlokedCursorTransaction SlokedTransactionFactory::ClearRegion(const TextBlock &text, const Encoding &encoding, const TextPosition &from, const TextPosition &to) {
        auto cfrom = ClampPosition(text, encoding, from);
        auto cto = ClampPosition(text, encoding, to);
        return SlokedCursorTransaction {
            SlokedCursorTransaction::Range {
                cfrom,
                cto,
                SlokedEditingPrimitives::Read(text, encoding, cfrom, cto)
            }
        };
    }
}