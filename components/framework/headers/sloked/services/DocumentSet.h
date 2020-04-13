#ifndef SLOKED_SERVICES_DOCUMENTSET_H_
#define SLOKED_SERVICES_DOCUMENTSET_H_

#include "sloked/editor/doc-mgr/DocumentSet.h"
#include "sloked/kgr/Service.h"
#include "sloked/services/Service.h"

namespace sloked {

    class SlokedDocumentSetService : public KgrService {
     public:
        SlokedDocumentSetService(
            SlokedEditorDocumentSet &,
            const SlokedTextTaggerRegistry<SlokedEditorDocument::TagType> &,
            KgrContextManager<KgrLocalContext> &);
        TaskResult<void> Attach(std::unique_ptr<KgrPipe>) override;

     private:
        SlokedEditorDocumentSet &documents;
        const SlokedTextTaggerRegistry<SlokedEditorDocument::TagType> &taggers;
        KgrContextManager<KgrLocalContext> &contextManager;
    };

    class SlokedDocumentSetClient {
     public:
        SlokedDocumentSetClient(std::unique_ptr<KgrPipe>);
        TaskResult<std::optional<SlokedEditorDocumentSet::DocumentId>> New(
            const std::string &, const std::string &);
        TaskResult<std::optional<SlokedEditorDocumentSet::DocumentId>> Open(
            const std::string &, const std::string &, const std::string &,
            const std::string & = "");
        TaskResult<bool> Open(SlokedEditorDocumentSet::DocumentId);
        TaskResult<bool> Save();
        TaskResult<bool> Save(const std::string &);
        void Close();
        TaskResult<std::optional<SlokedEditorDocumentSet::DocumentId>> GetId();
        TaskResult<std::optional<std::string>> GetUpstream();

     private:
        SlokedServiceClient client;
    };
}  // namespace sloked

#endif