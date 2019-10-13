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

#ifndef SLOKED_EDITOR_DOCMGR_DOCUMENTUPSTREAM_H_
#define SLOKED_EDITOR_DOCMGR_DOCUMENTUPSTREAM_H_

#include "sloked/core/NewLine.h"
#include "sloked/namespace/Object.h"
#include "sloked/text/TextBlock.h"
#include "sloked/text/TextDocument.h"
#include <utility>

namespace sloked {

    class SlokedDocumentUpstream {
     public:
        SlokedDocumentUpstream(const TextBlockFactory &, const NewLine &);
        SlokedDocumentUpstream(SlokedNamespace &, const SlokedPath &, const TextBlockFactory &, const NewLine &);
        TextBlock &GetText();
        bool HasUpstream() const;
        void Save();
        void Save(SlokedNamespace &, const SlokedPath &, const TextBlockFactory &, const NewLine &);

     private:
        struct Upstream {
            std::unique_ptr<SlokedNamespaceObject> file;
            std::unique_ptr<SlokedIOView> fileView;
        };

        std::optional<Upstream> upstream;
        std::unique_ptr<TextDocument> content;
        std::reference_wrapper<const TextBlockFactory> blockFactory;
        std::reference_wrapper<const NewLine> newline;
    };
}

#endif