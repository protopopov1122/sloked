#ifndef SLOKED_NAMESPACE_VIRTUAL_H_
#define SLOKED_NAMESPACE_VIRTUAL_H_

#include "sloked/namespace/Object.h"
#include <map>

namespace sloked {

    class SlokedVirtualNamespace : public SlokedNamespace {
     public:
        SlokedVirtualNamespace(std::unique_ptr<SlokedNamespace>);

        void Mount(const SlokedPath &, std::unique_ptr<SlokedNamespace>);
        void Umount(const SlokedPath &);

        std::unique_ptr<SlokedNamespaceObject> GetObject(const SlokedPath &) override;
        bool HasObject(const SlokedPath &) const override;
        void Iterate(const SlokedPath &, Visitor) const override;
        std::unique_ptr<SlokedNamespaceObjectHandle> GetHandle(const SlokedPath &) override;

        friend class SlokedVirtualObjectHandle;

     private:
        struct Entry {
            std::unique_ptr<SlokedNamespace> ns;
            SlokedPath path {"/"};
            std::map<std::string, Entry> subentries;
        };

        const Entry &find(const SlokedPath &) const;
        void cleanup(Entry &);
        Entry root;
    };
}

#endif