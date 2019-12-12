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

#ifndef SLOKED_NAMESPACE_PATH_H_
#define SLOKED_NAMESPACE_PATH_H_

#include "sloked/Base.h"
#include <string>
#include <vector>

namespace sloked {

    class SlokedPath {
     public:
        class Preset {
         public:
            explicit Preset(const std::string &, const std::string & = ".", const std::string & = "..", const std::string & = "~");
            const std::string &GetSeparators() const;
            const std::string &GetCurrentDir() const;
            const std::string &GetParentDir() const;
            const std::string &GetHomeDir() const;

            bool operator==(const Preset &) const;
            bool operator!=(const Preset &) const;

         private:
            std::string separators;
            std::string currentDir;
            std::string parentDir;
            std::string homeDir;
        };

        class String {
         public:
            String(std::string_view);
            String(const std::string &);
            String(const char *);

            operator std::string_view() const;

         private:
            std::string_view value;
        };

        SlokedPath(String, String, Preset = Preset{std::string{"/"}});
        SlokedPath(String, Preset = Preset{std::string{"/"}});
        SlokedPath(const SlokedPath &) = default;
        SlokedPath(SlokedPath &&) = default;
        virtual ~SlokedPath() = default;

        SlokedPath &operator=(const SlokedPath &) = default;
        SlokedPath &operator=(SlokedPath &&) = default;

        const Preset &GetPreset() const;
        const std::string &GetPrefix() const;
        const std::vector<std::string> &Components() const;
        const std::string &ToString() const;
        
        bool IsAbsolute() const;
        bool IsParent(const SlokedPath &) const;
        
        SlokedPath RelativeTo(const SlokedPath &) const;
        SlokedPath Parent() const;
        SlokedPath Child(String) const;
        SlokedPath Tail(std::size_t = 1) const;
        SlokedPath Migrate(String, const Preset &) const;
        SlokedPath Migrate(const Preset &) const;
        SlokedPath Migrate(String) const;
        SlokedPath Root() const;

        operator const std::string &() const;
        SlokedPath operator[](String) const;
        bool operator==(const SlokedPath &) const;

     private:
        SlokedPath(const Preset &);
        SlokedPath &Normalize();

        Preset preset;
        bool absolute;
        std::string prefix;
        std::vector<std::string> path;
        std::string literal;
    };
}

#endif