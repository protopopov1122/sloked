#include "sloked/namespace/Virtual.h"
#include <iostream>

namespace sloked {

    SlokedVirtualNamespace::SlokedVirtualNamespace(std::unique_ptr<SlokedNamespace> ns)
        : root{std::move(ns), SlokedPath("/"), {}} {}

    void SlokedVirtualNamespace::Mount(const SlokedPath &path, std::unique_ptr<SlokedNamespace> ns) {
        SlokedPath realPath = path.IsAbsolute() ? path : path.RelativeTo(SlokedPath("/"));
        Entry *entry = &this->root;
        SlokedPath entryPath("/");
        if (realPath == entryPath) {
            return;
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
        SlokedPath realPath = path.IsAbsolute() ? path : path.RelativeTo(SlokedPath("/"));
        Entry *entry = &this->root;
        SlokedPath entryPath("/");
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
        }
        this->cleanup(this->root);
    }

    std::unique_ptr<SlokedNamespaceObject> SlokedVirtualNamespace::GetObject(const SlokedPath &path) const {
        SlokedPath realPath = path.IsAbsolute() ? path : path.RelativeTo(SlokedPath("/"));
        const Entry &entry = this->find(realPath);
        SlokedPath nsPath = realPath.RelativeTo(entry.path).RelativeTo(SlokedPath("/"));
        return entry.ns->GetObject(nsPath);
    }

    bool SlokedVirtualNamespace::HasObject(const SlokedPath &path) const {
        SlokedPath realPath = path.IsAbsolute() ? path : path.RelativeTo(SlokedPath("/"));
        const Entry &entry = this->find(realPath);
        SlokedPath nsPath = realPath.RelativeTo(entry.path).RelativeTo(SlokedPath("/"));
        return entry.ns->HasObject(nsPath);
    }
    
    void SlokedVirtualNamespace::Iterate(const SlokedPath &path, Visitor visitor) const {
        SlokedPath realPath = path.IsAbsolute() ? path : path.RelativeTo(SlokedPath("/"));
        const Entry &entry = this->find(realPath);
        SlokedPath nsPath = realPath.RelativeTo(entry.path).RelativeTo(SlokedPath("/"));
        entry.ns->Iterate(nsPath, visitor);
    }

    void SlokedVirtualNamespace::MakeDir(const SlokedPath &path) {
        SlokedPath realPath = path.IsAbsolute() ? path : path.RelativeTo(SlokedPath("/"));
        const Entry &entry = this->find(realPath);
        SlokedPath nsPath = realPath.RelativeTo(entry.path).RelativeTo(SlokedPath("/"));
        entry.ns->MakeDir(nsPath);
    }

    void SlokedVirtualNamespace::Delete(const SlokedPath &path) {
        SlokedPath realPath = path.IsAbsolute() ? path : path.RelativeTo(SlokedPath("/"));
        const Entry &entry = this->find(realPath);
        SlokedPath nsPath = realPath.RelativeTo(entry.path).RelativeTo(SlokedPath("/"));
        entry.ns->Delete(nsPath);
    }

    void SlokedVirtualNamespace::Rename(const SlokedPath &from, const SlokedPath &to) {
        SlokedPath realFrom = from.IsAbsolute() ? from : from.RelativeTo(SlokedPath("/"));
        SlokedPath realTo = to.IsAbsolute() ? to : to.RelativeTo(SlokedPath("/"));
        const Entry &entry = this->find(realFrom);
        if (entry.path.IsChildOrSelf(realTo)) {
            SlokedPath nsFromPath = realFrom.RelativeTo(entry.path).RelativeTo(SlokedPath("/"));
            SlokedPath nsToPath = realTo.RelativeTo(entry.path).RelativeTo(SlokedPath("/"));
            entry.ns->Rename(nsFromPath, nsToPath);
        }
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