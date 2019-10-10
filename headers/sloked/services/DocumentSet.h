#ifndef SLOKED_SERVICES_DOCUMENTSET_H_
#define SLOKED_SERVICES_DOCUMENTSET_H_

#include "sloked/kgr/Service.h"
#include "sloked/services/Service.h"
#include "sloked/editor/DocumentSet.h"

namespace sloked {

    class SlokedDocumentSetService : public KgrService {
     public:
        enum class Command {
            New,
            Open,
            Save,
            Close,
            Get
        };

        SlokedDocumentSetService(SlokedEditorDocumentSet &, KgrContextManager<KgrLocalContext> &);
        bool Attach(std::unique_ptr<KgrPipe>) override;

     private:
        SlokedEditorDocumentSet &documents;
        KgrContextManager<KgrLocalContext> &contextManager;
    };

    class SlokedDocumentSetClient {
     public:
        SlokedDocumentSetClient(std::unique_ptr<KgrPipe>);
        std::optional<SlokedEditorDocumentSet::DocumentId> New(const std::string &, const std::string &);
        std::optional<SlokedEditorDocumentSet::DocumentId> Open(const std::string &, const std::string &, const std::string &);
        bool Save();
        bool Save(const std::string &);
        void Close();
        std::optional<SlokedEditorDocumentSet::DocumentId> Get() const;

     private:
        std::unique_ptr<KgrPipe> pipe;
    };
}

#endif