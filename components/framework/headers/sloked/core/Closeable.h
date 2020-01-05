#ifndef SLOKED_CORE_CLOSEABLE_H_
#define SLOKED_CORE_CLOSEABLE_H_

#include "sloked/Base.h"
#include <vector>
#include <functional>

namespace sloked {

    class SlokedCloseable {
     public:
        virtual ~SlokedCloseable() = default;
        virtual void Close() = 0;
    };

    class SlokedCloseablePool : public SlokedCloseable {
     public:
        ~SlokedCloseablePool();
        void Attach(SlokedCloseable &);
        void Close() final;
     private:
        std::vector<std::reference_wrapper<SlokedCloseable>> closeables;
    };
}

#endif