/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/text/cursor/Transaction.h"
#include "sloked/text/cursor/EditingPrimitives.h"
#include <limits>
#include <iostream>

namespace sloked {

    SlokedTransactionPatch::SlokedTransactionPatch() {
        this->NextTransaction();
    }

    void SlokedTransactionPatch::NextTransaction() {
        this->patch.push_back(RangeMap<TextPosition, TextPositionDelta>{});
    }

    void SlokedTransactionPatch::Insert(const TextPosition &from, const TextPosition &to, const TextPositionDelta & delta) {
        this->patch.back().Insert(from, to, delta);
    }

    bool SlokedTransactionPatch::Has(const TextPosition &pos) const {
        for (const auto &trans : this->patch) {
            if (trans.Has(pos)) {
                return true;
            }
        }
        return false;
    }

    TextPositionDelta SlokedTransactionPatch::At(const TextPosition &pos) const {
        TextPosition current {pos};
        for (const auto &trans : this->patch) {
            if (trans.Has(current)) {
                const auto &delta = trans.At(current);
                current.line += delta.line;
                current.column += delta.column;
            }
        }
        return TextPositionDelta {
            static_cast<TextPositionDelta::Line>(current.line) - static_cast<TextPositionDelta::Line>(pos.line),
            static_cast<TextPositionDelta::Column>(current.column) - static_cast<TextPositionDelta::Column>(pos.column)
        };
    }

    SlokedCursorTransaction::SlokedCursorTransaction(Action action, const Content &content)
        : action(action), argument(content) {}

    SlokedCursorTransaction::SlokedCursorTransaction(Action action, const DeletePosition &pos)
        : action(action), argument(pos) {}
    
    SlokedCursorTransaction::SlokedCursorTransaction(const Range &range)
        : action(Action::Clear), argument(range) {}

    SlokedCursorTransaction::SlokedCursorTransaction(const Batch &batch)
        : action(Action::Batch), argument(batch) {}

    TextPosition SlokedCursorTransaction::GetPosition() const {
        switch (this->action) {
            case Action::Insert:
            case Action::Newline: {
                const auto &arg = std::get<1>(this->argument);
                return arg.position;
            }

            case Action::DeleteForward:
            case Action::DeleteBackward: {
                const auto &arg = std::get<0>(this->argument);
                return arg.position;
            }

            case Action::Clear: {
                const auto &arg = std::get<2>(this->argument);
                return arg.from;
            }

            case Action::Batch: {
                const auto &batch = std::get<3>(this->argument);
                if (!batch.empty()) {
                    return batch.at(0).GetPosition();
                }
            } break;
        }
        return TextPosition{};
    }

    void SlokedCursorTransaction::Commit(TextBlock &text, const Encoding &encoding) const {
        switch (this->action) {
            case Action::Insert:
                SlokedEditingPrimitives::Insert(text, encoding, std::get<1>(this->argument).position, std::get<1>(this->argument).content);
                break;

            case Action::Newline:
                SlokedEditingPrimitives::Newline(text, encoding, std::get<1>(this->argument).position, std::get<1>(this->argument).content);
                break;

            case Action::DeleteBackward:
                SlokedEditingPrimitives::DeleteBackward(text, encoding, std::get<0>(this->argument).position);
                break;

            case Action::DeleteForward:
                SlokedEditingPrimitives::DeleteForward(text, encoding, std::get<0>(this->argument).position);
                break;

            case Action::Clear:
                SlokedEditingPrimitives::ClearRegion(text, encoding, std::get<2>(this->argument).from, std::get<2>(this->argument).to);
                break;
            
            case Action::Batch: {
                const auto &batch = std::get<3>(this->argument);
                for (const auto &trans : batch) {
                    trans.Commit(text, encoding);
                }
            } break;
        }
    }

