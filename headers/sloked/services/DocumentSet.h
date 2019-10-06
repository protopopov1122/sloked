#ifndef SLOKED_SERVICES_DOCUMENTSET_H_
#define SLOKED_SERVICES_DOCUMENTSET_H_

#include "sloked/kgr/Service.h"
#include "sloked/services/Service.h"
#include "sloked/editor/DocumentSet.h"

namespace sloked {

    class SlokedDocumentSetService : public KgrService {
     public:
        enum class Command {
            Open,
            Close,
            Get
        };

        SlokedDocumentSetService(SlokedEditorDocumentSet &, KgrContextManager<KgrLocalContext> &);
        bool Attach(std::unique_ptr<KgrPipe>) override;

     private:
        SlokedEditorDocumentSet &documents;
        KgrContextManager<KgrLocalContext> &contextManager;
    };
}

#endif