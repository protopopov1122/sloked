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

#ifndef SLOKED_SCREEN_WIDGETS_TEXTEDITOR_H_
#define SLOKED_SCREEN_WIDGETS_TEXTEDITOR_H_

#include "sloked/core/CharWidth.h"
#include "sloked/core/Encoding.h"
#include "sloked/text/TextBlock.h"
#include "sloked/text/TextFrame.h"
#include "sloked/text/cursor/Cursor.h"
#include "sloked/text/cursor/TransactionJournal.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/screen/widgets/TextPane.h"
#include "sloked/screen/widgets/TextPaneWidget.h"

namespace sloked {

    class SlokedTextEditor : public SlokedTextPaneWidget {
     public:
        SlokedTextEditor(TextBlock &, SlokedCursor &, SlokedTransactionJournal &, SlokedTaggedText<int> &, const EncodingConverter &, const SlokedCharWidth &, SlokedBackgroundGraphics = SlokedBackgroundGraphics::Black);

        bool ProcessInput(const SlokedKeyboardInput &) override;
        void Render(SlokedTextPane &) override;

     private:
        TextBlock &text;
        SlokedCursor &cursor;
        SlokedTransactionJournal &journal;
        SlokedTaggedText<int> &tags;
        const EncodingConverter &conv;
        const SlokedCharWidth &charWidth;

        TextFrameView frame;
        SlokedBackgroundGraphics background;
    };
}

#endif