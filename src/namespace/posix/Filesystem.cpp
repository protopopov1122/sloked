#include "sloked/namespace/posix/Filesystem.h"
#include "sloked/filesystem/posix/File.h"

namespace sloked {

    SlokedPosixFilesystemAdapter::SlokedPosixFilesystemAdapter(std::string_view root)
        : root(root) {}

    SlokedPath SlokedPosixFilesystemAdapter::GetRoot() {
        return SlokedPath(this->root);
    }

    SlokedPath SlokedPosixFilesystemAdapter::ToPath(const std::string &name) {
        return SlokedPath(name);
    }

    std::unique_ptr<SlokedFile> SlokedPosixFilesystemAdapter::NewFile(const SlokedPath &path) {
        SlokedPath realPath("/");
        if (path.IsAbsolute()) {
            realPath = this->GetRoot().RelativeTo(path.RelativeTo(SlokedPath("/")));
        } else {
            realPath = this->GetRoot().RelativeTo(path);
        }
        if (!this->GetRoot().IsChildOrSibling(realPath)) {
            return nullptr;
        } else {
            return std::make_unique<SlokedPosixFile>(realPath.ToString());
        }
    }
}