#ifndef SLOKED_NAMESPACE_VIEW_H_
#define SLOKED_NAMESPACE_VIEW_H_

#include "sloked/namespace/Object.h"

namespace sloked {

    class SlokedNamespaceView : public SlokedNamespace {
     public:
        SlokedNamespaceView(SlokedNamespace &, const SlokedPath &);

        std::unique_ptr<SlokedNamespaceObject> GetObject(const SlokedPath &) override;
        bool HasObject(const SlokedPath &) const override;
        void Iterate(const SlokedPath &, Visitor) const override;
        std::unique_ptr<SlokedNamespaceObjectHandle> GetHandle(const SlokedPath &) override;

     private:
        SlokedPath MakePath(const SlokedPath &) const;

        SlokedNamespace &base;
        SlokedPath root;
    };
}

#endif