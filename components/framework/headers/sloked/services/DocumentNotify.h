#ifndef SLOKED_SERVICES_DOCUMENTNOTIFY_H_
#define SLOKED_SERVICES_DOCUMENTNOTIFY_H_

#include "sloked/kgr/Service.h"
#include "sloked/kgr/ContextManager.h"
#include "sloked/kgr/local/Context.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"

namespace sloked {

    class SlokedDocumentNotifyService : public KgrService {
     public:
        SlokedDocumentNotifyService(SlokedEditorDocumentSet &, KgrContextManager<KgrLocalContext> &);
        void Attach(std::unique_ptr<KgrPipe>) override;
    
     private:
        SlokedEditorDocumentSet &documents;
        KgrContextManager<KgrLocalContext> &contextManager;
    };

    class SlokedDocumentNotifyClient {
     public:
        using Notification = std::optional<TextPositionRange>;
        SlokedDocumentNotifyClient(std::unique_ptr<KgrPipe>, SlokedEditorDocumentSet::DocumentId, bool = false);
        void OnUpdate(std::function<void(const Notification &)>);

     private:
        std::unique_ptr<KgrPipe> pipe;
    };
}

#endif