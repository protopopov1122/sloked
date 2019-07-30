#include "sloked/text/cursor/TransactionCursor.h"
#include <functional>
#include <iostream>

namespace sloked {

    class TransactionCursor::Command {
     public:
        Command(SlokedCursor &cursor, const SlokedTextReader &reader)
            : cursor(cursor), reader(reader) {}
        virtual ~Command() = default;

        virtual void apply() = 0;
        virtual void undo() = 0;
    
     protected:
        SlokedCursor &cursor;
        const SlokedTextReader &reader;
    };

    class PositioningCommand : public TransactionCursor::Command {
     public:
        PositioningCommand(SlokedCursor &cursor, const SlokedTextReader &reader, std::function<void(SlokedCursor &)> action)
            : Command(cursor, reader), action(action) {
            this->previousPosition = TextPosition {this->cursor.GetLine(), this->cursor.GetColumn()};
        }

        void apply() override {
            this->action(this->cursor);
        }
        
        void undo() override {
            this->cursor.SetPosition(this->previousPosition.line, this->previousPosition.column);
        }

     private:
        TextPosition previousPosition;
        std::function<void(SlokedCursor &)> action;
    };

    class InsertCommand : public TransactionCursor::Command {
     public:
        InsertCommand(SlokedCursor &cursor, const SlokedTextReader &reader, std::string_view content)
            : Command(cursor, reader), content(content) {
            this->position = TextPosition{cursor.GetLine(), cursor.GetColumn()};
            this->end = TextPosition{cursor.GetLine(), cursor.GetColumn()};
        }

        void apply() override {
            this->cursor.Insert(this->content);
            this->end = TextPosition{cursor.GetLine(), cursor.GetColumn()};
        }
        
        void undo() override {
            this->cursor.SetPosition(this->position.line, this->position.column);
            this->cursor.ClearRegion(this->end);
        }

     private:
        TextPosition position;
        TextPosition end;
        std::string content;
    };

    class NewlineCommand : public TransactionCursor::Command {
     public:
        NewlineCommand(SlokedCursor &cursor, const SlokedTextReader &reader, std::string_view content)
            : Command(cursor, reader), content(content) {
            this->position = TextPosition{cursor.GetLine(), cursor.GetColumn()};
        }

        void apply() override {
            this->cursor.NewLine(this->content);
            this->end = TextPosition{cursor.GetLine(), cursor.GetColumn()};
        }
        
        void undo() override {
            this->cursor.SetPosition(this->position.line, this->reader.LineLength(this->position.line));
            this->cursor.ClearRegion(this->end);
            this->cursor.SetPosition(this->position.line, this->position.column);
        }

     private:
        TextPosition position;
        TextPosition end;
        std::string content;
    };

    class DeleteBackCommand : public TransactionCursor::Command {
     public:
        DeleteBackCommand(SlokedCursor &cursor, const SlokedTextReader &reader)
            : Command(cursor, reader) {
            if (this->cursor.GetColumn() > 0) {
                this->position = TextPosition{cursor.GetLine(), cursor.GetColumn()};
                this->deleted = this->reader.Read(TextPosition{this->position.line, this->position.column - 1}, this->position)[0];
                this->line_removed = false;
            } else if (this->cursor.GetLine() > 0) {
                this->position = TextPosition{this->cursor.GetLine() - 1, this->reader.LineLength(this->cursor.GetLine() - 1)};
                this->line_removed = true;
            }
        }

        void apply() override {
            this->cursor.DeleteBackward();
        }
        
        void undo() override {
            if (!this->line_removed) {
                this->cursor.SetPosition(this->position.line, this->position.column - 1);
                this->cursor.Insert(this->deleted);
            } else {
                this->cursor.SetPosition(this->position.line, this->position.column);
                this->cursor.NewLine("");
            }
        }
    
     private:
        bool line_removed;
        TextPosition position;
        std::string deleted;
    };

    class DeleteFrontCommand : public TransactionCursor::Command {
     public:
        DeleteFrontCommand(SlokedCursor &cursor, const SlokedTextReader &reader)
            : Command(cursor, reader) {
            this->position = TextPosition{cursor.GetLine(), cursor.GetColumn()};
            if (this->cursor.GetColumn() < this->reader.LineLength(this->cursor.GetLine())) {
                this->deleted = this->reader.Read(this->position, TextPosition{this->position.line, this->position.column + 1})[0];
                this->line_removed = false;
            } else if (this->cursor.GetLine() > 0) {
                this->line_removed = true;
            }
        }

        void apply() override {
            this->cursor.DeleteForward();
        }
        
