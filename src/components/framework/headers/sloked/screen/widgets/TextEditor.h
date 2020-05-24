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

#ifndef SLOKED_SCREEN_WIDGETS_TEXTEDITOR_H_
#define SLOKED_SCREEN_WIDGETS_TEXTEDITOR_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/OrderedCache.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"
#include "sloked/screen/TaggedFrame.h"
#include "sloked/screen/widgets/TextPane.h"
#include "sloked/screen/widgets/TextPaneWidget.h"
#include "sloked/services/Cursor.h"
#include "sloked/services/DocumentNotify.h"
#include "sloked/services/TextRender.h"
#include "sloked/screen/GraphemeString.h"

namespace sloked {

    class SlokedTextEditor : public SlokedTextPaneWidget {
     public:
        SlokedTextEditor(
            const Encoding &, const SlokedCharPreset &, std::unique_ptr<KgrPipe>,
            std::function<void(SlokedCursorClient &)>, std::unique_ptr<KgrPipe>,
            std::unique_ptr<KgrPipe>, SlokedEditorDocumentSet::DocumentId,
            SlokedBackgroundGraphics = SlokedBackgroundGraphics::Black,
            SlokedForegroundGraphics = SlokedForegroundGraphics::White);
        ~SlokedTextEditor();

        bool ProcessInput(const SlokedKeyboardInput &) final;
        TaskResult<void> RenderSurface(SlokedGraphicsPoint::Coordinate,
                                       TextPosition::Line,
                                       const SlokedFontProperties &) final;
        void ShowSurface(SlokedTextPane &) final;
        void OnUpdate(std::function<void()>) final;

     private:
        class Helpers;
        class RendererFrame;
        class RendererState;
        friend class RendererState;

        struct DocumentState {
            DocumentState();
            RendererFrame DeriveRendererFrame(TextPosition::Line, SlokedGraphicsPoint::Coordinate);

            TextPosition directCursor;
            TextPosition virtualCursorOffset;
            TextPosition virtualCursor;
            std::unique_ptr<SlokedGraphemeStringLayout> currentLineLayout;
            SlokedOrderedCache<TextPosition::Line, SlokedTaggedTextFrame<bool>::TaggedLine> renderCache;
            std::vector<SlokedTaggedTextFrame<bool>::TaggedLine> rendered;
        };

        EncodingConverter conv;
        const SlokedCharPreset &charPreset;
        SlokedCursorClient cursorClient;
        SlokedTextRenderClient renderClient;
        SlokedDocumentNotifyClient notifyClient;
        SlokedBackgroundGraphics background;
        SlokedForegroundGraphics foreground;
        std::function<void()> updateListener;
        DocumentState documentState;
        std::shared_ptr<SlokedStandardLifetime> lifetime;
    };
}  // namespace sloked

#endif
