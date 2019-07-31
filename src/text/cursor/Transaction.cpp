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

    TextPosition SlokedEditTransaction::Commit(TextBlock &text, const Encoding &encoding) const {
        switch (this->action) {
            case Action::Insert:
                return SlokedEditingPrimitives::Insert(text, encoding, std::get<1>(this->argument).position, std::get<1>(this->argument).content);

            case Action::Newline:
                return SlokedEditingPrimitives::Newline(text, encoding, std::get<1>(this->argument).position, std::get<1>(this->argument).content);

            case Action::DeleteBackward:
                return SlokedEditingPrimitives::DeleteBackward(text, encoding, std::get<0>(this->argument).position);

            case Action::DeleteForward:
                return SlokedEditingPrimitives::DeleteForward(text, encoding, std::get<0>(this->argument).position);

            case Action::Clear:
                return SlokedEditingPrimitives::ClearRegion(text, encoding, std::get<2>(this->argument).from, std::get<2>(this->argument).to);
            
            case Action::Batch: {
                const auto &batch = std::get<3>(this->argument);
                for (const auto &trans : batch.second) {
                    trans.Commit(text, encoding);
                }
                return batch.first;
            };
        }
        return TextPosition{};
    }

    TextPosition SlokedEditTransaction::Rollback(TextBlock &text, const Encoding &encoding) const {
        switch (this->action) {
            case Action::Insert: {
                const auto &arg = std::get<1>(this->argument);
                TextPosition from = arg.position;
                TextPosition to {from.line, static_cast<TextPosition::Column>(from.column + encoding.CodepointCount(arg.content))};
                return SlokedEditingPrimitives::ClearRegion(text, encoding, from, to);
            }

            case Action::Newline: {
                const auto &arg = std::get<1>(this->argument);
                TextPosition from = arg.position;
                TextPosition to {from.line + 1, static_cast<TextPosition::Column>(encoding.CodepointCount(arg.content))};
                return SlokedEditingPrimitives::ClearRegion(text, encoding, from, to);
            }

            case Action::DeleteBackward: {
                const auto &arg = std::get<0>(this->argument);
                if (arg.position.column > 0) {
                    TextPosition from {arg.position.line, arg.position.column - 1};
                    return SlokedEditingPrimitives::Insert(text, encoding, from, arg.content);
                } else if (arg.position.line > 0) {
                    return SlokedEditingPrimitives::Newline(text, encoding, TextPosition{arg.position.line - 1, arg.width}, "");
                } else {
                    return arg.position;
                }
            }

            case Action::DeleteForward: {
                const auto &arg = std::get<0>(this->argument);
                if (arg.position.column < arg.width) {
                    SlokedEditingPrimitives::Insert(text, encoding, arg.position, arg.content);
                } else {
                    SlokedEditingPrimitives::Newline(text, encoding, arg.position, "");
                }
                return arg.position;
            }

            case Action::Clear: {
                const auto &arg = std::get<2>(this->argument);
                if (!arg.content.empty()) {
                    auto pos = SlokedEditingPrimitives::Insert(text, encoding, arg.from, arg.content[0]);
                    for (std::size_t i = 1 ; i < arg.content.size(); i++) {
                        pos = SlokedEditingPrimitives::Newline(text, encoding, pos, "");
                        pos = SlokedEditingPrimitives::Insert(text, encoding, pos, arg.content[i]);
                    }
                }
                return arg.from;
            }

            case Action::Batch: {
                const auto &batch = std::get<3>(this->argument);
                for (std::size_t i = 0; i < batch.second.size(); i++) {
                    batch.second.at(batch.second.size() - i - 1).Rollback(text, encoding);
                }
                return batch.first;
            }
        }

        return TextPosition{};
    }

    SlokedTransactionPatch SlokedEditTransaction::CommitPatch(const Encoding &encoding) const {
        SlokedTransactionPatch patch;
        this->CommitPatch(encoding, patch);
        return patch;
    }

    SlokedTransactionPatch SlokedEditTransaction::RollbackPatch(const Encoding &encoding) const {
        SlokedTransactionPatch patch;
        this->RollbackPatch(encoding, patch);
        return patch;
    }

    void SlokedEditTransaction::Apply(const SlokedTransactionPatch &impact) {
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
                if (impact.Has(batch.first)) {
                    const auto &delta = impact.At(batch.first);
                    batch.first.line += delta.line;
                    batch.first.column += delta.column;
                }
                for (auto &trans : batch.second) {
                    trans.Apply(impact);
                }
            } break;
        }
    }

    void SlokedEditTransaction::CommitPatch(const Encoding &encoding, SlokedTransactionPatch &patch) const {
        TextPosition::Line max_line = std::numeric_limits<TextPosition::Line>::max();
        TextPosition::Column max_column = std::numeric_limits<TextPosition::Column>::max();
        switch (this->action) {
            case Action::Insert: {
                const auto &arg = std::get<1>(this->argument);
                std::size_t content_len = encoding.CodepointCount(arg.content);
                patch.Insert(TextPosition{arg.position.line, arg.position.column},
                    TextPosition{arg.position.line, max_column},
                    TextPositionDelta{0, content_len});
            } break;

            case Action::Newline: {
                const auto &arg = std::get<1>(this->argument);
                std::size_t content_len = encoding.CodepointCount(arg.content);
                patch.Insert(TextPosition{arg.position.line, arg.position.column},
                    TextPosition{arg.position.line, max_column},
                    TextPositionDelta{1, -arg.position.column + content_len});
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
                    patch.Insert(TextPosition{arg.position.line, arg.position.column},
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
                        TextPositionDelta{-newlines, arg.from.column - encoding.CodepointCount(arg.content.at(arg.content.size() - 1))});
                    if (newlines > 0) {
                        patch.Insert(TextPosition{arg.to.line + 1, 0},
                            TextPosition{max_line, max_column},
                            TextPositionDelta{-newlines, 0});
                    }
                }
            } break;

            case Action::Batch: {
                const auto &batch = std::get<3>(this->argument);
                for (const auto &trans : batch.second) {
                    trans.CommitPatch(encoding, patch);
                    patch.NextTransaction();
                }
            } break;
        }
    }

    void SlokedEditTransaction::RollbackPatch(const Encoding &encoding, SlokedTransactionPatch &patch) const {
        TextPosition::Line max_line = std::numeric_limits<TextPosition::Line>::max();
        TextPosition::Column max_column = std::numeric_limits<TextPosition::Column>::max();
        switch (this->action) {
            case Action::Insert: {
                const auto &arg = std::get<1>(this->argument);
                std::size_t content_len = encoding.CodepointCount(arg.content);
                patch.Insert(TextPosition{arg.position.line, arg.position.column + content_len},
                    TextPosition{arg.position.line, max_column},
                    TextPositionDelta{0, -content_len});
            } break;

            case Action::Newline: {
                const auto &arg = std::get<1>(this->argument);
                std::size_t content_len = encoding.CodepointCount(arg.content);
                patch.Insert(TextPosition{arg.position.line + 1, content_len},
                    TextPosition{arg.position.line + 1, max_column},
                    TextPositionDelta{-1, arg.position.column - content_len});
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
                    std::size_t newlines = arg.content.size() - 1;
                    patch.Insert(TextPosition{arg.from.line, arg.from.column},
                        TextPosition{arg.from.line, max_column},
                        TextPositionDelta{newlines, encoding.CodepointCount(arg.content.at(arg.content.size() - 1))});
                    if (newlines > 0) {
                        patch.Insert(TextPosition{arg.from.line + 1, 0},
                            TextPosition{max_line, max_column},
                            TextPositionDelta{newlines, 0});
                    }
                }
            } break;

            case Action::Batch: {
                const auto &batch = std::get<3>(this->argument);
                for (const auto &trans : batch.second) {
                    trans.RollbackPatch(encoding, patch);
                    patch.NextTransaction();
                }
            } break;
        }
    }
}