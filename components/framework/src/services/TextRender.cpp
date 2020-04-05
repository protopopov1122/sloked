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
#include "sloked/text/cursor/TransactionJournal.h"
#include "sloked/text/fragment/Updater.h"

namespace sloked {

    class SlokedTextRenderContext : public SlokedServiceContext {
     public:
        SlokedTextRenderContext(std::unique_ptr<KgrPipe> pipe,
                                SlokedEditorDocumentSet &documents,
                                const SlokedCharPreset &charPreset)
            : SlokedServiceContext(std::move(pipe)), documents(documents),
              charPreset(charPreset), handle(documents.Empty()),
              document(nullptr),
              cache([this](const auto &begin, const auto &end) {
                  return this->RenderLines(begin, end);
              }) {

            this->BindMethod("attach", &SlokedTextRenderContext::Attach);
            this->BindMethod("realPosition",
                             &SlokedTextRenderContext::RealPosition);
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
                this->document = std::make_unique<DocumentContent>(
                    this->handle.GetObject(), this->charPreset);
            }
        }

        void RealPosition(const std::string &method, const KgrValue &params,
                          Response &rsp) {
            TextPosition::Line line = params.AsDictionary()["line"].AsInt();
            TextPosition::Column column =
                params.AsDictionary()["column"].AsInt();
            auto realLine = this->document->text.GetLine(line);
            auto realColumnPos = this->charPreset.GetRealPosition(
                realLine, column, this->document->encoding);
            auto realColumn =
                column < this->document->encoding.CodepointCount(realLine)
                    ? realColumnPos.first
                    : realColumnPos.second;
            rsp.Result(
                KgrDictionary{{"line", static_cast<int64_t>(line)},
                              {"column", static_cast<int64_t>(realColumn)}});
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
                    std::size_t width{0};
                    auto lineTags = this->document->tags.Get(lineIdx);
                    std::string buffer{};
                    buffer.reserve(line.size());
                    const TaggedTextFragment<SlokedEditorDocument::TagType>
                        *current{nullptr};
                    auto iter = lineTags.begin();
                    const auto length = line.size();
                    for (Encoding::Iterator it{};
                         this->document->encoding.Iterate(it, line, length);) {
                        if (current != nullptr &&
                            current->GetEnd().column <= columnIdx) {
                            if (!buffer.empty()) {
                                fragments.Append(KgrDictionary{
                                    {"tag", current != nullptr},
                                    {"content",
                                     this->document->conv.ReverseConvert(
                                         buffer)}});
                                buffer.clear();
                            }
                            current = nullptr;
                        }
                        if (iter != lineTags.end() &&
                            iter->GetStart().column == columnIdx) {
                            if (!buffer.empty()) {
                                fragments.Append(KgrDictionary{
                                    {"tag", current != nullptr},
                                    {"content",
                                     this->document->conv.ReverseConvert(
                                         buffer)}});
                                buffer.clear();
                            }
                            current = std::addressof(*iter);
                            ++iter;
                        }
                        std::string_view fragment =
                            it.value != U'\t'
                                ? line.substr(it.start, it.length)
                                : this->charPreset.GetTab(
                                      this->document->encoding, width);
                        buffer += fragment;
                        width += this->charPreset.GetCharWidth(it.value, width);
                        columnIdx++;
                    }
                    if (!buffer.empty()) {
                        fragments.Append(KgrDictionary{
                            {"tag", current != nullptr},
                            {"content",
                             this->document->conv.ReverseConvert(buffer)}});
                    }
                    lineIdx++;
                    result.emplace_back(std::move(fragments));
                });
            return result;
        }

        struct DocumentContent {
            DocumentContent(SlokedEditorDocument &document,
                            const SlokedCharPreset &charPreset)
                : text(document.GetText()), encoding(document.GetEncoding()),
                  conv(SlokedLocale::SystemEncoding(), document.GetEncoding()),
                  transactionListeners(document.GetTransactionListeners()),
                  tags(document.GetTagger()) {

                this->unsubscribeTaggers =
                    document.GetTagger().OnChange([this](const auto &range) {
                        std::unique_lock lock(this->mtx);
                        this->invalidated.push_back(range);
                    });
            }

            ~DocumentContent() {
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
            std::mutex mtx;
            std::vector<TextPositionRange> invalidated;
        };

        SlokedEditorDocumentSet &documents;
        const SlokedCharPreset &charPreset;
        SlokedEditorDocumentSet::Document handle;
        std::unique_ptr<DocumentContent> document;
        SlokedOrderedCache<TextPosition::Line, KgrValue> cache;
    };

    SlokedTextRenderService::SlokedTextRenderService(
        SlokedEditorDocumentSet &documents, const SlokedCharPreset &charPreset,
        KgrContextManager<KgrLocalContext> &contextManager)
        : documents(documents), charPreset(charPreset),
          contextManager(contextManager) {}

    TaskResult<void> SlokedTextRenderService::Attach(
        std::unique_ptr<KgrPipe> pipe) {
        TaskResultSupplier<void> supplier;
        try {
            auto ctx = std::make_unique<SlokedTextRenderContext>(
                std::move(pipe), documents, this->charPreset);
            this->contextManager.Attach(std::move(ctx));
            supplier.SetResult();
        } catch (...) { supplier.SetError(std::current_exception()); }
        return supplier.Result();
    }

    SlokedTextRenderClient::SlokedTextRenderClient(
        std::unique_ptr<KgrPipe> pipe,
        SlokedEditorDocumentSet::DocumentId docId)
        : client(std::move(pipe)) {
        auto rsp = this->client.Invoke(
            "attach", KgrDictionary{{"document", static_cast<int64_t>(docId)}});
    }

    std::optional<TextPosition> SlokedTextRenderClient::RealPosition(
        TextPosition src) {
        auto rsp = this->client.Invoke(
            "realPosition",
            KgrDictionary{{"line", static_cast<int64_t>(src.line)},
                          {"column", static_cast<int64_t>(src.column)}});
        auto renderRes = rsp.Get();
        if (!renderRes.HasResult()) {
            return {};
        }
        return TextPosition{
            static_cast<TextPosition::Line>(
                renderRes.GetResult().AsDictionary()["line"].AsInt()),
            static_cast<TextPosition::Column>(
                renderRes.GetResult().AsDictionary()["column"].AsInt())};
    }

    std::tuple<TextPosition::Line, TextPosition::Line, KgrValue>
        SlokedTextRenderClient::Render(TextPosition::Line line,
                                       TextPosition::Line height) {
        auto rsp = this->client.Invoke(
            "render", KgrDictionary{{"height", static_cast<int64_t>(height)},
                                    {"line", static_cast<int64_t>(line)}});
        auto renderRes = rsp.Get();
        const auto &result = renderRes.GetResult().AsDictionary();
        return {result["start"].AsInt(), result["end"].AsInt(),
                result["content"]};
    }

    std::tuple<TextPosition::Line, TextPosition::Line,
               std::vector<std::pair<TextPosition::Line, KgrValue>>>
        SlokedTextRenderClient::PartialRender(TextPosition::Line line,
                                              TextPosition::Line height) {
        auto rsp = this->client.Invoke(
            "render", KgrDictionary{{"height", static_cast<int64_t>(height)},
                                    {"line", static_cast<int64_t>(line)},
                                    {"partial", true}});
        auto renderRes = rsp.Get();
        if (!renderRes.HasResult()) {
            return {};
        }
        const auto &res = renderRes.GetResult().AsDictionary();
        std::vector<std::pair<TextPosition::Line, KgrValue>> result;
        for (const auto &line : res["content"].AsArray()) {
            result.emplace_back(
                std::make_pair(line.AsDictionary()["line"].AsInt(),
                               line.AsDictionary()["content"]));
        }
        return {res["start"].AsInt(), res["end"].AsInt(), std::move(result)};
    }
}  // namespace sloked