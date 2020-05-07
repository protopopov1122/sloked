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

#include "sloked/services/TextPane.h"

#include <set>

#include "sloked/core/Locale.h"
#include "sloked/sched/CompoundTask.h"
#include "sloked/screen/components/ComponentTree.h"
#include "sloked/screen/components/TextPane.h"

namespace sloked {

    enum class RenderCommand {
        SetPosition,
        MoveUp,
        MoveDown,
        MoveBackward,
        MoveForward,
        ShowCursor,
        ClearScreen,
        ClearChars,
        Write,
        SetTextGraphics,
        SetForegroundGraphics,
        SetBackgroundGraphics
    };

    class SlokedTextPaneImpl : public SlokedTextPaneWidget {
     public:
        class Frame {
         public:
            void Reset() {
                std::unique_lock<std::mutex> lock(this->mtx);
                this->input.clear();
                this->catchedKeys.clear();
                this->catchText = false;
            }

            void SubscribeOnText(bool ctext) {
                std::unique_lock<std::mutex> lock(this->mtx);
                this->catchText = ctext;
            }

            void Subscribe(SlokedControlKey key, bool alt) {
                std::unique_lock<std::mutex> lock(this->mtx);
                this->catchedKeys.emplace(std::make_pair(key, alt));
            }

            void Unsubscribe() {
                std::unique_lock<std::mutex> lock(this->mtx);
                this->catchText = false;
                this->catchedKeys.clear();
            }

            bool ProcessInput(const SlokedKeyboardInput &event) {
                std::unique_lock<std::mutex> lock(this->mtx);
                if ((event.value.index() == 0 && this->catchText) ||
                    (event.value.index() == 1 &&
                     this->catchedKeys.find(
                         std::make_pair(std::get<1>(event.value), event.alt)) !=
                         this->catchedKeys.end())) {
                    this->input.push_back(event);
                    return true;
                } else {
                    return false;
                }
            }

            std::vector<SlokedKeyboardInput> GetInput() {
                std::unique_lock<std::mutex> lock(this->mtx);
                auto input = std::move(this->input);
                this->input.clear();
                return input;
            }

            void SetRender(const KgrArray &render) {
                std::unique_lock<std::mutex> lock(this->mtx);
                this->render = std::make_shared<KgrArray>(std::move(render));
                if (this->listener) {
                    this->listener();
                }
            }

            std::shared_ptr<KgrArray> GetRender() {
                std::unique_lock<std::mutex> lock(this->mtx);
                return this->render;
            }

            void OnUpdate(std::function<void()> listener) {
                this->listener = listener;
            }

         private:
            std::vector<SlokedKeyboardInput> input;
            std::set<std::pair<SlokedControlKey, bool>> catchedKeys;
            bool catchText;
            std::mutex mtx;
            std::shared_ptr<KgrArray> render;
            std::function<void()> listener;
        };

        SlokedTextPaneImpl(Frame &frame) : frame(frame) {}

        bool ProcessInput(const SlokedKeyboardInput &event) override {
            return this->frame.ProcessInput(event);
        }

        TaskResult<void> RenderSurface(SlokedGraphicsPoint::Coordinate,
                                       TextPosition::Line,
                                       const SlokedFontProperties &) override {
            return TaskResult<void>::Resolve();
        }

