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

    class SlokedScreenInputContext : public SlokedServiceContext {
     public:
        SlokedScreenInputContext(std::unique_ptr<KgrPipe> pipe, SlokedSynchronized<SlokedScreenComponent &> &root, const Encoding &encoding)
            : SlokedServiceContext(std::move(pipe)), root(root), encoding(encoding) {
            
            this->BindMethod("connect", &SlokedScreenInputContext::Connect);
            this->BindMethod("close", &SlokedScreenInputContext::Close);
            this->BindMethod("getInput", &SlokedScreenInputContext::GetInput);
        }

        virtual ~SlokedScreenInputContext() {
            if (this->listener.has_value()) {
                root.Lock([&](auto &component) {
                    SlokedComponentTree::Traverse(component, this->path.value()).DetachInputHandle(this->listener.value());
                });
            }
        }
    
     private:
        void Connect(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            std::unique_lock<std::mutex> lock(this->mtx);
            this->input.clear();
            this->subscribeOnText = false;
            this->subscribes.clear();
            root.Lock([&](auto &component) {
                try {
                    if (this->listener.has_value()) {
                        SlokedComponentTree::Traverse(component, this->path.value()).DetachInputHandle(this->listener.value());
                    }
                    this->listener = SlokedComponentTree::Traverse(component, path).AsHandle().AttachInputHandler([this](const auto &evt) {
                        std::unique_lock<std::mutex> lock(this->mtx);
                        if ((evt.value.index() == 0 && this->subscribeOnText) ||
                            (evt.value.index() == 1 && this->subscribes.find(std::make_pair(std::get<1>(evt.value), evt.alt)) != this->subscribes.end())) {
                            this->input.push_back(evt);
                            while (this->input.size() > SlokedScreenInputService::MaxEvents) {
                                this->input.erase(this->input.begin());
                            }
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
                    rsp.Result(true);
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            });
        }

        void Close(const std::string &method, const KgrValue &params, Response &rsp) {
            std::unique_lock<std::mutex> lock(this->mtx);
            this->input.clear();
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

        void GetInput(const std::string &method, const KgrValue &params, Response &rsp) {
            if (!this->path.has_value()) {
                rsp.Result({});
                return;
            }
            EncodingConverter conv(this->encoding, SlokedLocale::SystemEncoding());
            std::unique_lock<std::mutex> lock(this->mtx);
            KgrArray array;
            for (const auto &in : input) {
                if (in.value.index() == 0) {
                    array.Append(KgrDictionary {
                        { "alt", in.alt },
                        { "text", conv.Convert(std::get<0>(in.value)) }
                    });
                } else {
                    array.Append(KgrDictionary {
                        { "alt", in.alt },
                        { "key", static_cast<int64_t>(std::get<1>(in.value)) }
                    });
                }
            }
            rsp.Result(std::move(array));
        }

        SlokedSynchronized<SlokedScreenComponent &> &root;
        const Encoding &encoding;
        std::optional<SlokedPath> path;
        std::optional<SlokedComponentListener> listener;
        std::mutex mtx;
        std::vector<SlokedKeyboardInput> input;
        bool subscribeOnText;
        std::set<std::pair<SlokedControlKey, bool>> subscribes;
    };

    SlokedScreenInputService::SlokedScreenInputService(SlokedSynchronized<SlokedScreenComponent &> &root, const Encoding &encoding, KgrContextManager<KgrLocalContext> &contextManager)
        : root(root), encoding(encoding), contextManager(contextManager) {}

    bool SlokedScreenInputService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedScreenInputContext>(std::move(pipe), this->root, this->encoding);
        this->contextManager.Attach(std::move(ctx));
        return true;
    }

    SlokedScreenInputClient::SlokedScreenInputClient(std::unique_ptr<KgrPipe> pipe)
        : client(std::move(pipe)) {}

    bool SlokedScreenInputClient::Connect(const std::string &path, bool text, const std::vector<std::pair<SlokedControlKey, bool>> &keys) {
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
        auto rsp = this->client.Invoke("connect", std::move(params));
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }

    void SlokedScreenInputClient::Close() {
        this->client.Invoke("close", {});
    }

    std::vector<SlokedKeyboardInput> SlokedScreenInputClient::GetInput() {
        auto rsp = this->client.Invoke("getInput", {});
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::Array)) {
            const auto &array = res.GetResult().AsArray();
            std::vector<SlokedKeyboardInput> input;
            for (const auto &in : array) {
                SlokedKeyboardInput event;
                event.alt = in.AsDictionary()["alt"].AsBoolean();
                if (in.AsDictionary().Has("text")) {
                    event.value = in.AsDictionary()["text"].AsString();
                } else if (in.AsDictionary().Has("key")) {
                    event.value = static_cast<SlokedControlKey>(in.AsDictionary()["key"].AsInt());
                } else {
                    continue;
                }
                input.push_back(event);
            }
            return input;
        } else {
            return {};
        }
    }
}