#ifndef SLOKED_SERVICES_DOCUMENTSET_H_
#define SLOKED_SERVICES_DOCUMENTSET_H_

#include "sloked/kgr/Service.h"
#include "sloked/services/Service.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"

namespace sloked {

    class SlokedDocumentSetService : public KgrService {
     public:
        SlokedDocumentSetService(SlokedEditorDocumentSet &, const SlokedTextTaggerRegistry<SlokedEditorDocument::TagType> &, KgrContextManager<KgrLocalContext> &);
        void Attach(std::unique_ptr<KgrPipe>) override;

     private:
        SlokedEditorDocumentSet &documents;
        const SlokedTextTaggerRegistry<SlokedEditorDocument::TagType> &taggers;
        KgrContextManager<KgrLocalContext> &contextManager;
    };

    class SlokedDocumentSetClient {
     public:
        SlokedDocumentSetClient(std::unique_ptr<KgrPipe>);
        std::optional<SlokedEditorDocumentSet::DocumentId> New(const std::string &, const std::string &);
        std::optional<SlokedEditorDocumentSet::DocumentId> Open(const std::string &, const std::string &, const std::string &, const std::string & = "");
        bool Open(SlokedEditorDocumentSet::DocumentId);
        bool Save();
        bool Save(const std::string &);
        void Close();
        std::optional<SlokedEditorDocumentSet::DocumentId> GetId();
        std::optional<std::string> GetUpstream();

     private:
        SlokedServiceClient client;
    };
}

#endif