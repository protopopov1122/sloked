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

#include "sloked/core/Encoding.h"
#include "sloked/screen/widgets/TextPane.h"
#include "sloked/screen/widgets/TextPaneWidget.h"
#include "sloked/editor/DocumentSet.h"
#include "sloked/services/Cursor.h"
#include "sloked/services/TextRender.h"

namespace sloked {

    class SlokedTextEditor : public SlokedTextPaneWidget {
     public:
        SlokedTextEditor(const Encoding &, std::unique_ptr<KgrPipe>, std::unique_ptr<KgrPipe>, SlokedEditorDocumentSet::DocumentId, SlokedBackgroundGraphics = SlokedBackgroundGraphics::Black);

        bool ProcessInput(const SlokedKeyboardInput &) override;
        void Render(SlokedTextPane &) override;

     private:
        EncodingConverter conv;
        SlokedCursorClient cursorClient;
        SlokedTextRenderClient renderClient;
        SlokedBackgroundGraphics background;
    };
}

#endif