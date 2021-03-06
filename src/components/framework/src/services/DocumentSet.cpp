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

#include "sloked/services/DocumentSet.h"

#include "sloked/sched/CompoundTask.h"

namespace sloked {

    class SlokedDocumentSetContext : public SlokedServiceContext {
     public:
        SlokedDocumentSetContext(
            std::unique_ptr<KgrPipe> pipe, SlokedEditorDocumentSet &documents,
            const SlokedTextTaggerRegistry<SlokedEditorDocument::TagType>
                &taggers)
            : SlokedServiceContext(std::move(pipe)), documents(documents),
              document(documents.Empty()), taggers(taggers) {

            this->BindMethod("new", &SlokedDocumentSetContext::New);
            this->BindMethod("open", &SlokedDocumentSetContext::Open);
            this->BindMethod("openById", &SlokedDocumentSetContext::OpenById);
            this->BindMethod("save", &SlokedDocumentSetContext::Save);
            this->BindMethod("saveAs", &SlokedDocumentSetContext::SaveAs);
            this->BindMethod("close", &SlokedDocumentSetContext::Close);
            this->BindMethod("getId", &SlokedDocumentSetContext::GetId);
            this->BindMethod("getUpstream",
                             &SlokedDocumentSetContext::GetUpstream);
        }

     protected:
        void New(const std::string &method, const KgrValue &params,
                 Response &rsp) {
            const auto &prms = params.AsDictionary();
            const auto &encoding = prms["encoding"].AsString();
            const auto &newline = prms["newline"].AsString();
            const Encoding &enc = Encoding::Get(encoding);
            auto nl = NewLine::Create(newline, enc);
            this->document = this->documents.NewDocument(enc, std::move(nl));
            rsp.Result(static_cast<int64_t>(this->document.GetKey()));
        }

        void Open(const std::string &method, const KgrValue &params,
                  Response &rsp) {
            const auto &prms = params.AsDictionary();
            const auto &path = prms["path"].AsString();
            const auto &encoding = prms["encoding"].AsString();
            const auto &newline = prms["newline"].AsString();
            const Encoding &enc = Encoding::Get(encoding);
            auto nl = NewLine::Create(newline, enc);
            this->document = this->documents.OpenDocument(SlokedPath{path}, enc,
                                                          std::move(nl));
            if (prms.Has("tagger")) {
                this->document.GetObject().AttachTagger(this->taggers.TryCreate(
                    prms["tagger"].AsString(), this->document.GetObject()));
            }
            rsp.Result(static_cast<int64_t>(this->document.GetKey()));
        }

