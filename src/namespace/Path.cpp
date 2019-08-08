#include "sloked/namespace/Path.h"
#include <algorithm>
#include <iostream>

namespace sloked {

    SlokedPath::SlokedPath(std::string_view str) {
        this->path = {""};
        for (char chr : str) {
            if (chr == SlokedPath::Separator) {
                this->path.push_back("");
            } else {
                this->path.back().push_back(chr);
            }
        }
    }

    bool SlokedPath::IsAbsolute() const {
        return this->path.front().empty();
    }

    bool SlokedPath::IsNormalized() const {
        bool parentRow = false;
        for (auto it = this->path.begin(); it != this->path.end(); ++it) {
            const auto &name = *it;
            if (it == this->path.begin()) {
                if (name == SlokedPath::ParentDir) {
                    parentRow = true;
                }
            } else if (name.empty() ||
                name == SlokedPath::CurrentDir ||
                (name == SlokedPath::ParentDir && !parentRow)) {
                return false;
            } else if (name == SlokedPath::ParentDir) {
                parentRow = true;
            } else {
                parentRow = false;
            }
        }
        return true;
    }

    std::string SlokedPath::ToString() const {
        std::string path {this->path.front()};
        if (this->path.size() == 1 && this->path.front().empty()) {
            path.push_back(SlokedPath::Separator);
        }
        for (auto it = std::next(this->path.begin()); it != this->path.end(); ++it) {
            path.push_back(SlokedPath::Separator);
            path.append(*it);
        }
        return path;
    }

    SlokedPath SlokedPath::RelativeTo(const SlokedPath &root) const {
        if (!this->IsAbsolute()) {
            SlokedPath path(*this);
            path.path.insert(path.path.begin(), root.path.begin(), root.path.end());
            path.Normalize();
            return path;
        } else if (root.IsAbsolute()) {
            SlokedPath path;
            path.path = {"."};
            SlokedPath normRoot(root);
            normRoot.Normalize();
            SlokedPath normSelf(*this);
            normSelf.Normalize();
            std::size_t i = 0;
            for (; i < std::min(normRoot.path.size(), normSelf.path.size()); i++) {
                if (normRoot.path[i] != normSelf.path[i]) {
                    break;
                }
            }
            if (i < normRoot.path.size()) {
                path.path = {normRoot.path.size() - i, SlokedPath::ParentDir};
            }
            path.path.insert(path.path.end(), normSelf.path.begin() + i, normSelf.path.end());
            return path;
        } else {
            SlokedPath path(root);
            path.path.insert(path.path.begin(), this->path.begin(), this->path.end());
            path.Normalize();
            return path;
        }
    }

    void SlokedPath::Iterate(Visitor visitor) const {
        for (const auto &name : this->path) {
            visitor(name);
        }
    }

    SlokedPath SlokedPath::GetParent() const {
        if (this->path.size() == 1) {
            return *this;
        } else {
            SlokedPath path;
            path.path.insert(path.path.end(), this->path.begin(), std::prev(this->path.end()));
            return path;
        }
    }

    SlokedPath SlokedPath::GetChild(std::string_view name) const {
        SlokedPath path;
        path.path = this->path;
        path.path.push_back(std::string{name});
        return path;
    }

    bool SlokedPath::IsDescendent(const SlokedPath &path) const {
        if (this->IsAbsolute() == path.IsAbsolute()) {
            SlokedPath normSelf(*this);
            normSelf.Normalize();
            SlokedPath normPath(path);
            normPath.Normalize();

            if (normPath.path.size() < normSelf.path.size()) {
                return false;
            }
            for (std::size_t i = 0; i < normSelf.path.size(); i++) {
                if (normSelf.path[i] != normPath.path[i]) {
                    return false;
                }
            }
            return true;
        } else if (this->IsAbsolute()) {
            return this->IsDescendent(path.RelativeTo(*this));
        } else {
            SlokedPath self = this->RelativeTo(path);
            return self.IsDescendent(path);
        }
    }

    SlokedPath &SlokedPath::Normalize() {
        // Remove '.'s and ''s
        for (std::size_t i = 1; i < this->path.size(); i++) {
            if (this->path[i] == SlokedPath::CurrentDir ||
                this->path[i].empty()) {
                this->path.erase(this->path.begin() + i);
                i--;
            }
        }

        // Remove '..'s
        for (std::size_t i = 1; i < this->path.size(); i++) {
            if (this->path[i] != SlokedPath::ParentDir) {
                continue;
            }
            if (this->path[i - 1] == SlokedPath::CurrentDir) {
                this->path.erase(this->path.begin() + (i - 1));
                i--;
            } else if (this->path[i - 1].empty()) {
                this->path.erase(this->path.begin() + i);
                i--;
            } else if (this->path[i - 1] != SlokedPath::ParentDir) {
                this->path.erase(this->path.begin() + i);
                this->path.erase(this->path.begin() + (i - 1));
                i -= 2;
            }
        }
        return *this;
    }
}