#ifndef SLOKED_NAMESPACE_POSIX_FILESYSTEM_H_
#define SLOKED_NAMESPACE_POSIX_FILESYSTEM_H_

#include "sloked/namespace/Filesystem.h"

namespace sloked {

    class SlokedPosixFilesystemAdapter : public SlokedFilesystemAdapter {
     public:
        SlokedPosixFilesystemAdapter(std::string_view);

        const SlokedPath &GetRoot() const override;
        std::unique_ptr<SlokedFile> NewFile(const SlokedPath &) const override;

     private:
        SlokedPath rootPath;
    };
}

#endif