    void SlokedCursorTransaction::Rollback(TextBlock &text, const Encoding &encoding) const {
        switch (this->action) {
            case Action::Insert: {
                const auto &arg = std::get<1>(this->argument);
                TextPosition from = arg.position;
                TextPosition to {from.line, static_cast<TextPosition::Column>(from.column + encoding.CodepointCount(arg.content))};
                SlokedEditingPrimitives::ClearRegion(text, encoding, from, to);
            } break;

            case Action::Newline: {
                const auto &arg = std::get<1>(this->argument);
                TextPosition from = arg.position;
                TextPosition to {from.line + 1, static_cast<TextPosition::Column>(encoding.CodepointCount(arg.content))};
                SlokedEditingPrimitives::ClearRegion(text, encoding, from, to);
            } break;

            case Action::DeleteBackward: {
                const auto &arg = std::get<0>(this->argument);
                if (arg.position.column > 0) {
                    TextPosition from {arg.position.line, arg.position.column - 1};
                    SlokedEditingPrimitives::Insert(text, encoding, from, arg.content);
                } else if (arg.position.line > 0) {
                    SlokedEditingPrimitives::Newline(text, encoding, TextPosition{arg.position.line - 1, arg.width}, "");
                }
            } break;

            case Action::DeleteForward: {
                const auto &arg = std::get<0>(this->argument);
                if (arg.position.column < arg.width) {
                    SlokedEditingPrimitives::Insert(text, encoding, arg.position, arg.content);
                } else {
                    SlokedEditingPrimitives::Newline(text, encoding, arg.position, "");
                }
            } break;

            case Action::Clear: {
                const auto &arg = std::get<2>(this->argument);
                if (!arg.content.empty()) {
                    auto pos = SlokedEditingPrimitives::Insert(text, encoding, arg.from, arg.content[0]);
                    for (std::size_t i = 1 ; i < arg.content.size(); i++) {
                        pos = SlokedEditingPrimitives::Newline(text, encoding, pos, "");
                        pos = SlokedEditingPrimitives::Insert(text, encoding, pos, arg.content[i]);
                    }
                }
            } break;

            case Action::Batch: {
                const auto &batch = std::get<3>(this->argument);
                for (std::size_t i = 0; i < batch.size(); i++) {
                    batch.at(batch.size() - i - 1).Rollback(text, encoding);
                }
            } break;
        }
    }

    SlokedTransactionPatch SlokedCursorTransaction::CommitPatch(const Encoding &encoding) const {
        SlokedTransactionPatch patch;
        this->CommitPatch(encoding, patch);
        return patch;
    }

    SlokedTransactionPatch SlokedCursorTransaction::RollbackPatch(const Encoding &encoding) const {
        SlokedTransactionPatch patch;
        this->RollbackPatch(encoding, patch);
        return patch;
    }

    void SlokedCursorTransaction::Apply(const SlokedTransactionPatch &impact) {
        switch (this->action) {
            case Action::Newline:
            case Action::Insert: {
                auto &arg = std::get<1>(this->argument);
                if (impact.Has(arg.position)) {
                    const auto &delta = impact.At(arg.position);
                    arg.position.line += delta.line;
                    arg.position.column += delta.column;
                }
            } break;

            case Action::DeleteBackward:
            case Action::DeleteForward: {
                auto &arg = std::get<0>(this->argument);
                if (impact.Has(arg.position)) {
                    const auto &delta = impact.At(arg.position);
                    arg.position.line += delta.line;
                    arg.position.column += delta.column;
                }
            } break;

            case Action::Clear: {
                auto &arg = std::get<2>(this->argument);
                if (impact.Has(arg.from)) {
                    const auto &delta = impact.At(arg.from);
                    arg.from.line += delta.line;
                    arg.from.column += delta.column;
                }
                if (impact.Has(arg.to)) {
                    const auto &delta = impact.At(arg.to);
                    arg.to.line += delta.line;
                    arg.to.column += delta.column;
                }
            } break;

            case Action::Batch: {
                auto &batch = std::get<3>(this->argument);
                for (auto &trans : batch) {
                    trans.Apply(impact);
                }
            } break;
        }
    }

