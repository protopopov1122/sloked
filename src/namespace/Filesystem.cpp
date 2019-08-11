#include "sloked/namespace/Filesystem.h"
#include "sloked/namespace/Directory.h"

namespace sloked {

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

   class SlokedFilesystemObjectHandle : public SlokedNamespaceObjectHandle {
    public:
      SlokedFilesystemObjectHandle(SlokedFilesystemAdapter &, const SlokedPath &);
      bool HasPermission(SlokedNamespacePermission) const override;
      bool Exists() const override;
      void MakeDir() override;
      void MakeFile() override;
      void Delete() override;
      void Rename(const SlokedPath &) override;

    private:
      SlokedFilesystemAdapter &filesystem;
      SlokedPath path;
   };

    SlokedFilesystemDocument::SlokedFilesystemDocument(std::unique_ptr<SlokedFile> file, const SlokedPath &path)
        : file(std::move(file)), path(path) {}
    
    SlokedNamespaceObject::Type SlokedFilesystemDocument::GetType() const {
        if (this->file && this->file->IsFile()) {
            return Type::File;
        } else  if (this->file && this->file->IsDirectory()) {
            return Type::Directory;
        } else {
            return Type::None;
        }
    }
    
    const SlokedPath &SlokedFilesystemDocument::GetPath() const {
        return this->path;
    }

    SlokedNamespaceFile *SlokedFilesystemDocument::AsFile() {
        if (this->file && this->file->IsFile()) {
            return this;
        } else {
            return nullptr;
        }
    }
    
    std::unique_ptr<SlokedIOReader> SlokedFilesystemDocument::Reader() const {
        if (this->file && this->file->IsFile()) {
            return this->file->Reader();
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedIOWriter> SlokedFilesystemDocument::Writer() {
        if (this->file && this->file->IsFile()) {
            return this->file->Writer();
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedIOView> SlokedFilesystemDocument::View() const {
        if (this->file && this->file->IsFile()) {
            return this->file->View();
        } else {
            return nullptr;
        }
    }

    SlokedFilesystemObjectHandle::SlokedFilesystemObjectHandle(SlokedFilesystemAdapter &filesystem, const SlokedPath &path)
        : filesystem(filesystem), path(path) {}

    bool SlokedFilesystemObjectHandle::HasPermission(SlokedNamespacePermission perm) const {
        auto file = this->filesystem.NewFile(this->path);
        if (file) switch (perm) {
            case SlokedNamespacePermission::Read:
                return file->HasPermission(SlokedFilesystemPermission::Read);

            case SlokedNamespacePermission::Write:
                return file->HasPermission(SlokedFilesystemPermission::Write);
        }
        return false;
    }

    bool SlokedFilesystemObjectHandle::Exists() const {
        auto file = this->filesystem.NewFile(this->path);
        return file && file->IsFile();
    }

    void SlokedFilesystemObjectHandle::MakeDir() {
        auto file = this->filesystem.NewFile(this->path);
        if (file && !file->Exists()) {
            file->Mkdir();
        }
    }

    void SlokedFilesystemObjectHandle::MakeFile() {
        auto file = this->filesystem.NewFile(this->path);
        if (file && !file->Exists()) {
            file->Create();
        }
    }

    void SlokedFilesystemObjectHandle::Delete() {
        auto file = this->filesystem.NewFile(this->path);
        if (file && file->Exists()) {
            file->Delete();
        }
    }

    void SlokedFilesystemObjectHandle::Rename(const SlokedPath &to) {
        auto file = this->filesystem.NewFile(this->path);
        auto fileTo = this->filesystem.NewFile(to);
        if (file && file->Exists() && fileTo) {
            file->Rename(fileTo->GetPath());
        }
    }

    SlokedFilesystemNamespace::SlokedFilesystemNamespace(std::unique_ptr<SlokedFilesystemAdapter> filesystem)
        : filesystem(std::move(filesystem)) {}
    
    std::unique_ptr<SlokedNamespaceObject> SlokedFilesystemNamespace::GetObject(const SlokedPath &path) {
        auto file = this->filesystem->NewFile(path);
        if (file && file->IsFile()) {
            return std::make_unique<SlokedFilesystemDocument>(std::move(file), path);
        } else if (file && file->IsDirectory()) {
            return std::make_unique<SlokedNamespaceDefaultDirectory>(*this, path);
        } else {
            return nullptr;
        }
    }

    bool SlokedFilesystemNamespace::HasObject(const SlokedPath &path) const {
        auto file = this->filesystem->NewFile(path);
        return file && (file->IsFile() || file->IsDirectory());
    }

    void SlokedFilesystemNamespace::Iterate(const SlokedPath &path, Visitor visitor) const {
        auto file = this->filesystem->NewFile(path);
        if (file && file->IsDirectory()) {
            file->ListFiles([&](const std::string &name) {
                auto child = file->GetFile(name);
                if (child && child->IsFile()) {
                    visitor(name, SlokedNamespaceObject::Type::File);
                } else if (child && child->IsDirectory()) {
                    visitor(name, SlokedNamespaceObject::Type::Directory);
                }
            });
        }
    }

    std::unique_ptr<SlokedNamespaceObjectHandle> SlokedFilesystemNamespace::GetHandle(const SlokedPath &path) {
        return std::make_unique<SlokedFilesystemObjectHandle>(*this->filesystem, path);
    }
}