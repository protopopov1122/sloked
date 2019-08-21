#include "sloked/text/TextFrame.h"
#include <iostream>
#include <sstream>

namespace sloked {

    TextFrameView::TextFrameView(const TextBlockView &text, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : text(text), encoding(encoding), charWidth(charWidth), offset{0, 0}, size{0, 0}, buffer{} {}

    std::size_t TextFrameView::TextFrameView::GetLastLine() const {
        return std::min(static_cast<std::size_t>(this->offset.line + this->size.line), this->text.GetLastLine()) - this->offset.line;
    }

    std::size_t TextFrameView::GetTotalLength() const {
        std::size_t length = 0;
        this->VisitLines([&](auto line, auto content) {
            length += content.size() + 1;
        });
        if (length > 0) {
            length--;
        }
        return length;
    }

    std::string_view TextFrameView::GetLine(std::size_t idx) const {
        if (this->offset.line + idx <= this->text.GetLastLine() && idx <= this->size.line) {
            std::string line = this->PreprocessLine(this->text.GetLine(this->offset.line + idx));
            this->buffer[idx] = line;
            return this->buffer[idx];
        } else {
            return "";
        }
    }

    bool TextFrameView::Empty() const {
        return this->GetTotalLength() == 0;
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
        this->size = dim;
        this->buffer.clear();
        this->buffer.insert(this->buffer.end(), this->size.line, "");
    }

    const TextPosition &TextFrameView::GetOffset() const {
        return this->offset;
    }

    std::ostream &TextFrameView::dump(std::ostream &os) const {
        auto lastLine = this->GetLastLine();
        this->VisitLines([&](auto line, auto content) {
            os << content;
            if (line < lastLine) {
                os << std::endl;
            }
        });
        return os;
    }

    void TextFrameView::VisitLines(std::function<void(std::size_t, std::string_view)> callback) const {
        std::size_t lineId = 0;
        this->text.Visit(this->offset.line, std::min(this->size.line, static_cast<TextPosition::Line>(this->text.GetLastLine() - this->offset.line) + 1), [&](const auto line) {
            callback(lineId++, this->PreprocessLine(line));
        });
    }

    std::string TextFrameView::PreprocessLine(std::string_view line) const {
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
            return ss.str().substr(offsetLine.first);
        } else {
            return "";
        }
    }
}