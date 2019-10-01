#ifndef SLOKED_SERVICES_SERVICE_H_
#define SLOKED_SERVICES_SERVICE_H_

#include "sloked/kgr/local/Context.h"
#include "sloked/core/Error.h"
#include <queue>
#include <functional>

namespace sloked {

    class SlokedServiceContext : public KgrLocalContext {
     public:
        using KgrLocalContext::KgrLocalContext;
        void Run() override;

     protected:
        void SendResponse(KgrValue &&);
        void OnRequest(std::function<void(const KgrValue &)>);

        virtual void ProcessRequest(const KgrValue &) = 0;
        virtual void HandleError(const SlokedError &);

     private:
        std::queue<std::function<void(const KgrValue &)>> requestHandlers;
    };
}

#endif