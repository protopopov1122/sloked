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

#include "sloked/kgr/local/Pipe.h"
#include "sloked/core/Error.h"

namespace sloked {

    KgrLocalPipe::KgrLocalPipe(std::shared_ptr<KgrLocalPipeDescriptor> descriptor, std::shared_ptr<KgrLocalPipeContent> input, std::shared_ptr<KgrLocalPipeContent> output)
        : descriptor(descriptor), input(input), output(output) {}
    
    KgrLocalPipe::~KgrLocalPipe() {
        if (this->descriptor->status == Status::Open) {
            std::unique_lock<std::mutex> ilock(this->input->content_mtx);
            std::unique_lock<std::mutex> olock(this->output->content_mtx);
            this->descriptor->status = Status::Closed;
        }
    }
    
    KgrLocalPipe::Status KgrLocalPipe::GetStatus() const {
        return this->descriptor->status.load();
    }

    bool KgrLocalPipe::Empty() const {
        std::unique_lock<std::mutex> lock(this->input->content_mtx);
        return this->input->content.empty();
    }

    KgrValue KgrLocalPipe::Receive() {
        std::unique_lock<std::mutex> lock(this->input->content_mtx);
        if (this->input->content.empty()) {
            throw SlokedError("KgrLocal: Pipe empty");
        }
        KgrValue msg = std::move(this->input->content.front());
        this->input->content.pop();
        return msg;
    }

    void KgrLocalPipe::Skip() {
        std::unique_lock<std::mutex> lock(this->input->content_mtx);
        if (this->input->content.empty()) {
            throw SlokedError("KgrLocal: Pipe empty");
        }
        this->input->content.pop();
    }

    void KgrLocalPipe::Send(KgrValue &&msg) {
        std::unique_lock<std::mutex> lock(this->output->content_mtx);
        if (this->descriptor->status != Status::Open) {
            throw SlokedError("KgrLocal: Pipe already closed");
        }
        this->output->content.push(std::forward<KgrValue>(msg));
    }

    void KgrLocalPipe::Close() {
        std::unique_lock<std::mutex> ilock(this->input->content_mtx);
        std::unique_lock<std::mutex> olock(this->output->content_mtx);
        this->descriptor->status = Status::Closed;
    }

    std::pair<std::unique_ptr<KgrPipe>, std::unique_ptr<KgrPipe>> KgrLocalPipe::Make() {
        auto descriptor = std::make_shared<KgrLocalPipeDescriptor>();
        descriptor->status = Status::Open;
        auto content1 = std::make_shared<KgrLocalPipeContent>();
        auto content2 = std::make_shared<KgrLocalPipeContent>();
        std::unique_ptr<KgrLocalPipe> pipe1(new KgrLocalPipe(descriptor, content1, content2));
        std::unique_ptr<KgrLocalPipe> pipe2(new KgrLocalPipe(descriptor, content2, content1));
        return std::make_pair(std::move(pipe1), std::move(pipe2));
    }
}