    void SlokedCursorTransaction::Update(TextBlock &text, const Encoding &encoding) {
        switch (this->action) {
            case Action::DeleteBackward: {
                auto &arg = std::get<0>(this->argument);
                arg.content = "";
                arg.width = 0;
                if (arg.position.column > 0) {
                    std::string_view view = text.GetLine(arg.position.line);
                    auto pos = encoding.GetCodepoint(view, arg.position.column - 1);
                    arg.content = view.substr(pos.first, pos.second);
                } else {
                    arg.width = encoding.CodepointCount(text.GetLine(arg.position.line - 1));
                }
            } break;

            case Action::DeleteForward: {
                auto &arg = std::get<0>(this->argument);
                arg.content = "";
                arg.width = encoding.CodepointCount(text.GetLine(arg.position.line));
                if (arg.position.column < arg.width) {
                    std::string_view view = text.GetLine(arg.position.line);
                    auto pos = encoding.GetCodepoint(view, arg.position.column);
                    arg.content = view.substr(pos.first, pos.second);
                }
            } break;

            case Action::Clear: {
                auto &arg = std::get<2>(this->argument);
                arg.content = SlokedEditingPrimitives::Read(text, encoding, arg.from, arg.to);
            } break;

            case Action::Batch: {
                auto &batch = std::get<3>(this->argument);
                for (auto &trans : batch) {
                    trans.Update(text, encoding);
                }
            } break;

            default:
                break;
        }
    }

    void SlokedCursorTransaction::CommitPatch(const Encoding &encoding, SlokedTransactionPatch &patch) const {
        TextPosition::Line max_line = std::numeric_limits<TextPosition::Line>::max();
        TextPosition::Column max_column = std::numeric_limits<TextPosition::Column>::max();
        switch (this->action) {
            case Action::Insert: {
                const auto &arg = std::get<1>(this->argument);
                std::size_t content_len = encoding.CodepointCount(arg.content);
                patch.Insert(TextPosition{arg.position.line, arg.position.column},
                    TextPosition{arg.position.line, max_column},
                    TextPositionDelta{0, static_cast<TextPositionDelta::Column>(content_len)});
            } break;

            case Action::Newline: {
                const auto &arg = std::get<1>(this->argument);
                std::size_t content_len = encoding.CodepointCount(arg.content);
                patch.Insert(TextPosition{arg.position.line, arg.position.column},
                    TextPosition{arg.position.line, max_column},
                    TextPositionDelta{1, -static_cast<TextPositionDelta::Column>(arg.position.column) + -static_cast<TextPositionDelta::Column>(content_len)});
                patch.Insert(TextPosition{arg.position.line + 1, 0},
                    TextPosition{max_line, max_column},
                    TextPositionDelta{1, 0});
            } break;

            case Action::DeleteBackward: {
                const auto &arg = std::get<0>(this->argument);
                if (arg.position.column > 0) {
                    patch.Insert(arg.position, TextPosition{arg.position.line, max_column}, TextPositionDelta{0, -1});
                } else if (arg.position.line > 0) {
                    patch.Insert(TextPosition{arg.position.line, arg.position.column}, TextPosition{arg.position.line, max_column}, TextPositionDelta{-1, arg.width});
                    patch.Insert(TextPosition{arg.position.line + 1, 0}, TextPosition{max_line, max_column}, TextPositionDelta{-1, 0});
                }
            } break;

            case Action::DeleteForward: {
                const auto &arg = std::get<0>(this->argument);
                if (arg.position.column < arg.width) {
                    patch.Insert(TextPosition{arg.position.line, arg.position.column + 1},
                        TextPosition{arg.position.line, max_column},
                        TextPositionDelta{0, -1});
                } else {
                    patch.Insert(TextPosition{arg.position.line + 1, arg.position.column},
                        TextPosition{arg.position.line + 1, max_column},
                        TextPositionDelta{-1, arg.position.column});
                    patch.Insert(TextPosition{arg.position.line + 2, 0},
                        TextPosition{max_line, max_column},
                        TextPositionDelta{-1, 0});
                }
            } break;

            case Action::Clear: {
                const auto &arg = std::get<2>(this->argument);
                if (arg.content.size() > 0) {
                    std::size_t newlines = arg.content.size() - 1;
                    patch.Insert(TextPosition{arg.to.line, arg.to.column},
                        TextPosition{arg.to.line, max_column},
                        TextPositionDelta{-static_cast<TextPositionDelta::Column>(newlines),
                        static_cast<TextPositionDelta::Column>(arg.from.column) - static_cast<TextPositionDelta::Column>(encoding.CodepointCount(arg.content.at(arg.content.size() - 1)))});
                    if (newlines > 0) {
                        patch.Insert(TextPosition{arg.to.line + 1, 0},
                            TextPosition{max_line, max_column},
                            TextPositionDelta{-static_cast<TextPositionDelta::Column>(newlines), 0});
                    }
                }
            } break;

            case Action::Batch: {
                const auto &batch = std::get<3>(this->argument);
                for (const auto &trans : batch) {
                    trans.CommitPatch(encoding, patch);
                    patch.NextTransaction();
                }
            } break;
        }
    }

