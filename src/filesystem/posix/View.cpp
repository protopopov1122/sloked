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

#include "sloked/filesystem/posix/View.h"
#include <sys/stat.h>
#include <sys/mman.h>

namespace sloked {

    SlokedPosixFileView::SlokedPosixFileView(FILE *file)
        : file(file) {
        struct stat file_stats;
        fstat(fileno(this->file), &file_stats);
        this->length = file_stats.st_size;
        this->data = mmap(nullptr, this->length, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fileno(this->file), 0);
    }
    
    SlokedPosixFileView::~SlokedPosixFileView() {
        munmap(this->data, this->length);
        fclose(this->file);
    }

    std::string_view SlokedPosixFileView::GetView() const {
        return std::string_view(static_cast<const char *>(this->data), this->length);
    }
}