        void ShowSurface(SlokedTextPane &pane) override {
            auto render = this->frame.GetRender();
            if (render == nullptr) {
                return;
            }
            for (const auto &cmd : *render) {
                RenderCommand command = static_cast<RenderCommand>(
                    cmd.AsDictionary()["command"].AsInt());
                switch (command) {
                    case RenderCommand::SetPosition:
                        pane.SetPosition(
                            static_cast<SlokedGraphicsPoint::Coordinate>(
                                cmd.AsDictionary()["line"].AsInt()),
                            static_cast<SlokedGraphicsPoint::Coordinate>(
                                cmd.AsDictionary()["column"].AsInt()));
                        break;

                    case RenderCommand::MoveUp:
                        pane.MoveUp(
                            static_cast<SlokedGraphicsPoint::Coordinate>(
                                cmd.AsDictionary()["line"].AsInt()));
                        break;

                    case RenderCommand::MoveDown:
                        pane.MoveDown(
                            static_cast<SlokedGraphicsPoint::Coordinate>(
                                cmd.AsDictionary()["line"].AsInt()));
                        break;

                    case RenderCommand::MoveBackward:
                        pane.MoveBackward(
                            static_cast<SlokedGraphicsPoint::Coordinate>(
                                cmd.AsDictionary()["column"].AsInt()));
                        break;

                    case RenderCommand::MoveForward:
                        pane.MoveForward(
                            static_cast<SlokedGraphicsPoint::Coordinate>(
                                cmd.AsDictionary()["column"].AsInt()));
                        break;

                    case RenderCommand::ShowCursor:
                        pane.ShowCursor(cmd.AsDictionary()["show"].AsBoolean());
                        break;

                    case RenderCommand::ClearScreen:
                        pane.ClearScreen();
                        break;

                    case RenderCommand::ClearChars:
                        pane.ClearArea(
                            {static_cast<SlokedGraphicsPoint::Coordinate>(
                                 cmd.AsDictionary()["line"].AsInt()),
                             static_cast<SlokedGraphicsPoint::Coordinate>(
                                 cmd.AsDictionary()["column"].AsInt())});
                        break;

                    case RenderCommand::Write:
                        pane.Write(cmd.AsDictionary()["text"].AsString());
                        break;

                    case RenderCommand::SetTextGraphics:
                        pane.SetGraphicsMode(static_cast<SlokedTextGraphics>(
                            cmd.AsDictionary()["mode"].AsInt()));
                        break;

                    case RenderCommand::SetForegroundGraphics:
                        pane.SetGraphicsMode(
                            static_cast<SlokedForegroundGraphics>(
                                cmd.AsDictionary()["mode"].AsInt()));
                        break;

                    case RenderCommand::SetBackgroundGraphics:
                        pane.SetGraphicsMode(
                            static_cast<SlokedBackgroundGraphics>(
                                cmd.AsDictionary()["mode"].AsInt()));
                        break;
                }
            }
        }

        void OnUpdate(std::function<void()> listener) override {
            this->frame.OnUpdate(std::move(listener));
        }

     private:
        Frame &frame;
    };

    class SlokedTextPaneContext : public SlokedServiceContext {
     public:
        SlokedTextPaneContext(std::unique_ptr<KgrPipe> pipe,
                              SlokedMonitor<SlokedScreenComponent &> &root,
                              const Encoding &encoding)
            : SlokedServiceContext(std::move(pipe)), root(root),
              encoding(encoding) {

            this->BindMethod("connect", &SlokedTextPaneContext::Connect);
            this->BindMethod("close", &SlokedTextPaneContext::Close);
            this->BindMethod("getMaxWidth",
                             &SlokedTextPaneContext::GetMaxWidth);
            this->BindMethod("getHeight", &SlokedTextPaneContext::GetHeight);
            this->BindMethod("getCharWidth",
                             &SlokedTextPaneContext::GetCharWidth);
            this->BindMethod("getCharHeight",
                             &SlokedTextPaneContext::GetCharHeight);
            this->BindMethod("render", &SlokedTextPaneContext::Render);
            this->BindMethod("getInput", &SlokedTextPaneContext::GetInput);
        }

        virtual ~SlokedTextPaneContext() {
            this->frame.Reset();
            if (this->path.has_value()) {
                this->root.Lock([&](auto &component) {
                    SlokedComponentTree::Traverse(component, this->path.value())
                        .AsHandle()
                        .Close();
                });
            }
        }

     private:
        void RootLock(std::function<void(SlokedScreenComponent &)> callback) {
            if (!this->root.TryLock(callback)) {
                this->Defer([this, callback = std::move(callback)] {
                    this->RootLock(callback);
                    return false;
                });
            }
        }

        void Connect(const std::string &method, const KgrValue &params,
                     Response &rsp) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            this->frame.Unsubscribe();
            RootLock([this, path, params,
                      rsp = std::move(rsp)](auto &component) mutable {
                try {
                    if (this->path.has_value()) {
                        SlokedComponentTree::Traverse(component,
                                                      this->path.value())
                            .AsHandle()
                            .Close();
                    }
                    this->frame.Reset();
                    SlokedComponentTree::Traverse(component, path)
                        .AsHandle()
                        .NewTextPane(
                            std::make_unique<SlokedTextPaneImpl>(frame));
                    this->path = path;
                    this->frame.SubscribeOnText(
                        params.AsDictionary()["text"].AsBoolean());
                    const auto &keys = params.AsDictionary()["keys"].AsArray();
                    for (const auto &key : keys) {
                        auto code = static_cast<SlokedControlKey>(
                            key.AsDictionary()["code"].AsInt());
                        auto alt = key.AsDictionary()["alt"].AsBoolean();
                        this->frame.Subscribe(code, alt);
                    }
                    rsp.Result(true);
                } catch (const SlokedError &err) { rsp.Result(false); }
            });
        }

