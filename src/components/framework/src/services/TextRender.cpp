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

#include "sloked/services/TextRender.h"

#include "sloked/core/Error.h"
#include "sloked/core/Locale.h"
#include "sloked/core/OrderedCache.h"
#include "sloked/sched/CompoundTask.h"
#include "sloked/text/cursor/TransactionJournal.h"
#include "sloked/text/fragment/Updater.h"

namespace sloked {

    class SlokedTextRenderContext : public SlokedServiceContext {
     public:
        SlokedTextRenderContext(std::unique_ptr<KgrPipe> pipe,
                                SlokedEditorDocumentSet &documents)
            : SlokedServiceContext(std::move(pipe)), documents(documents),
              handle(documents.Empty()), document(nullptr),
              cache([this](const auto &begin, const auto &end) {
                  return this->RenderLines(begin, end);
              }) {

            this->BindMethod("attach", &SlokedTextRenderContext::Attach);
            this->BindMethod("render", &SlokedTextRenderContext::Render);
        }

     protected:
        void Attach(const std::string &method, const KgrValue &params,
                    Response &rsp) {
            auto doc = this->documents.OpenDocument(
                static_cast<SlokedEditorDocumentSet::DocumentId>(
                    params.AsDictionary()["document"].AsInt()));
            if (doc.has_value()) {
                std::optional<std::string> externalUri;
                if (doc.value().Exists() &&
                    doc.value().GetObject().HasUpstream()) {
                    externalUri = doc.value().GetObject().GetUpstreamURI();
                }
                this->handle = std::move(doc.value());
                this->document =
                    std::make_unique<DocumentContent>(this->handle.GetObject());
            }
        }

        void Render(const std::string &method, const KgrValue &params,
                    Response &rsp) {
            const auto &dim = params.AsDictionary();
            auto lineNumber =
                static_cast<TextPosition::Line>(dim["line"].AsInt());
            auto height =
                static_cast<TextPosition::Line>(dim["height"].AsInt());

            std::unique_lock lock(this->document->mtx);
            for (auto &range : this->document->invalidated) {
                this->cache.Drop(range.start.line, range.end.line);
            }
            this->document->invalidated.clear();
            lock.unlock();

            auto maxLineNumber = std::min(
                lineNumber + height, static_cast<TextPosition::Line>(
                                         this->document->text.GetLastLine()));
            bool partial_render = params.AsDictionary().Has("partial") &&
                                  params.AsDictionary()["partial"].AsBoolean();
            if (!partial_render) {
                KgrArray lines;
                auto [begin, end] =
                    this->cache.Fetch(lineNumber, maxLineNumber);
                for (auto it = begin; it != end; ++it) {
                    lines.Append(it->second);
                }
                rsp.Result(
                    KgrDictionary{{"start", static_cast<int64_t>(lineNumber)},
                                  {"end", static_cast<int64_t>(maxLineNumber)},
                                  {"content", std::move(lines)}});
            } else {
                auto updated =
                    this->cache.FetchUpdated(lineNumber, maxLineNumber);
                KgrArray lines;
                for (auto &it : updated) {
                    lines.Append(
                        KgrDictionary{{"line", static_cast<int64_t>(it.first)},
                                      {"content", it.second->second}});
                }
                rsp.Result(
                    KgrDictionary{{"start", static_cast<int64_t>(lineNumber)},
                                  {"end", static_cast<int64_t>(maxLineNumber)},
                                  {"content", std::move(lines)}});
            }
        }

