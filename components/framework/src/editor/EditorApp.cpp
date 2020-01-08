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

#include "sloked/editor/EditorApp.h"
#include "sloked/core/Error.h"
#include "sloked/kgr/net/Config.h"

namespace sloked {

    SlokedEditorApp::SlokedEditorApp()
        : running(false), network{nullptr} {}

    bool SlokedEditorApp::IsRunning() const {
        return this->running.load();
    }

    void SlokedEditorApp::Initialize(std::unique_ptr<SlokedIOPoll> ioPoll, SlokedSocketFactory &network) {
        if (this->running.exchange(true)) {
            throw SlokedError("EditorApp: Already initialized");
        }
        this->ioPoll = std::move(ioPoll);
        this->ioPoller = std::make_unique<SlokedDefaultIOPollThread>(*this->ioPoll);
        this->ioPoller->Start(KgrNetConfig::RequestTimeout);
        this->closeables.Attach(*this->ioPoller);
        this->sched.Start();
        this->closeables.Attach(this->sched);
        this->network = std::addressof(network);
    }

    void SlokedEditorApp::InitializeCrypto(SlokedCrypto &crypto) {
        if (!this->running.load()) {
            throw SlokedError("EditorApp: Not running");
        } else if (this->crypto != nullptr) {
            throw SlokedError("EditorApp: Crypto already initialized");
        } else {
            this->crypto = std::addressof(crypto);
        }
    }

    void SlokedEditorApp::RequestStop() {
        if (!this->running.load()) {
            throw SlokedError("EditorApp: Not running");
        } else {
            this->termination.Notify();
        }
    }

    void SlokedEditorApp::WaitForStop() {
        this->termination.WaitAll();
        this->closeables.Close();
        this->crypto = nullptr;
        this->running = false;
    }

    void SlokedEditorApp::Attach(SlokedCloseable &closeable) {
        if (!this->running.load()) {
            throw SlokedError("EditorApp: Not running");
        } else {
            this->closeables.Attach(closeable);
        }
    }

    SlokedCharWidth &SlokedEditorApp::GetCharWidth() {
        return this->charWidth;
    }

    SlokedSchedulerThread &SlokedEditorApp::GetScheduler() {
        return this->sched;
    }

    SlokedIOPoller &SlokedEditorApp::GetIO() {
        return *this->ioPoller;
    }

    SlokedSocketFactory &SlokedEditorApp::GetNetwork() {
        return *this->network;
    }

    SlokedCrypto &SlokedEditorApp::GetCrypto() {
        if (this->crypto) {
            return *this->crypto;
        } else {
            throw SlokedError("EditorApp: Crypto not defined");
        }
    }
}