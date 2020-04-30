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

#include "sloked/filesystem/posix/Reader.h"

#include <memory>

namespace sloked {

    SlokedPosixFileReader::Offset SlokedPosixFileIO::Tell() {
        return ftell(this->file);
    }

    SlokedPosixFileReader::SlokedPosixFileReader(FILE *file)
        : SlokedPosixFileIO(file) {}

    std::string SlokedPosixFileReader::Read(std::size_t sz) {
        std::unique_ptr<char[]> buffer(new char[sz]);
        std::size_t length = fread(buffer.get(), sizeof(char), sz, this->file);
        return std::string{buffer.get(), length};
    }

    int SlokedPosixFileReader::Read() {
        return fgetc(this->file);
    }

    bool SlokedPosixFileReader::Unread(int c) {
        return ungetc(c, this->file) != EOF;
    }

    bool SlokedPosixFileReader::Eof() {
        return static_cast<bool>(feof(this->file));
    }
}  // namespace sloked