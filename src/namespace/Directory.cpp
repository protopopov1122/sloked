#include "sloked/namespace/Directory.h"

namespace sloked {

    SlokedNamespaceDefaultDirectory::SlokedNamespaceDefaultDirectory(SlokedNamespace &ns, const SlokedPath &path)
        : ns(ns), path(path) {}

    SlokedNamespaceObject::Type SlokedNamespaceDefaultDirectory::GetType() const {
        return Type::Directory;
    }

    const SlokedPath &SlokedNamespaceDefaultDirectory::GetPath() const {
        return this->path;
    }

    SlokedNamespaceDirectory *SlokedNamespaceDefaultDirectory::AsDirectory() {
        return this;
    }

    std::unique_ptr<SlokedNamespaceObject> SlokedNamespaceDefaultDirectory::GetObject(const SlokedPath &path) {
        if (path.IsAbsolute()) {
            return this->ns.GetObject(path);
        } else {
            return this->ns.GetObject(path.RelativeTo(this->path));
        }
    }

    bool SlokedNamespaceDefaultDirectory::HasObject(const SlokedPath &path) const {
        if (path.IsAbsolute()) {
            return this->ns.HasObject(path);
        } else {
            return this->ns.HasObject(path.RelativeTo(this->path));
        }
    }
    
    void SlokedNamespaceDefaultDirectory::Iterate(const SlokedPath &path, Visitor visitor) const {
        if (path.IsAbsolute()) {
            this->ns.Iterate(path, std::move(visitor));
        } else {
            this->ns.Iterate(path.RelativeTo(this->path), std::move(visitor));
        }
    }
    
    std::unique_ptr<SlokedNamespaceObjectHandle> SlokedNamespaceDefaultDirectory::GetHandle(const SlokedPath &path) {
        if (path.IsAbsolute()) {
            return this->ns.GetHandle(path);
        } else {
            return this->ns.GetHandle(path.RelativeTo(this->path));
        }
    }
}