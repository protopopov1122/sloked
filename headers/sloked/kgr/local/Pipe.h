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

#ifndef SLOKED_KGR_LOCAL_PIPE_H_
#define SLOKED_KGR_LOCAL_PIPE_H_

#include "sloked/kgr/Pipe.h"
#include <mutex>
#include <queue>
#include <memory>
#include <atomic>

namespace sloked {

    struct KgrLocalPipeDescriptor {
        std::atomic<KgrPipe::Status> status;
    };

    struct KgrLocalPipeContent {
        std::queue<KgrValue> content;
        std::mutex content_mtx;
    };

    class KgrLocalPipe : public KgrPipe {
     public:
        virtual ~KgrLocalPipe();
        Status GetStatus() const override;
        bool Empty() const override;
        KgrValue Receive() override;
        void Skip() override;
        void Send(KgrValue &&) override;
        void Close() override;

        static std::pair<std::unique_ptr<KgrPipe>, std::unique_ptr<KgrPipe>> Make();

     private:
        KgrLocalPipe(std::shared_ptr<KgrLocalPipeDescriptor>, std::shared_ptr<KgrLocalPipeContent>, std::shared_ptr<KgrLocalPipeContent>);

        std::shared_ptr<KgrLocalPipeDescriptor> descriptor;
        std::shared_ptr<KgrLocalPipeContent> input;
        std::shared_ptr<KgrLocalPipeContent> output;
    };
}

#endif