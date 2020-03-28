#ifndef SLOKED_CORE_CLOSEABLE_H_
#define SLOKED_CORE_CLOSEABLE_H_

#include <functional>
#include <vector>

#include "sloked/Base.h"

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
}  // namespace sloked

#endif