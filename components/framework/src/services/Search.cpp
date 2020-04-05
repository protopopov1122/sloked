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

#include "sloked/services/Search.h"

#include "sloked/text/search/Match.h"
#include "sloked/text/search/Replace.h"

namespace sloked {

    class SlokedSearchContext : public SlokedServiceContext {
     public:
        SlokedSearchContext(std::unique_ptr<KgrPipe> pipe,
                            SlokedEditorDocumentSet &documents)
            : SlokedServiceContext(std::move(pipe)), documents(documents) {
            this->BindMethod("connect", &SlokedSearchContext::Connect);
            this->BindMethod("matcher", &SlokedSearchContext::Matcher);
            this->BindMethod("match", &SlokedSearchContext::Match);
            this->BindMethod("rewind", &SlokedSearchContext::Rewind);
            this->BindMethod("get", &SlokedSearchContext::GetResults);
            this->BindMethod("replace", &SlokedSearchContext::Replace);
            this->BindMethod("replaceAll", &SlokedSearchContext::ReplaceAll);
        }

     private:
        void Connect(const std::string &method, const KgrValue &params,
                     Response &rsp) {
            auto docId = static_cast<SlokedEditorDocumentSet::DocumentId>(
                params.AsInt());
            auto doc = this->documents.OpenDocument(docId);
            if (doc.has_value()) {
                this->document = std::move(doc.value());
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void Matcher(const std::string &method, const KgrValue &params,
                     Response &rsp) {
            if (this->document.has_value()) {
                auto &doc = this->document.value().GetObject();
                const std::string &type = params.AsString();
                if (type == "plain") {
                    this->matcher = std::make_unique<SlokedTextPlainMatcher>(
                        doc.GetText(), doc.GetEncoding());
                    this->replacer = std::make_unique<SlokedTextReplacer>(
                        doc.GetText(), doc.NewStream(), doc.GetEncoding());
                    rsp.Result(true);
                } else if (type == "regex") {
                    this->matcher = std::make_unique<SlokedTextRegexMatcher>(
                        doc.GetText(), doc.GetEncoding());
                    this->replacer = std::make_unique<SlokedTextReplacer>(
                        doc.GetText(), doc.NewStream(), doc.GetEncoding());
                    rsp.Result(true);
                } else {
                    rsp.Result(false);
                }
            } else {
                rsp.Result(false);
            }
        }

        void Match(const std::string &method, const KgrValue &params,
                   Response &rsp) {
            if (this->matcher != nullptr) {
                const std::string &query =
                    params.AsDictionary()["query"].AsString();
                SlokedTextMatcherBase::Flags flags =
                    params.AsDictionary()["flags"].AsInt();
                this->matcher->Match(query, flags);
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void Rewind(const std::string &method, const KgrValue &params,
                    Response &rsp) {
            if (this->matcher != nullptr) {
                TextPosition pos{static_cast<TextPosition::Line>(
                                     params.AsDictionary()["line"].AsInt()),
                                 static_cast<TextPosition::Column>(
                                     params.AsDictionary()["column"].AsInt())};
                this->matcher->Rewind(pos);
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void GetResults(const std::string &method, const KgrValue &params,
                        Response &rsp) {
            if (this->matcher != nullptr) {
                const auto &res = this->matcher->GetResults();
                KgrArray result;
                for (const auto &entry : res) {
                    result.Append(KgrDictionary{
                        {"start",
                         KgrDictionary{
                             {"line", static_cast<int64_t>(entry.start.line)},
                             {"column",
                              static_cast<int64_t>(entry.start.column)}}},
                        {"length", static_cast<int64_t>(entry.length)},
                        {"content", entry.content}});
                }
                rsp.Result(std::move(result));
            } else {
                rsp.Result({});
            }
        }

        void Replace(const std::string &method, const KgrValue &params,
                     Response &rsp) {
            if (this->matcher != nullptr) {
                const auto &res = this->matcher->GetResults();
                std::size_t idx = params.AsDictionary()["occurence"].AsInt();
                const std::string &by = params.AsDictionary()["by"].AsString();
                if (idx < res.size()) {
                    this->replacer->Replace(res.at(idx), by);
                    this->matcher->Rewind(res.at(idx).start);
                    rsp.Result(true);
                } else {
                    rsp.Result(false);
                }
            } else {
                rsp.Result(false);
            }
        }

        void ReplaceAll(const std::string &method, const KgrValue &params,
                        Response &rsp) {
            if (this->matcher != nullptr) {
                const auto &res = this->matcher->GetResults();
                const std::string &by = params.AsString();
                this->replacer->Replace(res.rbegin(), res.rend(), by);
                this->matcher->Reset();
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        SlokedEditorDocumentSet &documents;
        std::optional<SlokedEditorDocumentSet::Document> document;
        std::unique_ptr<SlokedTextMatcher> matcher;
        std::unique_ptr<SlokedTextReplacer> replacer;
    };

    SlokedSearchService::SlokedSearchService(
        SlokedEditorDocumentSet &documents,
        KgrContextManager<KgrLocalContext> &contextManager)
        : documents(documents), contextManager(contextManager) {}

    TaskResult<void> SlokedSearchService::Attach(
        std::unique_ptr<KgrPipe> pipe) {
        TaskResultSupplier<void> supplier;
        try {
            auto ctx = std::make_unique<SlokedSearchContext>(std::move(pipe),
                                                             this->documents);
            this->contextManager.Attach(std::move(ctx));
            supplier.SetResult();
        } catch (...) { supplier.SetError(std::current_exception()); }
        return supplier.Result();
    }
}  // namespace sloked