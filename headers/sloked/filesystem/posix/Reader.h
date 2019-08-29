/*
  SPDX-License-Identifier: LGPL-3.0-or-later

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_FILESYSTEM_POSIX_READER_H_
#define SLOKED_FILESYSTEM_POSIX_READER_H_

#include "sloked/filesystem/posix/FileIO.h"

namespace sloked {

    class SlokedPosixFileReader : public SlokedPosixFileIO, public SlokedIOReader {
     public:
        SlokedPosixFileReader(FILE *);
        SlokedPosixFileReader(SlokedPosixFileReader &&) = default;
        SlokedPosixFileReader &operator=(SlokedPosixFileReader &&) = default;

        std::string Read(std::size_t) override;
        int Read() override;
        bool Unread(int) override;
        bool Eof() override;
    };
}

#endif