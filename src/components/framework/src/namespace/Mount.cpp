#include "sloked/namespace/Mount.h"

#include "sloked/core/Error.h"
#include "sloked/namespace/View.h"

namespace sloked {

    SlokedDefaultNamespaceMounter::SlokedDefaultNamespaceMounter(
        std::unique_ptr<SlokedFilesystemAdapter> filesystem,
        SlokedNamespace &root)
        : filesystem(std::move(filesystem)), root(root) {}

    SlokedFilesystemAdapter &SlokedDefaultNamespaceMounter::GetFilesystemAdapter() {
        return *this->filesystem;
    }

    std::unique_ptr<SlokedNamespace> SlokedDefaultNamespaceMounter::Mount(
        const SlokedUri &uri) const {
        if (uri.GetScheme() == "file") {
            return std::make_unique<SlokedFilesystemNamespace>(
                this->filesystem->Rebase(this->filesystem->FromPath({uri.GetPath()})));
        } else if (uri.GetScheme() == "root") {
            return std::make_unique<SlokedNamespaceView>(
                this->root, SlokedPath{uri.GetPath()});
        } else {
            throw SlokedError("MountDatabase: Unknown scheme \'" +
                              uri.GetScheme() + "\'");
        }
    }
}  // namespace sloked