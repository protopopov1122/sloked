#include "sloked/namespace/Filesystem.h"

namespace sloked {

    SlokedFilesystemObject::SlokedFilesystemObject(const SlokedPath &path, SlokedFilesystemAdapter &adapter)
        : adapter(adapter) {
        this->file = adapter.NewFile(path);
    }
    
    SlokedFilesystemObject::Type SlokedFilesystemObject::GetType() const {
        if (this->file && this->file->IsFile()) {
            return Type::Document;
        } else if (this->file) {
            return Type::Namespace;
        } else {
            return Type::Unknown;
        }
    }

    SlokedPath SlokedFilesystemObject::GetPath() const {
        if (this->file) {
            return this->adapter.ToPath(this->file->GetPath()).RelativeTo(this->adapter.GetRoot()).RelativeTo(SlokedPath("/"));
        } else {
            return SlokedPath("");
        }
    }

    std::unique_ptr<SlokedIOReader> SlokedFilesystemObject::Reader() const {
        if (this->file && this->file->IsFile()) {
            return this->file->Reader();
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedIOWriter> SlokedFilesystemObject::Writer() const {
        if (this->file && this->file->IsFile()) {
            return this->file->Writer();
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedIOView> SlokedFilesystemObject::View() const {
        if (this->file && this->file->IsFile()) {
            return this->file->View();
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedNamespaceObject> SlokedFilesystemObject::GetObject(const std::string &name) {
        return std::make_unique<SlokedFilesystemObject>(this->GetPath().GetChild(name), this->adapter);
    }

    bool SlokedFilesystemObject::HasObject(const std::string &name) {
        if (!this->file) {
            return false;
        }
        auto file = this->file->GetFile(name);
        return file != nullptr && file->Exists();
    }

    void SlokedFilesystemObject::Iterate(Visitor v) {
        if (this->file) {
            this->file->ListFiles(v);
        }
    }

    std::unique_ptr<SlokedNamespaceObject> SlokedFilesystemObject::Find(const SlokedPath &path) {
        if (path.IsAbsolute()) {
            return std::make_unique<SlokedFilesystemObject>(path, this->adapter);
        } else {
            return std::make_unique<SlokedFilesystemObject>(this->GetPath().RelativeTo(path), this->adapter);
        }
    }

    std::unique_ptr<SlokedNamespace> SlokedFilesystemObject::MakeNamespace(const std::string &name) {
        if (this->file && this->file->IsDirectory()) {
            auto file = this->file->GetFile(name);
            if (!file->Exists()) {
                file->Mkdir();
                return std::make_unique<SlokedFilesystemObject>(this->GetPath().GetChild(name), this->adapter);
            }
        }
        return nullptr;
    }

    void SlokedFilesystemObject::Delete(const std::string &name) {
        if (this->file && this->file->IsDirectory()) {
            auto file = this->file->GetFile(name);
            if (file->Exists()) {
                file->Delete();
            }
        }
    }

    void SlokedFilesystemObject::Rename(const std::string &name, const SlokedPath &to) {
        SlokedPath realTo(to);
        if (!realTo.IsAbsolute()) {
            realTo = realTo.RelativeTo(this->GetPath());
        }
        auto newFile = this->adapter.NewFile(realTo);
        if (newFile) {
            this->file->GetFile(name)->Rename(newFile->GetPath());
        }
    }
}