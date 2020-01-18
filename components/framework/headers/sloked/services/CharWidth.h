#ifndef SLOKED_SERVICES_CHARWIDTH_H_
#define SLOKED_SERVICES_CHARWIDTH_H_

#include "sloked/core/CharWidth.h"
#include "sloked/core/Closeable.h"
#include "sloked/kgr/Service.h"
#include "sloked/kgr/ContextManager.h"
#include "sloked/kgr/local/Context.h"

namespace sloked {

    class SlokedCharWidthService : public KgrService {
     public:
        SlokedCharWidthService(const SlokedCharWidth &, KgrContextManager<KgrLocalContext> &);
        void Attach(std::unique_ptr<KgrPipe>) final;

     private:
        const SlokedCharWidth &charWidth;
        KgrContextManager<KgrLocalContext> &contextManager;
    };

    class SlokedCharWidthClient : public SlokedCloseable {
     public:
        SlokedCharWidthClient(std::unique_ptr<KgrPipe>, SlokedCharWidth &);
        void Close() final;

     private:
        std::unique_ptr<KgrPipe> pipe;
        SlokedCharWidth &charWidth;
    };
}

#endif