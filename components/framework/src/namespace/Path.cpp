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

#include "sloked/namespace/Path.h"

#include <algorithm>
#include <iostream>

#include "sloked/core/Error.h"

namespace sloked {

    SlokedPath::Preset::Preset(const std::string &separators,
                               const std::string &currentDir,
                               const std::string &parentDir,
                               const std::string &homeDir)
        : separators(separators), currentDir(currentDir), parentDir(parentDir),
          homeDir(homeDir) {}

    const std::string &SlokedPath::Preset::GetSeparators() const {
        return this->separators;
    }

    const std::string &SlokedPath::Preset::GetCurrentDir() const {
        return this->currentDir;
    }

    const std::string &SlokedPath::Preset::GetParentDir() const {
        return this->parentDir;
    }

    const std::string &SlokedPath::Preset::GetHomeDir() const {
        return this->homeDir;
    }

    bool SlokedPath::Preset::operator==(const Preset &other) const {
        return this->separators == other.separators &&
               this->currentDir == other.currentDir &&
               this->parentDir == other.parentDir &&
               this->homeDir == other.homeDir;
    }

    bool SlokedPath::Preset::operator!=(const Preset &other) const {
        return !this->operator==(other);
    }

    SlokedPath::String::String(std::string_view value) : value(value) {}

    SlokedPath::String::String(const std::string &value) : value(value) {}

    SlokedPath::String::String(const char *value) : value(value) {}

    SlokedPath::String::operator std::string_view() const {
        return this->value;
    }

    SlokedPath::SlokedPath(String pathValue)
        : SlokedPath(pathValue, Preset{std::string{"/"}}) {}

    SlokedPath::SlokedPath(String prefix, String pathValue, Preset preset)
        : preset(std::move(preset)), absolute{false}, literal("") {
        this->prefix = prefix;
        std::string_view path = pathValue;
        std::size_t offset, start = 0;
        while ((offset = path.find_first_of(this->preset.GetSeparators(),
                                            start)) != path.npos) {
            if (offset == 0) {
                this->absolute = true;
                start = 1;
            } else if (offset > start) {
                this->path.push_back(
                    std::string{path.substr(start, offset - start)});
                start = offset + 1;
            } else {
                start++;
            }
        }
        if (start < path.size()) {
            this->path.push_back(
                std::string{path.substr(start, path.size() - start)});
        }
        if (!this->path.empty() &&
            this->path.front() == this->preset.GetHomeDir() &&
            !this->preset.GetHomeDir().empty()) {
            this->absolute = true;
        }
        this->Normalize();
    }

    SlokedPath::SlokedPath(String pathValue, Preset preset)
        : SlokedPath("", pathValue, preset) {}

    const SlokedPath::Preset &SlokedPath::GetPreset() const {
        return this->preset;
    }

    const std::string &SlokedPath::GetPrefix() const {
        return this->prefix;
    }

    const std::vector<std::string> &SlokedPath::Components() const {
        return this->path;
    }

    const std::string &SlokedPath::ToString() const {
        return this->literal;
    }

    bool SlokedPath::IsAbsolute() const {
        return this->absolute;
    }

    bool SlokedPath::IsParent(const SlokedPath &path) const {
        if (this->IsAbsolute() == path.IsAbsolute()) {
            if (path.path.size() < this->path.size()) {
                return false;
            }
            for (std::size_t i = 0; i < this->path.size(); i++) {
                if (this->path[i] != path.path[i]) {
                    return false;
                }
            }
            return true;
        } else if (this->IsAbsolute()) {
            return this->IsParent(path.RelativeTo(*this));
        } else {
            SlokedPath self = this->RelativeTo(path);
            return self.IsParent(path);
        }
    }

    SlokedPath SlokedPath::RelativeTo(const SlokedPath &root) const {
        if (this->preset != root.preset) {
            throw SlokedError("Path: Different presets");
        }
        if (!this->IsAbsolute()) {
            if (this->prefix != root.prefix && !this->prefix.empty()) {
                throw SlokedError("Path: Different prefixes");
            }
            SlokedPath path(root);
            path.path.insert(path.path.end(), this->path.begin(),
                             this->path.end());
            path.Normalize();
            return path;
        } else if (root.IsAbsolute()) {
            if (this->prefix != root.prefix) {
                throw SlokedError("Path: Different prefixes");
            }
            SlokedPath path(this->preset);
            path.prefix = "";
            path.path = {"."};
            std::size_t i = 0;
            for (; i < std::min(root.path.size(), this->path.size()); i++) {
                if (root.path[i] != this->path[i]) {
                    break;
                }
            }
            if (i < root.path.size()) {
                path.path = {root.path.size() - i, this->preset.GetParentDir()};
            }
            path.path.insert(path.path.end(), this->path.begin() + i,
                             this->path.end());
            path.Normalize();
            return path;
        } else {
            return root.RelativeTo(*this);
        }
    }

