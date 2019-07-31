#include "sloked/text/cursor/PlainCursor.h"
#include "sloked/text/cursor/EditingPrimitives.h"
#include <cassert>
#include <iostream>

namespace sloked {

    PlainCursor::PlainCursor(TextBlock &text, const Encoding &encoding)
        : text(text), encoding(encoding), line(0), column(0) {
        this->UpdateCursor();
    }

    TextPosition::Line PlainCursor::GetLine() const {
        return this->line;
    }

    TextPosition::Column PlainCursor::GetColumn() const {
        return this->column;
    }

    void PlainCursor::SetPosition(Line l, Column c) {
        if (l <= this->text.GetLastLine()) {
            auto columns = this->encoding.CodepointCount(this->text.GetLine(l));
            if (c <= columns) {
                this->line = l;
                this->column = c;
                this->UpdateCursor();
            }
        }
    }

    void PlainCursor::MoveUp(TextPosition::Line l) {
        this->line -= std::min(l, this->line);
        this->UpdateCursor();
        this->column = std::min(this->column, this->width);
    }

    void PlainCursor::MoveDown(TextPosition::Line l) {
        this->line += std::min(this->line + l, static_cast<TextPosition::Line>(this->text.GetLastLine())) - this->line;
        this->UpdateCursor();
        this->column = std::min(this->column, this->width);
    }

    void PlainCursor::MoveForward(TextPosition::Column c) {
        this->column = std::min(this->column + c, this->width);
        this->UpdateCursor();
    }

    void PlainCursor::MoveBackward(TextPosition::Column c) {
        this->column -= std::min(c, this->column);
        this->UpdateCursor();
    }

    void PlainCursor::Insert(std::string_view frag) {
        auto pos = SlokedEditingPrimitives::Insert(this->text, this->encoding, TextPosition{this->line, this->column}, frag);
        this->column = pos.column;
        this->UpdateCursor();
    }

    void PlainCursor::NewLine(std::string_view content) {
        auto pos = SlokedEditingPrimitives::Newline(this->text, this->encoding, TextPosition{this->line, this->column}, content);
        this->line = pos.line;
        this->column = pos.column;
        this->UpdateCursor();
    }

    void PlainCursor::DeleteBackward() {
        auto pos = SlokedEditingPrimitives::DeleteBackward(this->text, this->encoding, TextPosition{this->line, this->column});
        this->line = pos.line;
        this->column = pos.column;
        this->UpdateCursor();
    }

    void PlainCursor::DeleteForward() {
        SlokedEditingPrimitives::DeleteForward(this->text, this->encoding, TextPosition{this->line, this->column});
        this->UpdateCursor();
    }

    void PlainCursor::ClearRegion(const TextPosition &from, const TextPosition &to) {
        SlokedEditingPrimitives::ClearRegion(this->text, this->encoding, from, to);
        this->UpdateCursor();
    }

    std::vector<std::string> PlainCursor::Read(const TextPosition &from, const TextPosition &to) const {
        std::vector<std::string> result;
        if (from.line == to.line) {
            if (from.line <= this->text.GetLastLine()) {
                std::string line {this->text.GetLine(from.line)};
                std::size_t from_offset = SlokedEditingPrimitives::GetOffset(line, from.column, this->encoding);
                std::size_t to_offset = SlokedEditingPrimitives::GetOffset(line, to.column, this->encoding);
                result.push_back(line.substr(from_offset, to_offset - from_offset));
            }
            return result;
        }

        Line last_line = std::min(to.line, static_cast<Line>(this->text.GetLastLine()));
        for (Line line = from.line; line <= last_line; line++) {
            if (line == from.line) {
                auto first_line = this->text.GetLine(line);
                std::size_t from_offset = SlokedEditingPrimitives::GetOffset(first_line, from.column, this->encoding);
                result.push_back(std::string {first_line.substr(from_offset, first_line.size() - from_offset)});
            } else if (line == to.line) {
                auto last_line = this->text.GetLine(line);
                std::size_t to_offset = SlokedEditingPrimitives::GetOffset(last_line, to.column, this->encoding);
                result.push_back(std::string {last_line.substr(0, to_offset)});
            } else {
                result.push_back(std::string {this->text.GetLine(line)});
            }
        }
        return result;
    }

    TextPosition::Column PlainCursor::LineLength(Line l) const {
        if (l > this->text.GetLastLine()) {
            return 0;
        } else {
            return this->encoding.CodepointCount(this->text.GetLine(l));
        }
    }

    TextPosition::Line PlainCursor::LineCount() const {
        return this->text.GetLastLine() + 1;
    }

    void PlainCursor::UpdateCursor() {
        if (this->line <= this->text.GetLastLine()) {
            this->width = this->encoding.CodepointCount(this->text.GetLine(this->line));
        } else {
            this->width = 0;
        }
    }
}