        void GetMaxWidth(const std::string &method, const KgrValue &params,
                         Response &rsp) {
            if (this->path.has_value()) {
                RootLock([this, params,
                          rsp = std::move(rsp)](auto &component) mutable {
                    auto &cmp = SlokedComponentTree::Traverse(
                        component, this->path.value());
                    rsp.Result(
                        static_cast<int64_t>(cmp.GetDimensions().column));
                });
            }
        }

        void GetHeight(const std::string &method, const KgrValue &params,
                       Response &rsp) {
            if (this->path.has_value()) {
                RootLock([this, rsp = std::move(rsp)](auto &component) mutable {
                    auto &cmp = SlokedComponentTree::Traverse(
                        component, this->path.value());
                    rsp.Result(static_cast<int64_t>(cmp.GetDimensions().line));
                });
            }
        }

        void GetCharWidth(const std::string &method, const KgrValue &params,
                          Response &rsp) {
            if (this->path.has_value()) {
                RootLock([this, params,
                          rsp = std::move(rsp)](auto &component) mutable {
                    auto &cmp = SlokedComponentTree::Traverse(
                        component, this->path.value());
                    std::vector<char32_t> graphemes;
                    for (auto codepoint : params.AsArray()) {
                        graphemes.push_back(codepoint.AsInt());
                    }
                    rsp.Result(static_cast<int64_t>(cmp.AsHandle()
                                                        .GetComponent()
                                                        .AsTextPane()
                                                        .GetFontProperties()
                                                        .GetWidth(SlokedSpan(graphemes.data(), graphemes.size()))));
                });
            }
        }

        void GetCharHeight(const std::string &method, const KgrValue &params,
                           Response &rsp) {
            if (this->path.has_value()) {
                RootLock([this, rsp = std::move(rsp)](auto &component) mutable {
                    auto &cmp = SlokedComponentTree::Traverse(
                        component, this->path.value());
                    rsp.Result(static_cast<int64_t>(cmp.AsHandle()
                                                        .GetComponent()
                                                        .AsTextPane()
                                                        .GetFontProperties()
                                                        .GetHeight()));
                });
            }
        }

        void Render(const std::string &method, const KgrValue &params,
                    Response &rsp) {
            this->frame.SetRender(params.AsArray());
        }

        void Close(const std::string &method, const KgrValue &params,
                   Response &rsp) {
            this->frame.Reset();
            if (this->path.has_value()) {
                RootLock([this, params](auto &component) {
                    SlokedComponentTree::Traverse(component, this->path.value())
                        .AsHandle()
                        .Close();
                });
            }
        }

        void GetInput(const std::string &method, const KgrValue &params,
                      Response &rsp) {
            if (!this->path.has_value()) {
                rsp.Result({});
                return;
            }
            EncodingConverter conv(this->encoding,
                                   SlokedLocale::SystemEncoding());
            auto input = this->frame.GetInput();
            KgrArray array;
            for (const auto &in : input) {
                if (in.value.index() == 0) {
                    array.Append(KgrDictionary{
                        {"alt", in.alt},
                        {"text", conv.Convert(std::get<0>(in.value))}});
                } else {
                    array.Append(KgrDictionary{
                        {"alt", in.alt},
                        {"key", static_cast<int64_t>(std::get<1>(in.value))}});
                }
            }
            rsp.Result(std::move(array));
        }

