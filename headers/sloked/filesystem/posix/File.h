#ifndef SLOKED_FILESYSTEN_POSIX_FILE_H_
#define SLOKED_FILESYSTEN_POSIX_FILE_H_

#include "sloked/filesystem/File.h"

namespace sloked {

    class SlokedPosixFile : public SlokedFile, public SlokedDirectory {
     public:
        SlokedPosixFile(const std::string &);

        Type GetType() const override;
        const std::string &GetPath() const override;
        bool Exists() const override;

        uint64_t GetSize() const override;
        std::unique_ptr<SlokedFileReader> Reader() const override;
        std::unique_ptr<SlokedFileWriter> Writer() const override;
        std::unique_ptr<SlokedFileView> View() const override;

        std::unique_ptr<SlokedFSObject> GetFile(std::string_view) const override;
        void ListFiles(FileVisitor) const override;

     private:
        std::string path;
    };
}

#endif