#include "sloked/namespace/Virtual.h"
#include "sloked/core/Error.h"

namespace sloked {

    class SlokedVirtualObjectHandle : public SlokedNamespaceObjectHandle {
     public:
        using Entry = SlokedVirtualNamespace::Entry;
        SlokedVirtualObjectHandle(SlokedVirtualNamespace &, const SlokedPath &);
        void MakeDir() override;
        void MakeFile() override;
        void Delete() override;
        void Rename(const SlokedPath &) override;

     private:
        SlokedVirtualNamespace &ns;
        SlokedPath path;
    };

    SlokedVirtualObjectHandle::SlokedVirtualObjectHandle(SlokedVirtualNamespace &ns, const SlokedPath &path)
        : ns(ns), path(path) {}

    void SlokedVirtualObjectHandle::MakeDir() {
        SlokedPath realPath = this->path.IsAbsolute() ? this->path : this->path.RelativeTo(SlokedPath::Root);
        const Entry &entry = this->ns.find(realPath);
        SlokedPath nsPath = realPath.RelativeTo(entry.path).RelativeTo(SlokedPath::Root);
        entry.ns->GetHandle(nsPath)->MakeDir();
    }

    void SlokedVirtualObjectHandle::MakeFile() {
        SlokedPath realPath = this->path.IsAbsolute() ? this->path : this->path.RelativeTo(SlokedPath::Root);
        const Entry &entry = this->ns.find(realPath);
        SlokedPath nsPath = realPath.RelativeTo(entry.path).RelativeTo(SlokedPath::Root);
        entry.ns->GetHandle(nsPath)->MakeFile();
    }

    void SlokedVirtualObjectHandle::Delete() {
        SlokedPath realPath = this->path.IsAbsolute() ? this->path : this->path.RelativeTo(SlokedPath::Root);
        const Entry &entry = this->ns.find(realPath);
        SlokedPath nsPath = realPath.RelativeTo(entry.path).RelativeTo(SlokedPath::Root);
        entry.ns->GetHandle(nsPath)->Delete();
    }

    void SlokedVirtualObjectHandle::Rename(const SlokedPath &to) {
        SlokedPath realFrom = this->path.IsAbsolute() ? this->path : this->path.RelativeTo(SlokedPath::Root);
        SlokedPath realTo = to.IsAbsolute() ? to : to.RelativeTo(SlokedPath::Root);
        const Entry &entry = this->ns.find(realFrom);
        if (entry.path.IsChildOrSelf(realTo)) {
            SlokedPath nsFromPath = realFrom.RelativeTo(entry.path).RelativeTo(SlokedPath::Root);
            SlokedPath nsToPath = realTo.RelativeTo(entry.path).RelativeTo(SlokedPath::Root);
            entry.ns->GetHandle(nsFromPath)->Rename(nsToPath);
        }
    }

    SlokedVirtualNamespace::SlokedVirtualNamespace(std::unique_ptr<SlokedNamespace> ns)
        : root{std::move(ns), SlokedPath::Root, {}} {}

    void SlokedVirtualNamespace::Mount(const SlokedPath &path, std::unique_ptr<SlokedNamespace> ns) {
        SlokedPath realPath = path.IsAbsolute() ? path : path.RelativeTo(SlokedPath::Root);
        Entry *entry = &this->root;
        SlokedPath entryPath(SlokedPath::Root);
        if (realPath == entryPath) {
            throw SlokedError("Permission denied: Mounting /");
        }
        realPath.Iterate([&](const auto &name) {
            entryPath = entryPath.GetChild(name);
            if (entry->subentries.count(name)) {
                entry = &entry->subentries[name];
            } else {
                entry->subentries.emplace(name, Entry{nullptr, entryPath, {}});
                entry = &entry->subentries[name];
            }
        });
        entry->ns = std::move(ns);
    }

    void SlokedVirtualNamespace::Umount(const SlokedPath &path) {
        SlokedPath realPath = path.IsAbsolute() ? path : path.RelativeTo(SlokedPath::Root);
        Entry *entry = &this->root;
        SlokedPath entryPath(SlokedPath::Root);
        if (realPath == entryPath) {
            return;
        }
        realPath.Iterate([&](const auto &name) {
            if (entry == nullptr) {
                return;
            }
            entryPath = entryPath.GetChild(name);
            if (entry->subentries.count(name)) {
                entry = &entry->subentries[name];
            } else {
                entry = nullptr;
            }
        });
        if (entry) {
            entry->ns = nullptr;
        } else {
            throw SlokedError(std::string{"Not mounted: "} + path.ToString());
        }
        this->cleanup(this->root);
    }

    std::unique_ptr<SlokedNamespaceObject> SlokedVirtualNamespace::GetObject(const SlokedPath &path) const {
        SlokedPath realPath = path.IsAbsolute() ? path : path.RelativeTo(SlokedPath::Root);
        const Entry &entry = this->find(realPath);
        SlokedPath nsPath = realPath.RelativeTo(entry.path).RelativeTo(SlokedPath::Root);
        return entry.ns->GetObject(nsPath);
    }

    bool SlokedVirtualNamespace::HasObject(const SlokedPath &path) const {
        SlokedPath realPath = path.IsAbsolute() ? path : path.RelativeTo(SlokedPath::Root);
        const Entry &entry = this->find(realPath);
        SlokedPath nsPath = realPath.RelativeTo(entry.path).RelativeTo(SlokedPath::Root);
        return entry.ns->HasObject(nsPath);
    }
    
    void SlokedVirtualNamespace::Iterate(const SlokedPath &path, Visitor visitor) const {
        SlokedPath realPath = path.IsAbsolute() ? path : path.RelativeTo(SlokedPath::Root);
        const Entry &entry = this->find(realPath);
        SlokedPath nsPath = realPath.RelativeTo(entry.path).RelativeTo(SlokedPath::Root);
        entry.ns->Iterate(nsPath, visitor);
    }

    std::unique_ptr<SlokedNamespaceObjectHandle> SlokedVirtualNamespace::GetHandle(const SlokedPath &path) {
        return std::make_unique<SlokedVirtualObjectHandle>(*this, path);
    }

    const SlokedVirtualNamespace::Entry &SlokedVirtualNamespace::find(const SlokedPath &path) const {
        const Entry *entry = &this->root;
        const Entry *actualEntry = &this->root;
        for (const auto &name : path.Components()) {
            if (name.empty()) {
                continue;
            }
            if (entry->subentries.count(name)) {
                entry = &entry->subentries.at(name);
                if (entry->ns) {
                    actualEntry = entry;
                }
            } else {
                break;
            }
        }
        return *actualEntry;
    }

    void SlokedVirtualNamespace::cleanup(Entry &entry) {
        for (auto &kv : entry.subentries) {
            this->cleanup(kv.second);
        }

        for (auto it = entry.subentries.begin(); it != entry.subentries.end();) {
            if (it->second.ns == nullptr && it->second.subentries.empty()) {
                it = entry.subentries.erase(it);
            } else {
                ++it;
            }
        }
    }
}