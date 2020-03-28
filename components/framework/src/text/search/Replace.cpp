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

#include "sloked/text/search/Replace.h"

#include <regex>

#include "sloked/core/Locale.h"

namespace sloked {

    SlokedTextReplacer::SlokedTextReplacer(
        TextBlock &text, std::unique_ptr<SlokedTransactionStream> transactions,
        const Encoding &encoding)
        : text(text), transactions(std::move(transactions)), encoding(encoding),
          conv(Encoding::Get("system"), encoding) {}

    void SlokedTextReplacer::Replace(const SlokedSearchEntry &entry,
                                     std::string_view value,
                                     bool replace_groups) {
        TransactionBatch batch(*this->transactions, this->encoding);
        TransactionCursor cursor(this->text, this->encoding, batch);
        this->ReplaceImpl(cursor, entry, this->conv.Convert(value),
                          replace_groups);
        batch.Finish();
    }

    std::string SlokedTextReplacer::Prepare(const SlokedSearchEntry &entry,
                                            std::string_view value) {
        std::string str{value};
        if (entry.groups.empty()) {
            return str;
        }
        EncodingConverter conv(this->encoding, SlokedLocale::SystemEncoding());
        str = conv.Convert(str);
        std::regex groupNum(R"(\$\{[0-9]+\})");
        while (true) {
            std::smatch match;
            if (std::regex_search(str, match, groupNum)) {
                std::size_t idx = std::stoull(match.str().substr(2));
                std::string groupValue =
                    idx < entry.groups.size()
                        ? conv.Convert(entry.groups.at(idx))
                        : "";
                str.replace(match.position(), match.str().size(), groupValue);
            } else {
                break;
            }
        }
        return str;
    }

    void SlokedTextReplacer::ReplaceImpl(SlokedCursor &cursor,
                                         const SlokedSearchEntry &entry,
                                         std::string_view value,
                                         bool replace_groups) {
        cursor.ClearRegion(
            entry.start,
            TextPosition{entry.start.line, entry.start.column + entry.length});
        cursor.SetPosition(entry.start.line, entry.start.column);
        cursor.Insert(value);
    }
}  // namespace sloked
