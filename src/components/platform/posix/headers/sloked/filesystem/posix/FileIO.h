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

#ifndef SLOKED_FILESYSTEM_POSIX_FILEIO_H_
#define SLOKED_FILESYSTEM_POSIX_FILEIO_H_

#include <cstdio>

#include "sloked/core/IO.h"

namespace sloked {

    class SlokedPosixFileIO : public virtual SlokedBaseIO {
     public:
        SlokedPosixFileIO(FILE *);
        virtual ~SlokedPosixFileIO();

        SlokedPosixFileIO(const SlokedPosixFileIO &) = delete;
        SlokedPosixFileIO(SlokedPosixFileIO &&);
        SlokedPosixFileIO &operator=(const SlokedPosixFileIO &) = delete;
        SlokedPosixFileIO &operator=(SlokedPosixFileIO &&);

        void Close() override;
        Offset Tell() override;
        bool Seek(Offset, Origin) override;
        bool HasErrors() override;
        void ClearErrors() override;

     protected:
        FILE *file;
    };
}  // namespace sloked

#endif