#ifndef SLOKED_NAMESPACE_DIRECTORY_H_
#define SLOKED_NAMESPACE_DIRECTORY_H_

#include "sloked/namespace/Object.h"

namespace sloked {

    class SlokedNamespaceDefaultDirectory : public SlokedNamespaceDirectory {
     public:
        SlokedNamespaceDefaultDirectory(SlokedNamespace &, const SlokedPath &);

        Type GetType() const override;
        const SlokedPath &GetPath() const override;
        SlokedNamespaceDirectory *AsDirectory() override;

        std::unique_ptr<SlokedNamespaceObject> GetObject(const SlokedPath &) override;
        bool HasObject(const SlokedPath &) const override;
        void Iterate(const SlokedPath &, Visitor) const override;
        std::unique_ptr<SlokedNamespaceObjectHandle> GetHandle(const SlokedPath &) override;

     private:
        SlokedNamespace &ns;
        SlokedPath path;
    };
}

#endif