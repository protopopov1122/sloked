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

#ifndef SLOKED_SERVICES_TEXTRENDER_H_
#define SLOKED_SERVICES_TEXTRENDER_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/CharWidth.h"
#include "sloked/kgr/Service.h"
#include "sloked/kgr/ContextManager.h"
#include "sloked/text/TextBlock.h"
#include "sloked/text/cursor/TransactionStream.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/services/Service.h"

namespace sloked {

    class SlokedTextRenderService : public KgrService {
     public:
        SlokedTextRenderService(TextBlock &, const Encoding &, SlokedTransactionListenerManager &, const SlokedCharWidth &, SlokedTextTaggerFactory<int> &,
            KgrContextManager<KgrLocalContext> &);
        bool Attach(std::unique_ptr<KgrPipe>) override;
    
     private:
        TextBlock &text;
        EncodingConverter conv;
        SlokedTransactionListenerManager &transactionListeners;
        SlokedTextTaggerFactory<int> &taggerFactory;
        const SlokedCharWidth &charWidth;
        KgrContextManager<KgrLocalContext> &contextManager;
    };
}

#endif