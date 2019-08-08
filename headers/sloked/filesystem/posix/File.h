#ifndef SLOKED_FILESYSTEN_POSIX_FILE_H_
#define SLOKED_FILESYSTEN_POSIX_FILE_H_

#include "sloked/filesystem/File.h"

namespace sloked {

    class SlokedPosixFile : public SlokedFile {
     public:
        SlokedPosixFile(const std::string &);

        bool IsFile() const override;
        bool IsDirectory() const override;
        const std::string &GetPath() const override;
        std::string GetName() const override;
        std::string GetParent() const override;
        bool Exists() const override;
        void Delete() const override;
        void Rename(const std::string &) const override;
        void Mkdir() const override;

        uint64_t GetSize() const override;
        std::unique_ptr<SlokedIOReader> Reader() const override;
        std::unique_ptr<SlokedIOWriter> Writer() const override;
        std::unique_ptr<SlokedIOView> View() const override;

        std::unique_ptr<SlokedFile> GetFile(std::string_view) const override;
        void ListFiles(FileVisitor) const override;

     private:
        std::string path;
    };
}

#endif