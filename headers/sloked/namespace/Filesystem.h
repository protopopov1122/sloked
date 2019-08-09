#ifndef SLOKED_NAMESPACE_FILESYSTEM_H_
#define SLOKED_NAMESPACE_FILESYSTEM_H_

#include "sloked/namespace/Object.h"
#include "sloked/filesystem/File.h"

namespace sloked {

    class SlokedFilesystemAdapter {
     public:
        virtual ~SlokedFilesystemAdapter() = default;
        virtual const SlokedPath &GetRoot() const = 0;
        virtual std::unique_ptr<SlokedFile> NewFile(const SlokedPath &) const = 0;
    };

    class SlokedFilesystemDocument : public SlokedNamespaceFile {
     public:
        SlokedFilesystemDocument(std::unique_ptr<SlokedFile>, const SlokedPath &);
        Type GetType() const override;
        const SlokedPath &GetPath() const override;
        SlokedNamespaceFile *AsFile() override;
        std::unique_ptr<SlokedIOReader> Reader() const override;
        std::unique_ptr<SlokedIOWriter> Writer() override;
        std::unique_ptr<SlokedIOView> View() const override;

     private:
        std::unique_ptr<SlokedFile> file;
        SlokedPath path;
    };

    class SlokedFilesystemNamespace : public SlokedNamespace {
     public:
        SlokedFilesystemNamespace(std::unique_ptr<SlokedFilesystemAdapter>);
        std::unique_ptr<SlokedNamespaceObject> GetObject(const SlokedPath &) const override;
        bool HasObject(const SlokedPath &) const override;
        void Iterate(const SlokedPath &, Visitor) const override;

        void MakeFile(const SlokedPath &) override;
        void MakeDir(const SlokedPath &) override;
        void Delete(const SlokedPath &) override;
        void Rename(const SlokedPath &, const SlokedPath &) override;

     private:
        std::unique_ptr<SlokedFilesystemAdapter> filesystem;
    };
}

#endif