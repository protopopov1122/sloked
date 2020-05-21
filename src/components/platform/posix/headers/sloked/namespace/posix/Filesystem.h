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

#ifndef SLOKED_NAMESPACE_POSIX_FILESYSTEM_H_
#define SLOKED_NAMESPACE_POSIX_FILESYSTEM_H_

#include "sloked/namespace/Filesystem.h"

namespace sloked {

    class SlokedPosixFilesystemAdapter : public SlokedFilesystemAdapter {
     public:
        SlokedPosixFilesystemAdapter(std::string_view);

        const SlokedPath &GetRoot() const override;
        std::unique_ptr<SlokedFile> NewFile(const SlokedPath &) const override;
        SlokedPath ToPath(const std::string &) const override;
        std::string FromPath(const SlokedPath &) const override;
        std::string ToURI(const SlokedPath &) const override;
        std::unique_ptr<SlokedFilesystemAdapter> Rebase(
            std::string_view) const override;

     private:
        SlokedPath rootPath;
    };
}  // namespace sloked

#endif