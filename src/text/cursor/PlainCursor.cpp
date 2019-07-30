#include "sloked/text/cursor/PlainCursor.h"
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
        std::string current {this->text.GetLine(this->line)};
        if (this->column < this->width) {
            auto pos = this->encoding.GetCodepoint(current, this->column);
            current.insert(pos.first, frag);
        } else {
            current.append(frag);
        }
        this->column += this->encoding.CodepointCount(frag);
        this->text.SetLine(this->line, current);
        this->UpdateCursor();
    }

    void PlainCursor::NewLine(std::string_view content) {
        std::string current {this->text.GetLine(this->line)};
        if (this->column < this->width) {
            auto pos = this->encoding.GetCodepoint(current, this->column);
            auto sub1 = current.substr(0, pos.first);
            auto sub2 = current.substr(pos.first);

            text.SetLine(line, sub1);
            if (content.empty()) {
                text.InsertLine(line, sub2);
            } else {
                std::string str {content};
                str.append(sub2);
                text.InsertLine(line, str);
            }
        } else {
            text.InsertLine(line, content);
        }
        this->line++;
        this->column = 0;
        this->UpdateCursor();
    }

    void PlainCursor::DeleteBackward() {
        if (this->column > 0) {
            if (this->column < this->width) {
                std::string current {this->text.GetLine(this->line)};
                auto pos1 = this->encoding.GetCodepoint(current, this->column - 1);
                auto pos2 = this->encoding.GetCodepoint(current, this->column);
                auto sub1 = current.substr(0, pos1.first);
                auto sub2 = current.substr(pos2.first);

                text.SetLine(this->line, sub1 + sub2);
            } else {
                auto current = this->text.GetLine(this->line);
                auto pos = this->encoding.GetCodepoint(current, this->column - 1);
                text.SetLine(this->line, current.substr(0, pos.first));
            }
            this->column--;
            this->UpdateCursor();
        } else if (this->line > 0) {
            std::string ln1{text.GetLine(this->line - 1)};
            std::string ln2{text.GetLine(this->line)};
            text.SetLine(this->line - 1, ln1 + ln2);
            text.EraseLine(this->line);
            this->line--;

            this->column = this->encoding.CodepointCount(ln1);
            this->UpdateCursor();
        }
    }

    void PlainCursor::DeleteForward() {
        if (this->column + 1 < this->width) {
            std::string current {this->text.GetLine(this->line)};
            auto pos1 = this->encoding.GetCodepoint(current, this->column);
            auto pos2 = this->encoding.GetCodepoint(current, this->column + 1);
            auto sub1 = current.substr(0, pos1.first);
            auto sub2 = current.substr(pos2.first);

            text.SetLine(this->line, sub1 + sub2);
            this->UpdateCursor();
        } else if (this->column + 1 == this->width) {
            std::string current {this->text.GetLine(this->line)};
            auto pos = this->encoding.GetCodepoint(current, this->column);
            auto sub = current.substr(0, pos.first);
            text.SetLine(this->line, sub);
            this->UpdateCursor();
        } else if (this->column == this->width && this->line < this->text.GetLastLine()) {
            std::string ln1{text.GetLine(this->line)};
            std::string ln2{text.GetLine(this->line + 1)};
            text.SetLine(this->line, ln1 + ln2);
            text.EraseLine(this->line + 1);
            this->UpdateCursor();
        }
    }

    static std::size_t get_position(std::string_view str, TextPosition::Column position, const Encoding &encoding) {
        std::size_t total_length = encoding.CodepointCount(str);
        if (position < total_length) {
            return encoding.GetCodepoint(str, position).first;
        } else {
            return str.size();
        }
    }

    void PlainCursor::ClearRegion(const TextPosition &from, const TextPosition &to) {
        if (!(from < to)) {
            return;
        }
        if (from.line == to.line) {
            if (from.line <= this->text.GetLastLine()) {
                std::string line {this->text.GetLine(from.line)};
                std::size_t from_offset = get_position(line, from.column, this->encoding);
                std::size_t to_offset = get_position(line, to.column, this->encoding);
                this->text.SetLine(from.line, line.substr(0, from_offset) + line.substr(to_offset, line.size() - to_offset));
            }
            this->UpdateCursor();
            return;
        }
        
        if (from.line <= this->text.GetLastLine()) {
            std::string first_line {this->text.GetLine(from.line)};
            std::string last_line = "";
            if (to.line <= this->text.GetLastLine()) {
                last_line = this->text.GetLine(to.line);
                this->text.EraseLine(to.line);
            }
            std::size_t from_offset = get_position(first_line, from.column, this->encoding);
            std::size_t to_offset = get_position(last_line, to.column, this->encoding);
            this->text.SetLine(from.line, first_line.substr(0, from_offset) + last_line.substr(to_offset, last_line.size() - to_offset));
        }

        for (std::size_t line = to.line - 1; line >= from.line + 1; line--) {
            if (line <= this->text.GetLastLine()) {
                this->text.EraseLine(line);
            }
        }
        this->UpdateCursor();
    }

    std::vector<std::string> PlainCursor::Read(const TextPosition &from, const TextPosition &to) const {
        std::vector<std::string> result;
        if (from.line == to.line) {
            if (from.line <= this->text.GetLastLine()) {
                std::string line {this->text.GetLine(from.line)};
                std::size_t from_offset = get_position(line, from.column, this->encoding);
                std::size_t to_offset = get_position(line, to.column, this->encoding);
                result.push_back(line.substr(from_offset, to_offset - from_offset));
            }
            return result;
        }

        Line last_line = std::min(to.line, static_cast<Line>(this->text.GetLastLine()));
        for (Line line = from.line; line <= last_line; line++) {
            if (line == from.line) {
                auto first_line = this->text.GetLine(line);
                std::size_t from_offset = get_position(first_line, from.column, this->encoding);
                result.push_back(std::string {first_line.substr(from_offset, first_line.size() - from_offset)});
            } else if (line == to.line) {
                auto last_line = this->text.GetLine(line);
                std::size_t to_offset = get_position(last_line, to.column, this->encoding);
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