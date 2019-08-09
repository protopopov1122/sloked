#include "sloked/namespace/Filesystem.h"

namespace sloked {

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

    SlokedFilesystemNamespace::SlokedFilesystemNamespace(std::unique_ptr<SlokedFilesystemAdapter> filesystem)
        : filesystem(std::move(filesystem)) {}
    
    std::unique_ptr<SlokedNamespaceObject> SlokedFilesystemNamespace::GetObject(const SlokedPath &path) const {
        auto file = this->filesystem->NewFile(path);
        if (file && file->IsFile()) {
            return std::make_unique<SlokedFilesystemDocument>(std::move(file), path);
        } else {
            return nullptr;
        }
    }

    bool SlokedFilesystemNamespace::HasObject(const SlokedPath &path) const {
        auto file = this->filesystem->NewFile(path);
        return file && file->IsFile();
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
                } else if (child) {
                    visitor(name, SlokedNamespaceObject::Type::None);
                }
            });
        }
    }

    void SlokedFilesystemNamespace::MakeFile(const SlokedPath &path) {
        auto file = this->filesystem->NewFile(path);
        if (file && !file->Exists()) {
            file->Create();
        }
    }
    
    void SlokedFilesystemNamespace::MakeDir(const SlokedPath &path) {
        auto file = this->filesystem->NewFile(path);
        if (file && !file->Exists()) {
            file->Mkdir();
        }
    }

    void SlokedFilesystemNamespace::Delete(const SlokedPath &path) {
        auto file = this->filesystem->NewFile(path);
        if (file && file->Exists()) {
            file->Delete();
        }
    }

    void SlokedFilesystemNamespace::Rename(const SlokedPath &from, const SlokedPath &to) {
        auto file = this->filesystem->NewFile(from);
        auto fileTo = this->filesystem->NewFile(to);
        if (file && file->Exists() && fileTo) {
            file->Rename(fileTo->GetPath());
        }
    }
}