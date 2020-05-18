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

#include "sloked/filesystem/win32/Reader.h"

#include <memory>

namespace sloked {

    SlokedWin32FileReader::Offset SlokedWin32FileIO::Tell() {
        return ftell(this->file);
    }

    SlokedWin32FileReader::SlokedWin32FileReader(FILE *file)
        : SlokedWin32FileIO(file) {}

    std::string SlokedWin32FileReader::Read(std::size_t sz) {
        std::unique_ptr<char[]> buffer(new char[sz]);
        std::size_t length = fread(buffer.get(), sizeof(char), sz, this->file);
        return std::string{buffer.get(), length};
    }

    int SlokedWin32FileReader::Read() {
        return fgetc(this->file);
    }

    bool SlokedWin32FileReader::Unread(int c) {
        return ungetc(c, this->file) != EOF;
    }

    bool SlokedWin32FileReader::Eof() {
        return static_cast<bool>(feof(this->file));
    }
}  // namespace sloked