        void undo() override {
            if (!this->line_removed) {
                this->cursor.SetPosition(this->position.line, this->position.column);
                this->cursor.Insert(this->deleted);
                this->cursor.SetPosition(this->position.line, this->position.column);
            } else {
                this->cursor.SetPosition(this->position.line, this->position.column);
                this->cursor.NewLine("");
                this->cursor.SetPosition(this->position.line, this->position.column);
            }
        }
    
     private:
        bool line_removed;
        TextPosition position;
        std::string deleted;
    };

    class ClearRegionCommand : public TransactionCursor::Command {
     public:
        ClearRegionCommand(SlokedCursor &cursor, const SlokedTextReader &reader, const TextPosition &from, const TextPosition &to)
            : Command(cursor, reader), from(from), to(to) {
            this->position = TextPosition{cursor.GetLine(), cursor.GetColumn()};
            this->deleted = this->reader.Read(from, to);
        }

        void apply() override {
            this->cursor.ClearRegion(this->from, this->to);
        }

        void undo() override {
            this->cursor.SetPosition(this->from.line, this->from.column);
            if (!this->deleted.empty()) {
                this->cursor.Insert(this->deleted[0]);
                for (std::size_t i = 1 ; i < this->deleted.size(); i++) {
                    this->cursor.NewLine("");
                    this->cursor.Insert(this->deleted[i]);
                }
            }
            this->cursor.SetPosition(this->position.line, this->position.column);
        }

     private:
        TextPosition position;
        TextPosition from;
        TextPosition to;
        std::vector<std::string> deleted;
    };

    TransactionCursor::TransactionCursor(SlokedCursor &base, const SlokedTextReader &reader)
        : base(base), reader(reader) {}

    void TransactionCursor::Undo() {
        if (!this->history.empty()) {
            auto cmd = this->history.back();
            cmd->undo();
            this->history.pop_back();
            this->redo.push_back(cmd);
        }
    }

    bool TransactionCursor::HasUndoable() const {
        return !this->history.empty();
    }

    void TransactionCursor::Redo() {
        if (!this->redo.empty()) {
            auto cmd = this->redo.back();
            cmd->apply();
            this->redo.pop_back();
            this->history.push_back(cmd);
        }
    }

    bool TransactionCursor::HasRedoable() const {
        return !this->redo.empty();
    }
    
    TextPosition::Line TransactionCursor::GetLine() const {
        return this->base.GetLine();
    }

    TextPosition::Column TransactionCursor::GetColumn() const {
        return this->base.GetColumn();
    }

    void TransactionCursor::SetPosition(Line l, Column c) {
        this->applyCommand(std::make_shared<PositioningCommand>(this->base, this->reader, [l, c](SlokedCursor &cursor) {
            cursor.SetPosition(l, c);
        }));
    }

    void TransactionCursor::MoveUp(Line l) {
        this->applyCommand(std::make_shared<PositioningCommand>(this->base, this->reader, [l](SlokedCursor &cursor) {
            cursor.MoveUp(l);
        }));
    }

    void TransactionCursor::MoveDown(Line l) {
        this->applyCommand(std::make_shared<PositioningCommand>(this->base, this->reader, [l](SlokedCursor &cursor) {
            cursor.MoveDown(l);
        }));
    }

    void TransactionCursor::MoveForward(Column c) {
        this->applyCommand(std::make_shared<PositioningCommand>(this->base, this->reader, [c](SlokedCursor &cursor) {
            cursor.MoveForward(c);
        }));
    }

    void TransactionCursor::MoveBackward(Column c) {
        this->applyCommand(std::make_shared<PositioningCommand>(this->base, this->reader, [c](SlokedCursor &cursor) {
            cursor.MoveBackward(c);
        }));
    }

    void TransactionCursor::Insert(std::string_view view) {
        this->applyCommand(std::make_shared<InsertCommand>(this->base, this->reader, view));
    }
    
    void TransactionCursor::NewLine(std::string_view view) {
        this->applyCommand(std::make_shared<NewlineCommand>(this->base, this->reader, view));
    }

    void TransactionCursor::DeleteBackward() {
        this->applyCommand(std::make_shared<DeleteBackCommand>(this->base, this->reader));
    }

    void TransactionCursor::DeleteForward() {
        this->applyCommand(std::make_shared<DeleteFrontCommand>(this->base, this->reader));
    }

    void TransactionCursor::ClearRegion(const TextPosition &from, const TextPosition &to) {
        this->applyCommand(std::make_shared<ClearRegionCommand>(this->base, this->reader, from, to));
    }

    void TransactionCursor::applyCommand(std::shared_ptr<Command> cmd) {
        this->redo.clear();
        cmd->apply();
        this->history.push_back(cmd);
    }
}