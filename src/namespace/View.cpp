#include "sloked/namespace/View.h"

namespace sloked {

    SlokedNamespaceView::SlokedNamespaceView(SlokedNamespace &base, const SlokedPath &path)
        : base(base), root(path) {
        if (!root.IsAbsolute()) {
            root = root.RelativeTo("/");
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

    void SlokedNamespaceView::MakeDir(const SlokedPath &path) {
        this->base.MakeDir(this->MakePath(path));
    }

    void SlokedNamespaceView::MakeFile(const SlokedPath &path) {
        this->base.MakeFile(this->MakePath(path));
    }

    void SlokedNamespaceView::Delete(const SlokedPath &path) {
        this->base.Delete(this->MakePath(path));
    }

    void SlokedNamespaceView::Rename(const SlokedPath &from, const SlokedPath &to) {
        this->base.Rename(this->MakePath(from), this->MakePath(to));
    }

    SlokedPath SlokedNamespaceView::MakePath(const SlokedPath &path) const {
        SlokedPath realPath = path.IsAbsolute() ? path.RelativeTo("/") : path;
        SlokedPath result = realPath.RelativeTo(this->root);
        if (this->root.IsChildOrSelf(result)) {
            return result;
        } else {
            return this->root;
        }
    }
}