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

#include "sloked/services/ScreenInput.h"

#include <set>

#include "sloked/core/Locale.h"
#include "sloked/sched/CompoundTask.h"
#include "sloked/sched/EventLoop.h"
#include "sloked/screen/components/ComponentTree.h"

namespace sloked {

    static KgrValue EventToKgr(const Encoding &encoding,
                               const SlokedKeyboardInput &evt) {
        EncodingConverter conv(encoding, SlokedLocale::SystemEncoding());
        if (evt.value.index() == 0) {
            return KgrDictionary{
                {"alt", evt.alt},
                {"text", conv.Convert(std::get<0>(evt.value))}};
        } else {
            return KgrDictionary{
                {"alt", evt.alt},
                {"key", static_cast<int64_t>(std::get<1>(evt.value))}};
        }
    }

    static SlokedKeyboardInput KgrToEvent(const Encoding &encoding,
                                          const KgrValue &msg) {
        EncodingConverter conv(SlokedLocale::SystemEncoding(), encoding);
        SlokedKeyboardInput event;
        event.alt = msg.AsDictionary()["alt"].AsBoolean();
        if (msg.AsDictionary().Has("text")) {
            event.value = conv.Convert(msg.AsDictionary()["text"].AsString());
        } else if (msg.AsDictionary().Has("key")) {
            event.value = static_cast<SlokedControlKey>(
                msg.AsDictionary()["key"].AsInt());
        }
        return event;
    }

    class SlokedScreenInputNotifierContext : public KgrLocalContext {
     public:
        SlokedScreenInputNotifierContext(
            std::unique_ptr<KgrPipe> pipe,
            SlokedMonitor<SlokedScreenComponent &> &root,
            const Encoding &encoding)
            : KgrLocalContext(std::move(pipe)), root(root), encoding(encoding) {
        }

        virtual ~SlokedScreenInputNotifierContext() {
            if (this->listener.has_value()) {
                this->root.Lock([&](auto &component) {
                    SlokedComponentTree::Traverse(component, this->path.value())
                        .DetachInputHandle(this->listener.value());
                });
            }
        }

        State GetState() const final {
            auto state = this->KgrLocalContext::GetState();
            if (state == State::Idle && this->eventLoop.HasPending()) {
                state = State::Pending;
            }
            return state;
        }

        void SetActivationListener(std::function<void()> callback) final {
            this->KgrLocalContext::SetActivationListener(callback);
            this->eventLoop.Notify(callback);
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
            if (this->eventLoop.HasPending()) {
                this->eventLoop.Run();
            }
        }

     private:
        void RootLock(std::function<void(SlokedScreenComponent &)> callback) {
            if (!this->root.TryLock(callback)) {
                this->eventLoop.Attach([this, callback = std::move(callback)] {
                    this->RootLock(callback);
                });
            }
        }

        void Connect(const KgrValue &params) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            this->forwardInput = false;
            this->subscribeAll = false;
            this->subscribeOnText = false;
            this->subscribes.clear();
            if (params.AsDictionary().Has("forward")) {
                this->forwardInput =
                    params.AsDictionary()["forward"].AsBoolean();
            }
            if (params.AsDictionary().Has("all")) {
                this->subscribeAll = params.AsDictionary()["all"].AsBoolean();
            }
            RootLock([this, path = std::move(path), params](auto &component) {
                if (this->listener.has_value()) {
                    SlokedComponentTree::Traverse(component, this->path.value())
                        .DetachInputHandle(this->listener.value());
                }
                this->listener =
                    SlokedComponentTree::Traverse(component, path)
                        .AsHandle()
                        .AttachInputHandler([this](const auto &evt) {
                            if (this->subscribeAll ||
                                (evt.value.index() == 0 &&
                                 this->subscribeOnText) ||
                                (evt.value.index() == 1 &&
                                 this->subscribes.find(std::make_pair(
                                     std::get<1>(evt.value), evt.alt)) !=
                                     this->subscribes.end())) {
                                this->SendInput(evt);
                                return !this->forwardInput;
                            } else {
                                return false;
                            }
                        });
                this->path = path;
                if (!this->subscribeAll) {
                    this->subscribeOnText =
                        params.AsDictionary()["text"].AsBoolean();
                    const auto &keys = params.AsDictionary()["keys"].AsArray();
                    for (const auto &key : keys) {
                        auto code = static_cast<SlokedControlKey>(
                            key.AsDictionary()["code"].AsInt());
                        auto alt = key.AsDictionary()["alt"].AsBoolean();
                        this->subscribes.emplace(std::make_pair(code, alt));
                    }
                }
                if (this->pipe->GetStatus() == KgrPipe::Status::Open) {
                    this->pipe->Write({});
                }
            });
        }

