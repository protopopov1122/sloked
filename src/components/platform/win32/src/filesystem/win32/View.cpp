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

#include "sloked/filesystem/win32/View.h"
#include "sloked/core/Error.h"
#include <io.h>

namespace sloked {

    SlokedWin32FileView::SlokedWin32FileView(FILE *file) : file(file) {
		auto handle = (HANDLE) _get_osfhandle(_fileno(this->file));
		this->length = GetFileSize(handle, NULL);
        this->mapFile = CreateFileMapping(handle,
                NULL,
                PAGE_READONLY,
                0,
                0,
                NULL);
		SYSTEM_INFO SysInfo;
		GetSystemInfo(&SysInfo);
		this->data = MapViewOfFile(this->mapFile,
                               FILE_MAP_READ,
                               0,
                               0,
                               0);
    }

    SlokedWin32FileView::~SlokedWin32FileView() {
        UnmapViewOfFile(this->data);
		CloseHandle(this->mapFile);
        fclose(this->file);
    }

    std::string_view SlokedWin32FileView::GetView() const {
        return std::string_view(static_cast<const char *>(this->data),
                                this->length);
    }
}  // namespace sloked
