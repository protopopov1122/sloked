/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#include "sloked/services/ScreenSize.h"

namespace sloked {

    class SlokedScreenSizeNotificationContext : public KgrLocalContext {
     public:
        SlokedScreenSizeNotificationContext(std::unique_ptr<KgrPipe> pipe, SlokedScreenSize &size)
            : KgrLocalContext(std::move(pipe)), size(size) {}

        ~SlokedScreenSizeNotificationContext() {
            if (this->unsubscribe) {
                this->unsubscribe();
            }
        }

        void Run() final {
            if (!this->pipe->Empty()) {
                this->pipe->DropAll();
                this->SendSize();
                if (this->unsubscribe) {
                    this->unsubscribe();
                }
                this->unsubscribe = this->size.Listen([this](const auto &) {
                    this->SendSize();
                });
            }
        }

     private:
        void SendSize() {
            auto sz = this->size.GetSize();
            this->pipe->Write(KgrDictionary {
                { "height", static_cast<int64_t>(sz.line) },
                { "width", static_cast<int64_t>(sz.column) }
            });
        }

        SlokedScreenSize &size;
        std::function<void()> unsubscribe;
    };

    SlokedScreenSizeNotificationService::SlokedScreenSizeNotificationService(SlokedScreenSize &size, KgrContextManager<KgrLocalContext> &contextManager)
        : size(size), contextManager(contextManager) {}

    void SlokedScreenSizeNotificationService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedScreenSizeNotificationContext>(std::move(pipe), this->size);
        this->contextManager.Attach(std::move(ctx));
    }

    SlokedScreenSizeNotificationClient::SlokedScreenSizeNotificationClient(std::unique_ptr<KgrPipe> pipe)
        : pipe(std::move(pipe)) {
        this->pipe->Write({});
        auto sz = this->pipe->ReadWait();
        this->currentSize = {
            static_cast<TextPosition::Column>(sz.AsDictionary()["height"].AsInt()),
            static_cast<TextPosition::Line>(sz.AsDictionary()["width"].AsInt())
        };
    }

    TextPosition SlokedScreenSizeNotificationClient::GetSize() const {
        return this->currentSize.load();
    }

    void SlokedScreenSizeNotificationClient::Listen(Callback listener) {
        this->pipe->SetMessageListener([this, listener = std::move(listener)] {
            while (!this->pipe->Empty()) {
                auto sz = this->pipe->Read();
                this->currentSize = {
                    static_cast<TextPosition::Column>(sz.AsDictionary()["height"].AsInt()),
                    static_cast<TextPosition::Line>(sz.AsDictionary()["width"].AsInt())
                };
            }
            if (this->pipe->GetStatus() == KgrPipe::Status::Open) {
                listener(this->currentSize.load());
            }
        });
    }

    void SlokedScreenSizeNotificationClient::Close() {
        this->pipe->Close();
    }
}