    void SlokedCursorTransaction::RollbackPatch(const Encoding &encoding, SlokedTransactionPatch &patch) const {
        TextPosition::Line max_line = std::numeric_limits<TextPosition::Line>::max();
        TextPosition::Column max_column = std::numeric_limits<TextPosition::Column>::max();
        switch (this->action) {
            case Action::Insert: {
                const auto &arg = std::get<1>(this->argument);
                std::size_t content_len = encoding.CodepointCount(arg.content);
                patch.Insert(TextPosition{arg.position.line, arg.position.column + static_cast<TextPosition::Column>(content_len)},
                    TextPosition{arg.position.line, max_column},
                    TextPositionDelta{0, -static_cast<TextPositionDelta::Column>(content_len)});
            } break;

            case Action::Newline: {
                const auto &arg = std::get<1>(this->argument);
                std::size_t content_len = encoding.CodepointCount(arg.content);
                patch.Insert(TextPosition{arg.position.line + 1, static_cast<TextPosition::Column>(content_len)},
                    TextPosition{arg.position.line + 1, max_column},
                    TextPositionDelta{-1, static_cast<TextPositionDelta::Column>(arg.position.column) - static_cast<TextPositionDelta::Column>(content_len)});
                patch.Insert(TextPosition{arg.position.line + 2, 0},
                    TextPosition{max_line, max_column},
                    TextPositionDelta{-1, 0});
            } break;

            case Action::DeleteBackward: {
                const auto &arg = std::get<0>(this->argument);
                if (arg.position.column > 0) {
                    patch.Insert(arg.position, TextPosition{arg.position.line, max_column}, TextPositionDelta{0, 1});
                } else if (arg.position.line > 0) {
                    patch.Insert(TextPosition{arg.position.line - 1, arg.position.column}, TextPosition{arg.position.line - 1, max_column}, TextPositionDelta{1, -arg.width});
                    patch.Insert(TextPosition{arg.position.line, 0}, TextPosition{max_line, max_column}, TextPositionDelta{1, 0});
                }
            } break;

            case Action::DeleteForward: {
                const auto &arg = std::get<0>(this->argument);
                if (arg.position.column < arg.width) {
                    patch.Insert(TextPosition{arg.position.line, arg.position.column},
                        TextPosition{arg.position.line, max_column},
                        TextPositionDelta{0, 1});
                } else {
                    patch.Insert(TextPosition{arg.position.line, arg.position.column},
                        TextPosition{arg.position.line, max_column},
                        TextPositionDelta{1, -arg.position.column});
                    patch.Insert(TextPosition{arg.position.line + 1, 0},
                        TextPosition{max_line, max_column},
                        TextPositionDelta{1, 0});
                }
            } break;

            case Action::Clear: {
                const auto &arg = std::get<2>(this->argument);
                if (arg.content.size() > 0) {
                    auto newlines = static_cast<TextPositionDelta::Line>(arg.content.size()) - 1;
                    patch.Insert(TextPosition{arg.from.line, arg.from.column + 1},
                        TextPosition{arg.from.line, max_column},
                        TextPositionDelta{newlines, static_cast<TextPositionDelta::Column>(encoding.CodepointCount(arg.content.at(arg.content.size() - 1)))});
                    if (newlines > 0) {
                        patch.Insert(TextPosition{arg.from.line + 1, 0},
                            TextPosition{max_line, max_column},
                            TextPositionDelta{newlines, 0});
                    }
                }
            } break;

            case Action::Batch: {
                const auto &batch = std::get<3>(this->argument);
                for (const auto &trans : batch) {
                    trans.RollbackPatch(encoding, patch);
                    patch.NextTransaction();
                }
            } break;
        }
    }
}