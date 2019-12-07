/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/namespace/Path.h"
#include "sloked/core/Error.h"
#include <algorithm>
#include <iostream>

namespace sloked {

    SlokedPath::Preset::Preset(char separator, const std::string &currentDir, const std::string &parentDir)
        : separator(separator), currentDir(currentDir), parentDir(parentDir) {}

    char SlokedPath::Preset::GetSeparator() const {
        return this->separator;
    }

    const std::string &SlokedPath::Preset::GetCurrentDir() const {
        return this->currentDir;
    }

    const std::string &SlokedPath::Preset::GetParentDir() const {
        return this->parentDir;
    }

    bool SlokedPath::Preset::operator==(const Preset &other) const {
        return this->separator == other.separator &&
            this->currentDir == other.currentDir &&
            this->parentDir == other.parentDir;
    }

    bool SlokedPath::Preset::operator!=(const Preset &other) const {
        return !this->operator==(other);
    }

    SlokedPath::String::String(std::string_view value)
        : value(value) {}

    SlokedPath::String::String(const std::string &value)
        : value(value) {}

    SlokedPath::String::String(const char *value)
        : value(value) {}

    SlokedPath::String::operator std::string_view() const {
        return this->value;
    }

    SlokedPath::SlokedPath(String prefix, String pathValue, Preset preset)
        : preset(std::move(preset)), absolute{true}, literal("") {
        this->prefix = prefix;
        std::string_view path = pathValue;
        for (char chr : path) {
            if (chr == this->preset.GetSeparator()) {
                if (this->path.empty()) {
                    this->absolute = true;
                } else if (!this->path.back().empty()) {
                    this->path.push_back("");
                }
            } else {
                if (this->path.empty()) {
                    this->path.push_back(std::string{chr});
                } else {
                    this->path.back().push_back(chr);
                }
            }
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
            SlokedPath path(root);
            path.path.insert(path.path.end(), this->path.begin(), this->path.end());
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
            path.path.insert(path.path.end(), this->path.begin() + i, this->path.end());
            path.Normalize();
            return path;
        } else {
            return root.RelativeTo(*this);
        }
    }

    SlokedPath SlokedPath::Parent() const {
        if (this->path.size() == 1) {
            return *this;
        } else {
            SlokedPath path(this->preset);
            path.path.insert(path.path.end(), this->path.begin(), std::prev(this->path.end()));
            path.Normalize();
            return path;
        }
    }

    SlokedPath SlokedPath::Child(String name) const {
        SlokedPath path(this->preset);
        path.path = this->path;
        path.path.push_back(std::string{name});
        path.Normalize();
        return path;
    }

    SlokedPath SlokedPath::Tail(std::size_t count) const {
        auto begin = this->path.begin();
        if (begin != this->path.end() &&
            (*begin == "." || *begin == "..")) {
            ++begin;
        }
        if (count > static_cast<std::size_t>(std::distance(begin, this->path.end()))) {
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
            if (component == this->preset.GetCurrentDir()) {
                path.path.push_back(next.GetCurrentDir());
            } else if (component == this->preset.GetParentDir()) {
                path.path.push_back(next.GetParentDir());
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
        return SlokedPath(this->prefix, std::string{this->preset.GetSeparator()}, this->preset);
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

    SlokedPath::SlokedPath(const Preset &preset)
        : preset(preset), absolute{false} {}

    SlokedPath &SlokedPath::Normalize() {
        // Remove '.'s and ''s
        for (std::size_t i = this->absolute ? 0 : 1; i < this->path.size(); i++) {
            if (this->path[i] == this->preset.GetCurrentDir()) {
                this->path.erase(this->path.begin() + i);
                i--;
            }
        }

        // Remove '..'s
        for (std::size_t i = 1; i < this->path.size(); i++) {
            if (this->path[i] != this->preset.GetParentDir()) {
                continue;
            }
            if (this->path[i - 1] == this->preset.GetCurrentDir()) {
                this->path.erase(this->path.begin() + (i - 1));
                i--;
            } else if (this->path[i - 1] != this->preset.GetParentDir()) {
                this->path.erase(this->path.begin() + i);
                this->path.erase(this->path.begin() + (i - 1));
                i -= 2;
            }
        }

        // Update literal path
        if (this->IsAbsolute()) {
            this->literal = this->prefix;
            this->literal.push_back(this->preset.GetSeparator());
        } else {
            this->literal = "";
        }
        for (std::size_t i = 0; i < this->path.size(); i++) {
            literal += this->path.at(i);
            if (i + 1 < this->path.size()) {
                this->literal.push_back(this->preset.GetSeparator());
            }
        }
        return *this;
    }
}