     private:
        std::vector<KgrValue> RenderLines(const TextPosition::Line &begin,
                                          const TextPosition::Line &end) {
            std::vector<KgrValue> result;
            auto lineIdx = begin;
            this->document->text.Visit(
                begin,
                std::min(end - begin + 1,
                         static_cast<TextPosition::Line>(
                             this->document->text.GetLastLine() - begin + 1)),
                [&](auto line) {
                    std::optional<std::pair<
                        std::string, std::optional<TaggedTextFragment<
                                         SlokedEditorDocument::TagType>>>>
                        back;
                    KgrArray fragments;
                    TextPosition::Column columnIdx{0};
                    auto lineTags = this->document->tags.Get(lineIdx);
                    std::string_view::const_iterator buffer_begin =
                        line.begin();
                    const TaggedTextFragment<SlokedEditorDocument::TagType>
                        *current{nullptr};
                    auto iter = lineTags.begin();
                    const auto length = line.size();
                    for (Encoding::Iterator it{};
                         this->document->encoding.Iterate(it, line, length);) {
                        const auto current_iter =
                            std::next(line.begin(), it.start);
                        if (current != nullptr &&
                            current->GetEnd().column <= columnIdx) {
                            const std::size_t buffer_size =
                                std::distance(buffer_begin, current_iter);
                            if (buffer_size > 0) {
                                fragments.Append(KgrDictionary{
                                    {"tag", current != nullptr},
                                    {"content",
                                     this->document->conv.ReverseConvert(
                                         std::string_view{buffer_begin,
                                                          buffer_size})}});
                                buffer_begin = current_iter;
                            }
                            current = nullptr;
                        }
                        if (iter != lineTags.end() &&
                            iter->GetStart().column == columnIdx) {
                            const std::size_t buffer_size =
                                std::distance(buffer_begin, current_iter);
                            if (std::distance(buffer_begin, current_iter) > 0) {
                                fragments.Append(KgrDictionary{
                                    {"tag", current != nullptr},
                                    {"content",
                                     this->document->conv.ReverseConvert(
                                         std::string_view{buffer_begin,
                                                          buffer_size})}});
                                buffer_begin = current_iter;
                            }
                            current = std::addressof(*iter);
                            ++iter;
                        }
                        columnIdx++;
                    }
                    const std::size_t buffer_size =
                        std::distance(buffer_begin, line.end());
                    if (buffer_size > 0) {
                        fragments.Append(KgrDictionary{
                            {"tag", current != nullptr},
                            {"content", this->document->conv.ReverseConvert(
                                            std::string_view{buffer_begin,
                                                             buffer_size})}});
                    }
                    lineIdx++;
                    result.emplace_back(std::move(fragments));
                });
            return result;
        }

        struct DocumentContent {
            class TransactionListener : public SlokedTransactionStreamListener {
             public:
                TransactionListener(DocumentContent &content)
                    : content(content) {}

                void OnCommit(const SlokedCursorTransaction &trans) final {
                    this->Process(trans);
                }

                void OnRollback(const SlokedCursorTransaction &trans) final {
                    this->Process(trans);
                }

                void OnRevert(const SlokedCursorTransaction &trans) final {
                    this->Process(trans);
                }

             private:
                void Process(const SlokedCursorTransaction &trans) {
                    const auto &pos = trans.GetPosition();
                    auto patch = trans.CommitPatch(this->content.encoding);
                    if (patch.At(pos).line != 0) {
                        std::unique_lock lock(this->content.mtx);
                        this->content.invalidated.emplace_back(
                            TextPositionRange{pos, TextPosition::Max});
                    } else {
                        std::unique_lock lock(this->content.mtx);
                        this->content.invalidated.emplace_back(
                            TextPositionRange{
                                {pos.line, 0},
                                {pos.line, TextPosition::Max.column}});
                    }
                }

                DocumentContent &content;
            };

            DocumentContent(SlokedEditorDocument &document)
                : text(document.GetText()), encoding(document.GetEncoding()),
                  conv(SlokedLocale::SystemEncoding(), document.GetEncoding()),
                  transactionListeners(document.GetTransactionListeners()),
                  tags(document.GetTagger()) {

                this->unsubscribeTaggers =
                    document.GetTagger().OnChange([this](const auto &range) {
                        std::unique_lock lock(this->mtx);
                        this->invalidated.push_back(range);
                    });

                this->transListener =
                    std::make_shared<TransactionListener>(*this);
                this->transactionListeners.AddListener(this->transListener);
            }

