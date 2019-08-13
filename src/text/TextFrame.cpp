#include "sloked/text/TextFrame.h"
#include <iostream>
#include <sstream>

namespace sloked {

    TextFrameView::TextFrameView(const TextBlockView &text, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : text(text), encoding(encoding), charWidth(charWidth) {}

    std::size_t TextFrameView::TextFrameView::GetLastLine() const {
        return this->content.size();
    }

    std::size_t TextFrameView::GetTotalLength() const {
        std::size_t length = 0;
        for (const auto &line : this->content) {
            length += line.size() + 1;
        }
        if (length > 0) {
            length--;
        }
        return length;
    }

    std::string_view TextFrameView::GetLine(std::size_t idx) const {
        if (idx < this->content.size()) {
            return this->content.at(idx);
        } else {
            return "";
        }
    }

    bool TextFrameView::Empty() const {
        return this->content.empty();
    }

    void TextFrameView::Update(const TextPosition &dim, const TextPosition &cursor) {
        if (this->offset.line + dim.line - 1 < cursor.line) {
            this->offset.line = cursor.line - dim.line + 1;
        }
        if (cursor.line < offset.line) {
            this->offset.line = cursor.line;
        }

        auto realColumn = this->charWidth.GetRealPosition(std::string {this->text.GetLine(cursor.line)}, cursor.column, this->encoding);
        if (this->offset.column + dim.column - 1 < realColumn) {
            this->offset.column = realColumn - dim.column + 1;
        }
        if (realColumn < this->offset.column) {
            this->offset.column = realColumn;
        }

        this->content.clear();
        this->text.Visit(this->offset.line, std::min(dim.line, static_cast<TextPosition::Line>(this->text.GetLastLine() - this->offset.line) + 1), [&](const auto line) {
            std::stringstream ss;
            this->encoding.IterateCodepoints(line, [&](auto start, auto length, auto chr) {
                if (chr != u'\t') {
                    ss << line.substr(start, length);
                } else {
                    ss << this->charWidth.GetTab();
                }
                return true;
            });
            auto offsetLine = this->encoding.GetCodepoint(ss.str(), this->offset.column);
            if (offsetLine.second != 0) {
                this->content.push_back(ss.str().substr(offsetLine.first));
            } else {
                this->content.push_back("");
            }
        });
    }

    const TextPosition &TextFrameView::GetOffset() const {
        return this->offset;
    }

    std::ostream &TextFrameView::dump(std::ostream &os) const {
        for (std::size_t i = 0; i < this->content.size(); i++) {
            os << this->content.at(i);
            if (i + 1 < this->content.size()) {
                os << std::endl;
            }
        }
        return os;
    }
}