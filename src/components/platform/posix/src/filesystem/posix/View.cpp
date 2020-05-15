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

#include "sloked/filesystem/posix/View.h"
#include "sloked/core/Error.h"

#include <sys/mman.h>
#include <sys/stat.h>

namespace sloked {

    SlokedPosixFileView::SlokedPosixFileView(FILE *file) : file(file) {
        struct stat file_stats;
        if (fstat(fileno(this->file), &file_stats) != 0) {
            throw SlokedError("PosixFileView: Error getting file stats");
        }
        this->length = file_stats.st_size;
#ifdef SLOKED_PLATFORM_OS_LINUX
        this->data = mmap(nullptr, this->length, PROT_READ,
                          MAP_PRIVATE | MAP_POPULATE, fileno(this->file), 0);
#elif defined(SLOKED_PLATFORM_OS_UNIX)
        this->data = mmap(nullptr, this->length, PROT_READ, MAP_PRIVATE,
                          fileno(this->file), 0);
#else
#error "Error calling mmap function"
#endif
    }

    SlokedPosixFileView::~SlokedPosixFileView() {
        munmap(this->data, this->length);
        fclose(this->file);
    }

    std::string_view SlokedPosixFileView::GetView() const {
        return std::string_view(static_cast<const char *>(this->data),
                                this->length);
    }
}  // namespace sloked
