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

#include "sloked/services/Shutdown.h"

namespace sloked {

    class SlokedShutdownContext : public KgrLocalContext {
     public:
        SlokedShutdownContext(std::unique_ptr<KgrPipe> pipe,
                                    SlokedEditorShutdown &shutdown)
            : KgrLocalContext(std::move(pipe)), shutdown(shutdown) {}

        void Run() override {
            if (!this->pipe->Empty()) {
                this->pipe->Drop();
                bool result = this->shutdown.RequestShutdown();
                this->pipe->Write(result);
            }
        }

     private:
        SlokedEditorShutdown &shutdown;
    };

    SlokedShutdownService::SlokedShutdownService(
        SlokedEditorShutdown &shutdown,
        KgrContextManager<KgrLocalContext> &contextManager)
        : shutdown(shutdown), contextManager(contextManager) {}

    TaskResult<void> SlokedShutdownService::Attach(std::unique_ptr<KgrPipe> pipe) {
        TaskResultSupplier<void> supplier;
        supplier.Wrap([&] {
            auto ctx = std::make_unique<SlokedShutdownContext>(
                std::move(pipe), this->shutdown);
            this->contextManager.Attach(std::move(ctx));
        });
        return supplier.Result();
    }
}  // namespace sloked