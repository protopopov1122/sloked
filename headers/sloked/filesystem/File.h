#ifndef SLOKED_FILESYSTEM_FILE_H_
#define SLOKED_FILESYSTEM_FILE_H_

#include "sloked/filesystem/FileIO.h"
#include <memory>
#include <functional>

namespace sloked {

    class SlokedFile;
    class SlokedDirectory;

    class SlokedFSObject {
     public:
        enum class Type  {
            RegularFile,
            Directory,
            Unsupported
        };

        virtual ~SlokedFSObject() = default;
        virtual Type GetType() const = 0;
        virtual const std::string &GetPath() const = 0;
        virtual bool Exists() const = 0;

        virtual SlokedFile *AsFile();
        virtual SlokedDirectory *AsDirectory();
    };

    class SlokedFile : public virtual SlokedFSObject {
     public:
        using Size = uint64_t;

        virtual ~SlokedFile() = default;

        virtual Size GetSize() const = 0;
        virtual std::unique_ptr<SlokedFileReader> Reader() const = 0;
        virtual std::unique_ptr<SlokedFileWriter> Writer() const = 0;
        virtual std::unique_ptr<SlokedFileView> View() const = 0;
    };

    class SlokedDirectory : public virtual SlokedFSObject {
     public:
        using FileVisitor = std::function<void(std::string_view)>;
        virtual ~SlokedDirectory() = default;
        virtual std::unique_ptr<SlokedFSObject> GetFile(std::string_view) const = 0;
        virtual void ListFiles(FileVisitor) const = 0;
    };
}

#endif