#include "sloked/namespace/posix/Filesystem.h"
#include "sloked/filesystem/posix/File.h"

namespace sloked {

    SlokedPosixFilesystemAdapter::SlokedPosixFilesystemAdapter(std::string_view root)
        : rootPath(root) {}

    const SlokedPath &SlokedPosixFilesystemAdapter::GetRoot() const {
        return this->rootPath;
    }

    std::unique_ptr<SlokedFile> SlokedPosixFilesystemAdapter::NewFile(const SlokedPath &path) const {
        SlokedPath realPath("/");
        if (path.IsAbsolute()) {
            realPath = this->GetRoot().RelativeTo(path.RelativeTo(SlokedPath::Root));
        } else {
            realPath = this->GetRoot().RelativeTo(path);
        }
        if (!this->GetRoot().IsChildOrSelf(realPath)) {
            return nullptr;
        } else {
            return std::make_unique<SlokedPosixFile>(realPath.ToString());
        }
    }
}