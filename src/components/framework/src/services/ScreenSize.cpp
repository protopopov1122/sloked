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

#include "sloked/services/ScreenSize.h"

#include "sloked/sched/CompoundTask.h"

namespace sloked {

    class SlokedScreenSizeNotificationContext : public KgrLocalContext {
     public:
        SlokedScreenSizeNotificationContext(std::unique_ptr<KgrPipe> pipe,
                                            SlokedScreenSize &size)
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
                this->unsubscribe = this->size.Listen(
                    [this](const auto &) { this->SendSize(); });
            }
        }

     private:
        void SendSize() {
            auto sz = this->size.GetScreenSize();
            this->pipe->Write(
                KgrDictionary{{"height", static_cast<int64_t>(sz.line)},
                              {"width", static_cast<int64_t>(sz.column)}});
        }

        SlokedScreenSize &size;
        std::function<void()> unsubscribe;
    };

    SlokedScreenSizeNotificationService::SlokedScreenSizeNotificationService(
        SlokedScreenSize &size,
        KgrContextManager<KgrLocalContext> &contextManager)
        : size(size), contextManager(contextManager) {}

    TaskResult<void> SlokedScreenSizeNotificationService::Attach(
        std::unique_ptr<KgrPipe> pipe) {
        TaskResultSupplier<void> supplier;
        supplier.Wrap([&] {
            auto ctx = std::make_unique<SlokedScreenSizeNotificationContext>(
                std::move(pipe), this->size);
            this->contextManager.Attach(std::move(ctx));
        });
        return supplier.Result();
    }

    SlokedScreenSizeNotificationClient::SlokedScreenSizeNotificationClient()
        : pipe(nullptr), lifetime(std::make_shared<SlokedStandardLifetime>()) {}

    SlokedScreenSizeNotificationClient::~SlokedScreenSizeNotificationClient() {
        this->lifetime->Close();
    }

    TaskResult<void> SlokedScreenSizeNotificationClient::Connect(
        std::unique_ptr<KgrPipe> pipe) {
        this->pipe.ChangePipe(std::move(pipe));
        this->pipe.Write({});
        return SlokedTaskTransformations::Transform(
            this->pipe.Read(),
            [this](const KgrValue &sz) {
                this->currentSize = {static_cast<TextPosition::Column>(
                                         sz.AsDictionary()["height"].AsInt()),
                                     static_cast<TextPosition::Line>(
                                         sz.AsDictionary()["width"].AsInt())};
            },
            this->lifetime);
    }

    TextPosition SlokedScreenSizeNotificationClient::GetSize() const {
        return this->currentSize.load();
    }

    void SlokedScreenSizeNotificationClient::Listen(Callback listener) {
        this->pipe.SetMessageListener([this, listener = std::move(listener)] {
            while (!this->pipe.Empty()) {
                auto sz = this->pipe.Read().UnwrapWait();
                this->currentSize = {static_cast<TextPosition::Column>(
                                         sz.AsDictionary()["height"].AsInt()),
                                     static_cast<TextPosition::Line>(
                                         sz.AsDictionary()["width"].AsInt())};
            }
            if (this->pipe.GetStatus() == KgrPipe::Status::Open) {
                listener(this->currentSize.load());
            }
        });
    }

    void SlokedScreenSizeNotificationClient::Close() {
        this->pipe.Close();
    }
}  // namespace sloked