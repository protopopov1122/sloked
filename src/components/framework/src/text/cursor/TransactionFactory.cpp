/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/text/cursor/TransactionFactory.h"

#include "sloked/text/cursor/EditingPrimitives.h"

namespace sloked {

    SlokedCursorTransaction SlokedTransactionFactory::Insert(
        const TextPosition &position, std::string_view text) {
        return SlokedCursorTransaction{
            SlokedCursorTransaction::Action::Insert,
            SlokedCursorTransaction::Content{position, std::string{text}}};
    }

    SlokedCursorTransaction SlokedTransactionFactory::Newline(
        const TextPosition &position, std::string_view text) {
        return SlokedCursorTransaction{
            SlokedCursorTransaction::Action::Newline,
            SlokedCursorTransaction::Content{position, std::string{text}}};
    }

    SlokedCursorTransaction SlokedTransactionFactory::DeleteBackward(
        const TextBlock &text, const Encoding &encoding,
        const TextPosition &position) {
        std::string content = "";
        TextPosition::Column width = 0;
        if (position.column > 0) {
            std::string_view view = text.GetLine(position.line);
            auto pos = encoding.GetCodepoint(view, position.column - 1);
            content = view.substr(pos.first, pos.second);
        } else if (position.line > 0) {
            width = encoding.CodepointCount(text.GetLine(position.line - 1));
        }
        return SlokedCursorTransaction{
            SlokedCursorTransaction::Action::DeleteBackward,
            SlokedCursorTransaction::DeletePosition{position, content, width}};
    }

    SlokedCursorTransaction SlokedTransactionFactory::DeleteForward(
        const TextBlock &text, const Encoding &encoding,
        const TextPosition &position) {
        std::string content = "";
        TextPosition::Column width =
            encoding.CodepointCount(text.GetLine(position.line));
        if (position.column < width) {
            std::string_view view = text.GetLine(position.line);
            auto pos = encoding.GetCodepoint(view, position.column);
            content = view.substr(pos.first, pos.second);
        }
        return SlokedCursorTransaction{
            SlokedCursorTransaction::Action::DeleteForward,
            SlokedCursorTransaction::DeletePosition{position, content, width}};
    }

    static TextPosition ClampPosition(const TextBlock &text,
                                      const Encoding &encoding,
                                      const TextPosition &position) {
        TextPosition res;
        res.line = std::min(
            position.line, static_cast<TextPosition::Line>(text.GetLastLine()));
        res.column =
            std::min(position.column,
                     static_cast<TextPosition::Column>(
                         encoding.CodepointCount(text.GetLine(res.line))));
        return res;
    }

    SlokedCursorTransaction SlokedTransactionFactory::ClearRegion(
        const TextBlock &text, const Encoding &encoding,
        const TextPosition &from, const TextPosition &to) {
        auto cfrom = ClampPosition(text, encoding, from);
        auto cto = ClampPosition(text, encoding, to);
        return SlokedCursorTransaction{SlokedCursorTransaction::Range{
            cfrom, cto,
            SlokedEditingPrimitives::Read(text, encoding, cfrom, cto)}};
    }
}  // namespace sloked