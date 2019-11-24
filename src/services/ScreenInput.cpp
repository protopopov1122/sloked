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

#include "sloked/services/ScreenInput.h"
#include "sloked/screen/components/ComponentTree.h"
#include "sloked/core/Locale.h"
#include <set>

namespace sloked {

    class SlokedScreenInputContext : public KgrLocalContext {
     public:
        SlokedScreenInputContext(std::unique_ptr<KgrPipe> pipe, SlokedMonitor<SlokedScreenComponent &> &root, const Encoding &encoding)
            : KgrLocalContext(std::move(pipe)), root(root), encoding(encoding) {}

        virtual ~SlokedScreenInputContext() {
            if (this->listener.has_value()) {
                root.Lock([&](auto &component) {
                    SlokedComponentTree::Traverse(component, this->path.value()).DetachInputHandle(this->listener.value());
                });
            }
        }

        void Run() final {
            while (!this->pipe->Empty()) {
                auto msg = this->pipe->Read();
                if (msg.GetType() == KgrValueType::Null) {
                    this->Close();
                } else {
                    this->Connect(msg);
                }
            }
        }    
    
     private:
        void Connect(const KgrValue &params) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            this->subscribeOnText = false;
            this->subscribes.clear();
            root.Lock([&](auto &component) {
                if (this->listener.has_value()) {
                    SlokedComponentTree::Traverse(component, this->path.value()).DetachInputHandle(this->listener.value());
                }
                this->listener = SlokedComponentTree::Traverse(component, path).AsHandle().AttachInputHandler([this](const auto &evt) {
                    if ((evt.value.index() == 0 && this->subscribeOnText) ||
                        (evt.value.index() == 1 && this->subscribes.find(std::make_pair(std::get<1>(evt.value), evt.alt)) != this->subscribes.end())) {
                        this->SendInput(evt);
                        return true;
                    } else {
                        return false;
                    }
                });
                this->path = path;
                this->subscribeOnText = params.AsDictionary()["text"].AsBoolean();
                const auto &keys = params.AsDictionary()["keys"].AsArray();
                for (const auto &key : keys) {
                    auto code = static_cast<SlokedControlKey>(key.AsDictionary()["code"].AsInt());
                    auto alt = key.AsDictionary()["alt"].AsBoolean();
                    this->subscribes.emplace(std::make_pair(code, alt));
                }
            });
            this->pipe->Write({});
        }

        void Close() {
            this->subscribeOnText = false;
            this->subscribes.clear();
            if (this->listener.has_value()) {
                root.Lock([&](auto &component) {
                    SlokedComponentTree::Traverse(component, this->path.value()).DetachInputHandle(this->listener.value());
                });
                this->listener.reset();
                this->path.reset();
            }
        }

        void SendInput(const SlokedKeyboardInput &evt) {
            EncodingConverter conv(this->encoding, SlokedLocale::SystemEncoding());
            if (evt.value.index() == 0) {
                this->pipe->Write(KgrDictionary {
                    { "alt", evt.alt },
                    { "text", conv.Convert(std::get<0>(evt.value)) }
                });
            } else {
                this->pipe->Write(KgrDictionary {
                    { "alt", evt.alt },
                    { "key", static_cast<int64_t>(std::get<1>(evt.value)) }
                });
            }
        }

        SlokedMonitor<SlokedScreenComponent &> &root;
        const Encoding &encoding;
        std::optional<SlokedPath> path;
        std::optional<SlokedComponentListener> listener;
        bool subscribeOnText;
        std::set<std::pair<SlokedControlKey, bool>> subscribes;
    };

    SlokedScreenInputNotificationService::SlokedScreenInputNotificationService(SlokedMonitor<SlokedScreenComponent &> &root, const Encoding &encoding, KgrContextManager<KgrLocalContext> &contextManager)
        : root(root), encoding(encoding), contextManager(contextManager) {}

    bool SlokedScreenInputNotificationService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedScreenInputContext>(std::move(pipe), this->root, this->encoding);
        this->contextManager.Attach(std::move(ctx));
        return true;
    }

    SlokedScreenInputNotificationClient::SlokedScreenInputNotificationClient(std::unique_ptr<KgrPipe> pipe)
        : pipe(std::move(pipe)) {}

    void SlokedScreenInputNotificationClient::Listen(const std::string &path, bool text, const std::vector<std::pair<SlokedControlKey, bool>> &keys, Callback callback) {
        KgrDictionary params {
            { "path", path },
            { "text", text }
        };
        KgrArray array;
        for (const auto &key : keys) {
            array.Append(KgrDictionary {
                { "code", static_cast<int64_t>(key.first) },
                { "alt", key.second }
            });
        }
        params.Put("keys", std::move(array));
        this->pipe->Write(std::move(params));
        this->pipe->ReadWait();
        this->pipe->SetMessageListener([this, callback = std::move(callback)] {
            while (!this->pipe->Empty()) {
                auto msg = this->pipe->Read();
                if (callback) {
                    SlokedKeyboardInput event;
                    event.alt = msg.AsDictionary()["alt"].AsBoolean();
                    if (msg.AsDictionary().Has("text")) {
                        event.value = msg.AsDictionary()["text"].AsString();
                    } else if (msg.AsDictionary().Has("key")) {
                        event.value = static_cast<SlokedControlKey>(msg.AsDictionary()["key"].AsInt());
                    }
                    callback(event);
                }
            }
        });
    }

    void SlokedScreenInputNotificationClient::Close() {
        this->pipe->Write({});
        this->pipe = nullptr;
    }
}