    SlokedPath SlokedPath::Parent() const {
        if (this->path.size() == 0) {
            return *this;
        } else {
            SlokedPath path(this->preset);
            path.absolute = this->absolute;
            path.path.insert(path.path.end(), this->path.begin(),
                             std::prev(this->path.end()));
            path.Normalize();
            return path;
        }
    }

    SlokedPath SlokedPath::Child(String name) const {
        SlokedPath path(this->preset);
        path.absolute = this->absolute;
        path.path = this->path;
        path.path.push_back(std::string{name});
        path.Normalize();
        return path;
    }

    SlokedPath SlokedPath::Tail(std::size_t count) const {
        auto begin = this->path.begin();
        if (begin != this->path.end() && (*begin == "." || *begin == "..")) {
            ++begin;
        }
        if (count >
            static_cast<std::size_t>(std::distance(begin, this->path.end()))) {
            throw SlokedError("Path: too large shift");
        } else {
            SlokedPath shifted(this->preset);
            shifted.path = std::vector(begin + count, this->path.end());
            if (begin != this->path.begin()) {
                shifted.path.insert(shifted.path.begin(), *this->path.begin());
            }
            shifted.Normalize();
            return shifted;
        }
    }

    SlokedPath SlokedPath::Migrate(String prefix, const Preset &next) const {
        SlokedPath path(next);
        path.prefix = prefix;
        path.absolute = this->absolute;
        for (const auto &component : this->path) {
            if (component == this->preset.GetCurrentDir() &&
                !this->preset.GetCurrentDir().empty()) {
                path.path.push_back(next.GetCurrentDir());
            } else if (component == this->preset.GetParentDir() &&
                       !this->preset.GetParentDir().empty()) {
                path.path.push_back(next.GetParentDir());
            } else if (component == this->preset.GetHomeDir() &&
                       !this->preset.GetHomeDir().empty()) {
                path.path.push_back(next.GetHomeDir());
            } else {
                path.path.push_back(component);
            }
        }
        path.Normalize();
        return path;
    }

    SlokedPath SlokedPath::Migrate(String prefix) const {
        return this->Migrate(prefix, this->preset);
    }

    SlokedPath SlokedPath::Migrate(const Preset &next) const {
        return this->Migrate(this->prefix, next);
    }

    SlokedPath SlokedPath::Root() const {
        return SlokedPath(this->prefix,
                          this->preset.GetSeparators().substr(0, 1),
                          this->preset);
    }

    SlokedPath::operator const std::string &() const {
        return this->ToString();
    }

    SlokedPath SlokedPath::operator[](String name) const {
        return this->Child(name);
    }

    bool SlokedPath::operator==(const SlokedPath &path) const {
        if (this->path.size() != path.path.size()) {
            return false;
        }
        for (std::size_t i = 0; i < this->path.size(); i++) {
            if (this->path[i] != path.path[i]) {
                return false;
            }
        }
        return true;
    }

    bool SlokedPath::operator<(const SlokedPath &other) const {
        return this->literal < other.literal;
    }

    SlokedPath::SlokedPath(const Preset &preset)
        : preset(preset), absolute{false} {}

    SlokedPath &SlokedPath::Normalize() {
        // Remove '.'s and ''s
        for (std::size_t i = this->absolute ? 0 : 1; i < this->path.size();
             i++) {
            if (this->path[i] == this->preset.GetCurrentDir() &&
                !this->preset.GetCurrentDir().empty()) {
                this->path.erase(this->path.begin() + i);
                i--;
            }
        }

        // Remove '..'s
        for (std::size_t i = 1; i < this->path.size(); i++) {
            if (this->path[i] != this->preset.GetParentDir() ||
                this->preset.GetParentDir().empty()) {
                continue;
            }
            if (this->path[i - 1] == this->preset.GetCurrentDir() &&
                !this->preset.GetCurrentDir().empty()) {
                this->path.erase(this->path.begin() + (i - 1));
                i--;
            } else if (this->path[i - 1] != this->preset.GetParentDir()) {
                this->path.erase(this->path.begin() + i);
                this->path.erase(this->path.begin() + (i - 1));
                i -= 2;
            }
        }

        // Update literal path
        if (this->IsAbsolute() &&
            (this->path.empty() ||
             this->path.front() != this->preset.GetHomeDir() ||
             this->preset.GetHomeDir().empty())) {
            this->literal = this->prefix;
            this->literal.push_back(this->preset.GetSeparators().at(0));
        } else {
            this->literal = "";
        }
        for (std::size_t i = 0; i < this->path.size(); i++) {
            literal += this->path.at(i);
            if (i + 1 < this->path.size()) {
                this->literal.push_back(this->preset.GetSeparators().at(0));
            }
        }
        return *this;
    }
}  // namespace sloked