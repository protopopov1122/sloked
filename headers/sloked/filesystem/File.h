#ifndef SLOKED_FILESYSTEM_FILE_H_
#define SLOKED_FILESYSTEM_FILE_H_

#include "sloked/core/IO.h"
#include <memory>
#include <functional>

namespace sloked {

    class SlokedFile {
     public:
        using Size = uint64_t;
        using FileVisitor = std::function<void(const std::string &)>;
        enum class Type  {
            RegularFile,
            Directory,
            Unsupported
        };

        virtual ~SlokedFile() = default;

        virtual bool IsFile() const = 0;
        virtual bool IsDirectory() const = 0;
        virtual const std::string &GetPath() const = 0;
        virtual std::string GetName() const = 0;
        virtual std::string GetParent() const = 0;
        virtual bool Exists() const = 0;
        virtual void Delete() const = 0;
        virtual void Rename(const std::string &) const = 0;
        virtual void Mkdir() const = 0;

        virtual Size GetSize() const = 0;
        virtual std::unique_ptr<SlokedIOReader> Reader() const = 0;
        virtual std::unique_ptr<SlokedIOWriter> Writer() const = 0;
        virtual std::unique_ptr<SlokedIOView> View() const = 0;

        virtual std::unique_ptr<SlokedFile> GetFile(std::string_view) const = 0;
        virtual void ListFiles(FileVisitor) const = 0;
    };
}

#endif