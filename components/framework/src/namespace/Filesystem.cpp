/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/namespace/Filesystem.h"

#include "sloked/namespace/Directory.h"

namespace sloked {

    class SlokedFilesystemDocument : public SlokedNamespaceFile {
     public:
        SlokedFilesystemDocument(std::unique_ptr<SlokedFile>,
                                 const SlokedPath &);
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
        SlokedFilesystemObjectHandle(SlokedFilesystemAdapter &,
                                     const SlokedPath &);
        bool HasPermission(SlokedNamespacePermission) const override;
        std::optional<std::string> ToURI() const override;
        bool Exists() const override;
        void MakeDir() override;
        void MakeFile() override;
        void Delete() override;
        void Rename(const SlokedPath &) override;

     private:
        SlokedFilesystemAdapter &filesystem;
        SlokedPath path;
    };

    SlokedFilesystemDocument::SlokedFilesystemDocument(
        std::unique_ptr<SlokedFile> file, const SlokedPath &path)
        : file(std::move(file)), path(path) {}

    SlokedNamespaceObject::Type SlokedFilesystemDocument::GetType() const {
        if (this->file && this->file->IsFile()) {
            return Type::File;
        } else if (this->file && this->file->IsDirectory()) {
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

    SlokedFilesystemObjectHandle::SlokedFilesystemObjectHandle(
        SlokedFilesystemAdapter &filesystem, const SlokedPath &path)
        : filesystem(filesystem), path(path) {}

    bool SlokedFilesystemObjectHandle::HasPermission(
        SlokedNamespacePermission perm) const {
        auto file = this->filesystem.NewFile(this->path);
        if (file)
            switch (perm) {
                case SlokedNamespacePermission::Read:
                    return file->HasPermission(
                        SlokedFilesystemPermission::Read);

                case SlokedNamespacePermission::Write:
                    return file->HasPermission(
                        SlokedFilesystemPermission::Write);
            }
        return false;
    }

    std::optional<std::string> SlokedFilesystemObjectHandle::ToURI() const {
        return this->filesystem.ToURI(this->path);
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

    SlokedFilesystemNamespace::SlokedFilesystemNamespace(
        std::unique_ptr<SlokedFilesystemAdapter> filesystem)
        : filesystem(std::move(filesystem)) {}

    std::unique_ptr<SlokedNamespaceObject> SlokedFilesystemNamespace::GetObject(
        const SlokedPath &path) {
        auto file = this->filesystem->NewFile(path);
        if (file && file->IsFile()) {
            return std::make_unique<SlokedFilesystemDocument>(std::move(file),
                                                              path);
        } else if (file && file->IsDirectory()) {
            return std::make_unique<SlokedNamespaceDefaultDirectory>(*this,
                                                                     path);
        } else {
            return nullptr;
        }
    }

    bool SlokedFilesystemNamespace::HasObject(const SlokedPath &path) const {
        auto file = this->filesystem->NewFile(path);
        return file && (file->IsFile() || file->IsDirectory());
    }

    void SlokedFilesystemNamespace::Iterate(const SlokedPath &path,
                                            Visitor visitor) const {
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

    void SlokedFilesystemNamespace::Traverse(const SlokedPath &path,
                                             Visitor visitor,
                                             bool include_dirs) const {
        auto file = this->filesystem->NewFile(path);
        if (file) {
            file->Traverse(
                [&](const std::string &name) {
                    auto childPath = this->filesystem->ToPath(name);
                    auto file = this->filesystem->NewFile(childPath);
                    visitor(childPath.RelativeTo(path).ToString(),
                            file->IsFile()
                                ? SlokedNamespaceObject::Type::File
                                : SlokedNamespaceObject::Type::Directory);
                },
                include_dirs);
        }
    }

    std::unique_ptr<SlokedNamespaceObjectHandle>
        SlokedFilesystemNamespace::GetHandle(const SlokedPath &path) {
        return std::make_unique<SlokedFilesystemObjectHandle>(*this->filesystem,
                                                              path);
    }
}  // namespace sloked