        void OpenById(const std::string &method, const KgrValue &params,
                      Response &rsp) {
            auto id = static_cast<SlokedEditorDocumentSet::DocumentId>(
                params.AsInt());
            if (this->documents.HasDocument(id)) {
                this->document = this->documents.OpenDocument(id).value();
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void Save(const std::string &method, const KgrValue &params,
                  Response &rsp) {
            if (this->document.Exists() &&
                this->document.GetObject().HasUpstream()) {
                this->document.GetObject().Save();
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void SaveAs(const std::string &method, const KgrValue &params,
                    Response &rsp) {
            if (this->document.Exists()) {
                SlokedPath to{params.AsString()};
                this->documents.SaveAs(this->document.GetObject(), to);
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void Close(const std::string &method, const KgrValue &params,
                   Response &rsp) {
            this->document.Release();
        }

        void GetId(const std::string &method, const KgrValue &params,
                   Response &rsp) {
            if (this->document.Exists()) {
                rsp.Result(static_cast<int64_t>(this->document.GetKey()));
            } else {
                rsp.Result({});
            }
        }

        void GetUpstream(const std::string &method, const KgrValue &params,
                         Response &rsp) {
            if (this->document.Exists()) {
                auto upstream = this->document.GetObject().GetUpstream();
                if (upstream.has_value()) {
                    rsp.Result(upstream.value().ToString());
                } else {
                    rsp.Result({});
                }
            } else {
                rsp.Result({});
            }
        }

     private:
        SlokedEditorDocumentSet &documents;
        SlokedEditorDocumentSet::Document document;
        const SlokedTextTaggerRegistry<SlokedEditorDocument::TagType> &taggers;
    };

    SlokedDocumentSetService::SlokedDocumentSetService(
        SlokedEditorDocumentSet &documents,
        const SlokedTextTaggerRegistry<SlokedEditorDocument::TagType> &taggers,
        KgrContextManager<KgrLocalContext> &contextManager)
        : documents(documents), taggers(taggers),
          contextManager(contextManager) {}

    TaskResult<void> SlokedDocumentSetService::Attach(
        std::unique_ptr<KgrPipe> pipe) {
        TaskResultSupplier<void> supplier;
        supplier.Wrap([&] {
            auto ctx = std::make_unique<SlokedDocumentSetContext>(
                std::move(pipe), this->documents, this->taggers);
            this->contextManager.Attach(std::move(ctx));
        });
        return supplier.Result();
    }

    SlokedDocumentSetClient::SlokedDocumentSetClient(
        std::unique_ptr<KgrPipe> pipe)
        : client(std::move(pipe)) {}

    TaskResult<std::optional<SlokedEditorDocumentSet::DocumentId>>
        SlokedDocumentSetClient::New(const std::string &encoding,
                                     const std::string &newline) {
        return SlokedTaskTransformations::Transform(
            this->client
                .Invoke("new", KgrDictionary{{"encoding", encoding},
                                             {"newline", newline}})
                ->Next(),
            [](const SlokedNetResponseBroker::Response &res) {
                std::optional<SlokedEditorDocumentSet::DocumentId> result;
                if (res.HasResult()) {
                    result = res.GetResult().AsInt();
                }
                return result;
            });
    }

    TaskResult<std::optional<SlokedEditorDocumentSet::DocumentId>>
        SlokedDocumentSetClient::Open(const std::string &path,
                                      const std::string &encoding,
                                      const std::string &newline,
                                      const std::string &tagger) {
        return SlokedTaskTransformations::Transform(
            this->client
                .Invoke("open", KgrDictionary{{"path", path},
                                              {"encoding", encoding},
                                              {"newline", newline},
                                              {"tagger", tagger}})
                ->Next(),
            [](const SlokedNetResponseBroker::Response &res) {
                std::optional<SlokedEditorDocumentSet::DocumentId> result;
                if (res.HasResult()) {
                    result = res.GetResult().AsInt();
                }
                return result;
            });
    }

    TaskResult<bool> SlokedDocumentSetClient::Open(
        SlokedEditorDocumentSet::DocumentId docId) {
        return SlokedTaskTransformations::Transform(
            this->client
                .Invoke("openById",
                        KgrDictionary{{"id", static_cast<int64_t>(docId)}})
                ->Next(),
            [](const SlokedNetResponseBroker::Response &res) {
                if (res.HasResult()) {
                    return res.GetResult().AsBoolean();
                } else {
                    return false;
                }
            });
    }

    TaskResult<bool> SlokedDocumentSetClient::Save() {
        return SlokedTaskTransformations::Transform(
            this->client.Invoke("save", {})->Next(),
            [](const SlokedNetResponseBroker::Response &res) {
                if (res.HasResult()) {
                    return res.GetResult().AsBoolean();
                } else {
                    return false;
                }
            });
    }

    TaskResult<bool> SlokedDocumentSetClient::Save(const std::string &path) {
        return SlokedTaskTransformations::Transform(
            this->client.Invoke("saveAs", path)->Next(),
            [](const SlokedNetResponseBroker::Response &res) {
                if (res.HasResult()) {
                    return res.GetResult().AsBoolean();
                } else {
                    return false;
                }
            });
    }

    void SlokedDocumentSetClient::Close() {
        this->client.Invoke("close", {});
    }

    TaskResult<std::optional<SlokedEditorDocumentSet::DocumentId>>
        SlokedDocumentSetClient::GetId() {
        return SlokedTaskTransformations::Transform(
            this->client.Invoke("getId", {})->Next(),
            [](const SlokedNetResponseBroker::Response &res) {
                std::optional<SlokedEditorDocumentSet::DocumentId> result;
                if (res.HasResult() &&
                    res.GetResult().Is(KgrValueType::Integer)) {
                    result = res.GetResult().AsInt();
                }
                return result;
            });
    }

    TaskResult<std::optional<std::string>>
        SlokedDocumentSetClient::GetUpstream() {
        return SlokedTaskTransformations::Transform(
            this->client.Invoke("getUpstream", {})->Next(),
            [](const SlokedNetResponseBroker::Response &res) {
                std::optional<std::string> result{};
                if (res.HasResult() &&
                    res.GetResult().Is(KgrValueType::String)) {
                    result = res.GetResult().AsString();
                }
                return result;
            });
    }
}  // namespace sloked