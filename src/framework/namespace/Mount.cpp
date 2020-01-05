#include "sloked/namespace/Mount.h"
#include "sloked/namespace/View.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedDefaultNamespaceMounter::SlokedDefaultNamespaceMounter(std::unique_ptr<SlokedFilesystemAdapter> filesystem, SlokedNamespace &root)
        : filesystem(std::move(filesystem)), root(root) {}

    std::unique_ptr<SlokedNamespace> SlokedDefaultNamespaceMounter::Mount(const SlokedUri &uri) const {
        if (uri.GetScheme() == "file") {
            return std::make_unique<SlokedFilesystemNamespace>(this->filesystem->Rebase(uri.GetPath()));
        } else if (uri.GetScheme() == "root") {
            return std::make_unique<SlokedNamespaceView>(this->root, SlokedPath{uri.GetPath()});
        } else {
            throw SlokedError("MountDatabase: Unknown scheme \'" + uri.GetScheme() + "\'");
        }
    }
}