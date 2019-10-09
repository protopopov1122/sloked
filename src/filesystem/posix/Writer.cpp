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

#include "sloked/filesystem/posix/Writer.h"
// #include <unistd.h>

namespace sloked {

    SlokedPosixFileWriter::SlokedPosixFileWriter(FILE *file)
        : SlokedPosixFileIO(file) {}
    
    std::size_t SlokedPosixFileWriter::Write(std::string_view str) {
        // ftruncate(fileno(this->file), ftell(this->file) + sizeof(std::string_view::value_type) * str.size());
        return fwrite(str.data(), sizeof(std::string_view::value_type), str.size(), this->file);
    }
    
    bool SlokedPosixFileWriter::Write(Char chr) {
        return fputc(chr, this->file) != EOF;
    }

    bool SlokedPosixFileWriter::Flush() {
        return fflush(this->file) != EOF;
    }
}