            ~DocumentContent() {
                this->transactionListeners.RemoveListener(*this->transListener);
                if (this->unsubscribeTaggers) {
                    this->unsubscribeTaggers();
                }
            }

            TextBlock &text;
            const Encoding &encoding;
            EncodingConverter conv;
            SlokedTransactionListenerManager &transactionListeners;
            SlokedTextTagger<SlokedEditorDocument::TagType> &tags;
            SlokedTextTagger<SlokedEditorDocument::TagType>::Unbind
                unsubscribeTaggers;
            std::shared_ptr<TransactionListener> transListener;
            std::mutex mtx;
            std::vector<TextPositionRange> invalidated;
        };

        SlokedEditorDocumentSet &documents;
        SlokedEditorDocumentSet::Document handle;
        std::unique_ptr<DocumentContent> document;
        SlokedOrderedCache<TextPosition::Line, KgrValue> cache;
    };

    SlokedTextRenderService::SlokedTextRenderService(
        SlokedEditorDocumentSet &documents,
        KgrContextManager<KgrLocalContext> &contextManager)
        : documents(documents), contextManager(contextManager) {}

    TaskResult<void> SlokedTextRenderService::Attach(
        std::unique_ptr<KgrPipe> pipe) {
        TaskResultSupplier<void> supplier;
        supplier.Wrap([&] {
            auto ctx = std::make_unique<SlokedTextRenderContext>(
                std::move(pipe), documents);
            this->contextManager.Attach(std::move(ctx));
        });
        return supplier.Result();
    }

    SlokedTextRenderClient::SlokedTextRenderClient(
        std::unique_ptr<KgrPipe> pipe,
        SlokedEditorDocumentSet::DocumentId docId)
        : client(std::move(pipe)) {
        this->client.Invoke(
            "attach", KgrDictionary{{"document", static_cast<int64_t>(docId)}});
    }

    TaskResult<std::tuple<TextPosition::Line, TextPosition::Line, KgrValue>>
        SlokedTextRenderClient::Render(TextPosition::Line line,
                                       TextPosition::Line height) {
        return SlokedTaskTransformations::Transform(
            this->client
                .Invoke("render",
                        KgrDictionary{{"height", static_cast<int64_t>(height)},
                                      {"line", static_cast<int64_t>(line)}})
                ->Next(),
            [](const SlokedNetResponseBroker::Response &renderRes)
                -> std::tuple<TextPosition::Line, TextPosition::Line,
                              KgrValue> {
                const auto &result = renderRes.GetResult().AsDictionary();
                return {result["start"].AsInt(), result["end"].AsInt(),
                        result["content"]};
            });
    }

    TaskResult<std::tuple<TextPosition::Line, TextPosition::Line,
                          std::vector<std::pair<TextPosition::Line, KgrValue>>>>
        SlokedTextRenderClient::PartialRender(TextPosition::Line line,
                                              TextPosition::Line height) {
        return SlokedTaskTransformations::Transform(
            this->client
                .Invoke("render",
                        KgrDictionary{{"height", static_cast<int64_t>(height)},
                                      {"line", static_cast<int64_t>(line)},
                                      {"partial", true}})
                ->Next(),
            [](const SlokedNetResponseBroker::Response &renderRes)
                -> std::tuple<
                    TextPosition::Line, TextPosition::Line,
                    std::vector<std::pair<TextPosition::Line, KgrValue>>> {
                if (renderRes.HasResult()) {
                    const auto &res = renderRes.GetResult().AsDictionary();
                    std::vector<std::pair<TextPosition::Line, KgrValue>> result;
                    for (const auto &line : res["content"].AsArray()) {
                        result.emplace_back(
                            std::make_pair(line.AsDictionary()["line"].AsInt(),
                                           line.AsDictionary()["content"]));
                    }
                    return {res["start"].AsInt(), res["end"].AsInt(),
                            std::move(result)};
                } else {
                    return {};
                }
            });
    }
}  // namespace sloked