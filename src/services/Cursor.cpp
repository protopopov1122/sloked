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

#include "sloked/services/Cursor.h"
#include "sloked/core/Error.h"
#include "sloked/core/Locale.h"
#include "sloked/text/cursor/TransactionCursor.h"
#include "sloked/text/cursor/TransactionJournal.h"
#include "sloked/text/fragment/Updater.h"
#include "sloked/text/TextFrame.h"

namespace sloked {

    class SlokedCursorContext : public SlokedServiceContext {
     public:
        SlokedCursorContext(std::unique_ptr<KgrPipe> pipe,
            TextBlock &text, const EncodingConverter &conv,
            std::unique_ptr<SlokedTransactionStream> stream)
            : SlokedServiceContext(std::move(pipe)),
              text(text), conv(conv), cursor(text, conv.GetDestination(), *stream) {
            this->stream = std::move(stream);
        }

     protected:
        void ProcessRequest(const KgrValue &message) override {
            const auto &prms = message.AsDictionary();
            auto command = static_cast<SlokedCursorService::Command>(prms["command"].AsInt());
            switch (command) {
                case SlokedCursorService::Command::Insert:
                    this->cursor.Insert(this->conv.Convert(prms["content"].AsString()));
                    break;

                case SlokedCursorService::Command::MoveUp:
                    this->cursor.MoveUp(1);
                    break;

                case SlokedCursorService::Command::MoveDown:
                    this->cursor.MoveDown(1);
                    break;

                case SlokedCursorService::Command::MoveBackward:
                    this->cursor.MoveBackward(1);
                    break;

                case SlokedCursorService::Command::MoveForward:
                    this->cursor.MoveForward(1);
                    break;

                case SlokedCursorService::Command::NewLine:
                    this->cursor.NewLine("");
                    break;

                case SlokedCursorService::Command::DeleteBackward:
                    this->cursor.DeleteBackward();
                    break;

                case SlokedCursorService::Command::DeleteForward:
                    this->cursor.DeleteForward();
                    break;

                case SlokedCursorService::Command::Undo:
                    this->cursor.Undo();
                    break;

                case SlokedCursorService::Command::Redo:
                    this->cursor.Redo();
                    break;

                case SlokedCursorService::Command::Info:
                    this->SendResponse(KgrDictionary {
                        { "line", static_cast<int64_t>(this->cursor.GetLine()) },
                        { "column", static_cast<int64_t>(this->cursor.GetColumn()) }
                    });
                    break;
            }
        }

        TextBlock &text;
        const EncodingConverter &conv;
        std::unique_ptr<SlokedTransactionStream> stream;
        TransactionCursor cursor;
    };


    SlokedCursorService::SlokedCursorService(TextBlock &text, const Encoding &encoding, TransactionStreamMultiplexer &multiplexer, KgrContextManager<KgrLocalContext> &contextManager)
        : text(text), conv(SlokedLocale::SystemEncoding(), encoding), multiplexer(multiplexer), contextManager(contextManager) {}

    bool SlokedCursorService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedCursorContext>(std::move(pipe), this->text, this->conv, this->multiplexer.NewStream());
        this->contextManager.Attach(std::move(ctx));
        return true;
    }
}