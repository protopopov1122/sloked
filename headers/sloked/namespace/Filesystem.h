#ifndef SLOKED_NAMESPACE_FILESYSTEM_H_
#define SLOKED_NAMESPACE_FILESYSTEM_H_

#include "sloked/namespace/Object.h"
#include "sloked/filesystem/File.h"

namespace sloked {

    class SlokedFilesystemAdapter {
     public:
        virtual ~SlokedFilesystemAdapter() = default;
        virtual SlokedPath GetRoot() = 0;
        virtual SlokedPath ToPath(const std::string &) = 0;
        virtual std::unique_ptr<SlokedFile> NewFile(const SlokedPath &) = 0;
    };

    class SlokedFilesystemObject : public SlokedNSDocument, public SlokedNamespace {
     public:
        SlokedFilesystemObject(const SlokedPath &, SlokedFilesystemAdapter &);
        
        Type GetType() const override;
        SlokedPath GetPath() const override;

        std::unique_ptr<SlokedIOReader> Reader() const override;
        std::unique_ptr<SlokedIOWriter> Writer() const override;
        std::unique_ptr<SlokedIOView> View() const override;

        std::unique_ptr<SlokedNamespaceObject> GetObject(const std::string &) override;
        bool HasObject(const std::string &) override;
        void Iterate(Visitor) override;
        std::unique_ptr<SlokedNamespaceObject> Find(const SlokedPath &) override;
        std::unique_ptr<SlokedNamespace> MakeNamespace(const std::string &) override;
        void Delete(const std::string &) override;
        void Rename(const std::string &, const SlokedPath &) override;

     private:
        std::unique_ptr<SlokedFile> file;
        SlokedFilesystemAdapter &adapter;
    };
}

#endif