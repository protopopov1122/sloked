#include "sloked/namespace/View.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedNamespaceView::SlokedNamespaceView(SlokedNamespace &base, const SlokedPath &path)
        : base(base), root(path) {
        if (!root.IsAbsolute()) {
            root = root.RelativeTo(SlokedPath::Root);
        }
    }

    std::unique_ptr<SlokedNamespaceObject> SlokedNamespaceView::GetObject(const SlokedPath &path) const {
        return this->base.GetObject(this->MakePath(path));
    }

    bool SlokedNamespaceView::HasObject(const SlokedPath &path) const {
        return this->base.HasObject(this->MakePath(path));
    }

    void SlokedNamespaceView::Iterate(const SlokedPath &path, Visitor visitor) const {
        this->base.Iterate(this->MakePath(path), std::move(visitor));
    }

    std::unique_ptr<SlokedNamespaceObjectHandle> SlokedNamespaceView::GetHandle(const SlokedPath &path) {
        return this->base.GetHandle(this->MakePath(path));
    }

    SlokedPath SlokedNamespaceView::MakePath(const SlokedPath &path) const {
        SlokedPath realPath = path.IsAbsolute() ? path.RelativeTo(SlokedPath::Root) : path;
        SlokedPath result = realPath.RelativeTo(this->root);
        if (this->root.IsChildOrSelf(result)) {
            return result;
        } else {
            throw SlokedError(std::string{"Path out of namespace scope: "} + path.ToString());
        }
    }
}