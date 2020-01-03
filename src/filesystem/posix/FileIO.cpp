/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#include "sloked/filesystem/posix/FileIO.h"

namespace sloked {

    SlokedPosixFileIO::SlokedPosixFileIO(FILE *file)
        : file(file) {}

    SlokedPosixFileIO::~SlokedPosixFileIO() {
        this->Close();
    }

    SlokedPosixFileIO::SlokedPosixFileIO(SlokedPosixFileIO &&io)
        : file(io.file) {
        io.file = nullptr;
    }

    SlokedPosixFileIO &SlokedPosixFileIO::operator=(SlokedPosixFileIO &&io) {
        this->Close();
        this->file = io.file;
        io.file = nullptr;
        return *this;
    }

    void SlokedPosixFileIO::Close() {
        if (this->file) {
            fclose(this->file);
            this->file = nullptr;
        }
    }

    bool SlokedPosixFileIO::HasErrors() {
        return ferror(this->file) != 0;
    }

    bool SlokedPosixFileIO::Seek(Offset pos, Origin origin) {
        static int origins[] = {
            SEEK_SET,
            SEEK_CUR,
            SEEK_END
        };
        return fseek(this->file, pos, origins[static_cast<std::size_t>(origin)]);
    }

    void SlokedPosixFileIO::ClearErrors() {
        clearerr(this->file);
    }
}