        void Close() {
            this->subscribeOnText = false;
            this->subscribes.clear();
            if (this->listener.has_value()) {
                RootLock([this](auto &component) {
                    SlokedComponentTree::Traverse(component, this->path.value())
                        .DetachInputHandle(this->listener.value());
                    this->listener.reset();
                    this->path.reset();
                });
            }
        }

        void SendInput(const SlokedKeyboardInput &evt) {
            if (this->pipe->GetStatus() == KgrPipe::Status::Open) {
                this->pipe->Write(EventToKgr(this->encoding, evt));
            }
        }

        SlokedMonitor<SlokedScreenComponent &> &root;
        const Encoding &encoding;
        std::optional<SlokedPath> path;
        std::optional<SlokedComponentListener> listener;
        bool forwardInput;
        bool subscribeAll;
        bool subscribeOnText;
        std::set<std::pair<SlokedControlKey, bool>> subscribes;
        SlokedDefaultEventLoop eventLoop;
    };

    SlokedScreenInputNotificationService::SlokedScreenInputNotificationService(
        SlokedMonitor<SlokedScreenComponent &> &root, const Encoding &encoding,
        KgrContextManager<KgrLocalContext> &contextManager)
        : root(root), encoding(encoding), contextManager(contextManager) {}

    TaskResult<void> SlokedScreenInputNotificationService::Attach(
        std::unique_ptr<KgrPipe> pipe) {
        TaskResultSupplier<void> supplier;
        supplier.Wrap([&] {
            auto ctx = std::make_unique<SlokedScreenInputNotifierContext>(
                std::move(pipe), this->root, this->encoding);
            this->contextManager.Attach(std::move(ctx));
        });
        return supplier.Result();
    }

    SlokedScreenInputNotificationClient::SlokedScreenInputNotificationClient(
        std::unique_ptr<KgrPipe> pipe, const Encoding &encoding,
        std::function<bool()> holdsLock)
        : asyncPipe(std::move(pipe)), encoding(encoding),
          holdsLock(std::move(holdsLock)) {}

    TaskResult<void> SlokedScreenInputNotificationClient::Listen(
        const std::string &path, bool text,
        const std::vector<std::pair<SlokedControlKey, bool>> &keys,
        Callback callback, bool forwardInput) {
        if (this->holdsLock && this->holdsLock()) {
            throw SlokedError(
                "ScreenInputNotificationService: Deadlock prevented");
        }
        KgrDictionary params{
            {"path", path}, {"forward", forwardInput}, {"text", text}};
        KgrArray array;
        for (const auto &key : keys) {
            array.Append(
                KgrDictionary{{"code", static_cast<int64_t>(key.first)},
                              {"alt", key.second}});
        }
        params.Put("keys", std::move(array));
        this->asyncPipe.Write(std::move(params));
        return SlokedTaskTransformations::Transform(
            this->asyncPipe.Read(),
            [this, callback = std::move(callback)](const KgrValue &) mutable {
                this->asyncPipe.SetMessageListener(
                    [this, callback = std::move(callback)] {
                        while (!this->asyncPipe.Empty()) {
                            auto msg = this->asyncPipe.Read().UnwrapWait();
                            if (callback) {
                                callback(KgrToEvent(this->encoding, msg));
                            }
                        }
                    });
            });
    }

    TaskResult<void> SlokedScreenInputNotificationClient::Listen(
        const std::string &path, Callback callback, bool forwardInput) {
        if (this->holdsLock && this->holdsLock()) {
            throw SlokedError(
                "ScreenInputNotificationService: Deadlock prevented");
        }
        KgrDictionary params{
            {"path", path}, {"forward", forwardInput}, {"all", true}};
        this->asyncPipe.Write(std::move(params));
        return SlokedTaskTransformations::Transform(
            this->asyncPipe.Read(),
            [this, callback = std::move(callback)](const KgrValue &) mutable {
                this->asyncPipe.SetMessageListener(
                    [this, callback = std::move(callback)] {
                        while (!this->asyncPipe.Empty()) {
                            auto msg = this->asyncPipe.Read().UnwrapWait();
                            if (callback) {
                                callback(KgrToEvent(this->encoding, msg));
                            }
                        }
                    });
            });
    }

    void SlokedScreenInputNotificationClient::Close() {
        this->asyncPipe.Write({});
        this->asyncPipe.Close();
    }

    class SlokedScreenInputForwardingContext : public SlokedServiceContext {
     public:
        SlokedScreenInputForwardingContext(
            std::unique_ptr<KgrPipe> pipe,
            SlokedMonitor<SlokedScreenComponent &> &root,
            const Encoding &encoding)
            : SlokedServiceContext(std::move(pipe)), root(root),
              encoding(encoding) {
            this->BindMethod("send", &SlokedScreenInputForwardingContext::Send);
        }

     private:
        void Send(const std::string &method, const KgrValue &params,
                  Response &rsp) {
            SlokedPath path(params.AsDictionary()["path"].AsString());
            auto event =
                KgrToEvent(this->encoding, params.AsDictionary()["event"]);
            this->Process(path, event);
        }

        void Process(const SlokedPath &path, const SlokedKeyboardInput &event) {
            if (!this->root.TryLock([&](auto &screen) {
                    SlokedComponentTree::Traverse(screen, path)
                        .ProcessInput(event);
                })) {
                this->Defer([this, path, event] {
                    this->Process(path, event);
                    return false;
                });
            }
        }

        SlokedMonitor<SlokedScreenComponent &> &root;
        const Encoding &encoding;
    };

    SlokedScreenInputForwardingService::SlokedScreenInputForwardingService(
        SlokedMonitor<SlokedScreenComponent &> &root, const Encoding &encoding,
        KgrContextManager<KgrLocalContext> &contextManager)
        : root(root), encoding(encoding), contextManager(contextManager) {}

    TaskResult<void> SlokedScreenInputForwardingService::Attach(
        std::unique_ptr<KgrPipe> pipe) {
        TaskResultSupplier<void> supplier;
        supplier.Wrap([&] {
            auto ctx = std::make_unique<SlokedScreenInputForwardingContext>(
                std::move(pipe), this->root, this->encoding);
            this->contextManager.Attach(std::move(ctx));
        });
        return supplier.Result();
    }

    SlokedScreenInputForwardingClient::SlokedScreenInputForwardingClient(
        std::unique_ptr<KgrPipe> pipe, const Encoding &encoding,
        std::function<bool()> holdsLock)
        : client(std::move(pipe)), encoding(encoding),
          holdsLock(std::move(holdsLock)) {}

    void SlokedScreenInputForwardingClient::Send(
        const std::string &path, const SlokedKeyboardInput &event) {
        if (this->holdsLock && this->holdsLock()) {
            throw SlokedError(
                "ScreenInputNotificationService: Deadlock prevented");
        }
        this->client.Invoke(
            "send",
            KgrDictionary{{"path", path},
                          {"event", EventToKgr(this->encoding, event)}});
    }
}  // namespace sloked