        SlokedMonitor<SlokedScreenComponent &> &root;
        const Encoding &encoding;
        SlokedTextPaneImpl::Frame frame;
        std::optional<SlokedPath> path;
    };

    SlokedTextPaneService::SlokedTextPaneService(
        SlokedMonitor<SlokedScreenComponent &> &root, const Encoding &encoding,
        KgrContextManager<KgrLocalContext> &contextManager)
        : root(root), encoding(encoding), contextManager(contextManager) {}

    TaskResult<void> SlokedTextPaneService::Attach(
        std::unique_ptr<KgrPipe> pipe) {
        TaskResultSupplier<void> supplier;
        supplier.Wrap([&] {
            auto ctx = std::make_unique<SlokedTextPaneContext>(
                std::move(pipe), this->root, this->encoding);
            this->contextManager.Attach(std::move(ctx));
        });
        return supplier.Result();
    }

    class SlokedTextPaneClient::SlokedTextPaneRender
        : public SlokedTextPaneClient::Render {
     public:
        class VisualPreset : public SlokedFontProperties {
         public:
            VisualPreset(SlokedServiceClient &client) : client(client) {}

            SlokedGraphicsPoint::Coordinate GetWidth(
                SlokedSpan<const char32_t> graphemes) const final {
                KgrArray graphemeArray;
                for (std::size_t i = 0; i < graphemes.Size(); i++) {
                    graphemeArray.Append(static_cast<int64_t>(graphemes[i]));
                }
                auto res =
                    this->client
                        .Invoke("getCharWidth", std::move(graphemeArray))
                        ->Next()
                        .UnwrapWait();
                if (res.HasResult() &&
                    res.GetResult().Is(KgrValueType::Integer)) {
                    return static_cast<SlokedGraphicsPoint::Coordinate>(
                        res.GetResult().AsInt());
                } else {
                    return 0;
                }
            }

            SlokedGraphicsPoint::Coordinate GetHeight() const final {
                auto res = this->client.Invoke("getCharHeight", {})
                               ->Next()
                               .UnwrapWait();
                if (res.HasResult() &&
                    res.GetResult().Is(KgrValueType::Integer)) {
                    return static_cast<SlokedGraphicsPoint::Coordinate>(
                        res.GetResult().AsInt());
                } else {
                    return 0;
                }
            }

         private:
            SlokedServiceClient &client;
        };

        SlokedTextPaneRender(SlokedServiceClient &client)
            : client(client), fontProperties(client) {}

        void SetPosition(TextPosition::Line line,
                         TextPosition::Column column) override {
            this->renderCommands.Append(KgrDictionary{
                {"command", static_cast<int64_t>(RenderCommand::SetPosition)},
                {"line", static_cast<int64_t>(line)},
                {"column", static_cast<int64_t>(column)}});
        }

        void MoveUp(TextPosition::Line line) override {
            this->renderCommands.Append(KgrDictionary{
                {"command", static_cast<int64_t>(RenderCommand::MoveUp)},
                {"line", static_cast<int64_t>(line)}});
        }

        void MoveDown(TextPosition::Line line) override {
            this->renderCommands.Append(KgrDictionary{
                {"command", static_cast<int64_t>(RenderCommand::MoveDown)},
                {"line", static_cast<int64_t>(line)}});
        }

        void MoveBackward(TextPosition::Column column) override {
            this->renderCommands.Append(KgrDictionary{
                {"command", static_cast<int64_t>(RenderCommand::MoveBackward)},
                {"column", static_cast<int64_t>(column)}});
        }

        void MoveForward(TextPosition::Column column) override {
            this->renderCommands.Append(KgrDictionary{
                {"command", static_cast<int64_t>(RenderCommand::MoveForward)},
                {"column", static_cast<int64_t>(column)}});
        }

        void ShowCursor(bool b) override {
            this->renderCommands.Append(KgrDictionary{
                {"command", static_cast<int64_t>(RenderCommand::ShowCursor)},
                {"show", b}});
        }

        void ClearScreen() override {
            this->renderCommands.Append(KgrDictionary{
                {"command", static_cast<int64_t>(RenderCommand::ClearScreen)}});
        }

        void ClearArea(TextPosition range) override {
            this->renderCommands.Append(KgrDictionary{
                {"command", static_cast<int64_t>(RenderCommand::ClearChars)},
                {"line", static_cast<int64_t>(range.line)},
                {"column", static_cast<int64_t>(range.column)}});
        }

        SlokedGraphicsPoint::Coordinate GetMaxWidth() override {
            auto res =
                this->client.Invoke("getMaxWidth", {})->Next().UnwrapWait();
            if (res.HasResult() && res.GetResult().Is(KgrValueType::Integer)) {
                return static_cast<SlokedGraphicsPoint::Coordinate>(
                    res.GetResult().AsInt());
            } else {
                return 0;
            }
        }

        TextPosition::Line GetHeight() override {
            auto res =
                this->client.Invoke("getHeight", {})->Next().UnwrapWait();
            if (res.HasResult() && res.GetResult().Is(KgrValueType::Integer)) {
                return static_cast<SlokedGraphicsPoint::Coordinate>(
                    res.GetResult().AsInt());
            } else {
                return 0;
            }
        }

        const SlokedFontProperties &GetFontProperties() const override {
            return this->fontProperties;
        }

        void Write(const std::string &text) override {
            this->renderCommands.Append(KgrDictionary{
                {"command", static_cast<int64_t>(RenderCommand::Write)},
                {"text", text}});
        }

        void SetGraphicsMode(SlokedTextGraphics mode) override {
            this->renderCommands.Append(KgrDictionary{
                {"command",
                 static_cast<int64_t>(RenderCommand::SetTextGraphics)},
                {"mode", static_cast<int64_t>(mode)}});
        }

        void SetGraphicsMode(SlokedBackgroundGraphics mode) override {
            this->renderCommands.Append(KgrDictionary{
                {"command",
                 static_cast<int64_t>(RenderCommand::SetBackgroundGraphics)},
                {"mode", static_cast<int64_t>(mode)}});
        }

        void SetGraphicsMode(SlokedForegroundGraphics mode) override {
            this->renderCommands.Append(KgrDictionary{
                {"command",
                 static_cast<int64_t>(RenderCommand::SetForegroundGraphics)},
                {"mode", static_cast<int64_t>(mode)}});
        }

        void Reset() override {
            this->renderCommands = {};
        }

        void Flush() override {
            this->client.Invoke("render", std::move(this->renderCommands));
            this->renderCommands = {};
        }

     private:
        SlokedServiceClient &client;
        KgrArray renderCommands;
        VisualPreset fontProperties;
    };

    SlokedTextPaneClient::SlokedTextPaneClient(std::unique_ptr<KgrPipe> pipe)
        : client(std::move(pipe)),
          render(std::make_unique<SlokedTextPaneRender>(client)) {}

    SlokedTextPaneClient::~SlokedTextPaneClient() = default;

    TaskResult<bool> SlokedTextPaneClient::Connect(
        const std::string &path, bool text,
        const std::vector<std::pair<SlokedControlKey, bool>> &keys) {
        KgrDictionary params{{"path", path}, {"text", text}};
        KgrArray array;
        for (const auto &key : keys) {
            array.Append(
                KgrDictionary{{"code", static_cast<int64_t>(key.first)},
                              {"alt", key.second}});
        }
        params.Put("keys", std::move(array));
        return SlokedTaskTransformations::Transform(
            this->client.Invoke("connect", std::move(params))->Next(),
            [](const SlokedNetResponseBroker::Response &res) {
                return res.HasResult() && res.GetResult().AsBoolean();
            });
    }

    void SlokedTextPaneClient::Close() {
        this->client.Invoke("close", {});
    }

    SlokedTextPaneClient::Render &SlokedTextPaneClient::GetRender() {
        return *this->render;
    }

    TaskResult<std::vector<SlokedKeyboardInput>>
        SlokedTextPaneClient::GetInput() {
        return SlokedTaskTransformations::Transform(
            this->client.Invoke("getInput", {})->Next(),
            [](const SlokedNetResponseBroker::Response &res) {
                std::vector<SlokedKeyboardInput> input;
                if (res.HasResult() &&
                    res.GetResult().Is(KgrValueType::Array)) {
                    const auto &array = res.GetResult().AsArray();
                    for (const auto &in : array) {
                        SlokedKeyboardInput event;
                        event.alt = in.AsDictionary()["alt"].AsBoolean();
                        if (in.AsDictionary().Has("text")) {
                            event.value = in.AsDictionary()["text"].AsString();
                        } else if (in.AsDictionary().Has("key")) {
                            event.value = static_cast<SlokedControlKey>(
                                in.AsDictionary()["key"].AsInt());
                        } else {
                            continue;
                        }
                        input.push_back(event);
                    }
                }
                return input;
            });
    }
}  // namespace sloked