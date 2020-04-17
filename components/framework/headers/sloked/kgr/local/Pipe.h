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

#ifndef SLOKED_KGR_LOCAL_PIPE_H_
#define SLOKED_KGR_LOCAL_PIPE_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

#include "sloked/kgr/Pipe.h"

namespace sloked {

    struct KgrLocalPipeDescriptor {
        std::atomic<KgrPipe::Status> status;
    };

    struct KgrLocalPipeContent {
        std::queue<KgrValue> content;
        std::mutex content_mtx;
        std::condition_variable content_cv;
        std::function<void()> callback;
    };

    class KgrLocalPipe : public KgrPipe {
     public:
        virtual ~KgrLocalPipe();
        Status GetStatus() const override;
        bool Empty() const override;
        std::size_t Count() const override;
        void Close() override;

        KgrValue Read() override;
        std::optional<KgrValue> ReadOptional() override;
        KgrValue ReadWait() override;
        void SetMessageListener(std::function<void()>) override;
        bool Wait(std::size_t = 1) override;
        void Drop(std::size_t = 1) override;
        void DropAll() override;

        void Write(KgrValue &&) override;
        bool SafeWrite(KgrValue &&) override;

        static std::pair<std::unique_ptr<KgrPipe>, std::unique_ptr<KgrPipe>>
            Make();

     private:
        KgrLocalPipe(std::shared_ptr<KgrLocalPipeDescriptor>,
                     std::shared_ptr<KgrLocalPipeContent>,
                     std::shared_ptr<KgrLocalPipeContent>);

        std::shared_ptr<KgrLocalPipeDescriptor> descriptor;
        std::shared_ptr<KgrLocalPipeContent> input;
        std::shared_ptr<KgrLocalPipeContent> output;
    };
}  // namespace sloked

#endif