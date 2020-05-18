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

#ifndef SLOKED_FILESYSTEM_WIN32_READER_H_
#define SLOKED_FILESYSTEM_WIN32_READER_H_

#include "sloked/filesystem/win32/FileIO.h"

namespace sloked {

    class SlokedWin32FileReader : public SlokedWin32FileIO,
                                  public SlokedIOReader {
     public:
        SlokedWin32FileReader(FILE *);
        SlokedWin32FileReader(SlokedWin32FileReader &&) = default;
        SlokedWin32FileReader &operator=(SlokedWin32FileReader &&) = default;

        std::string Read(std::size_t) override;
        int Read() override;
        bool Unread(int) override;
        bool Eof() override;
    };
}  // namespace sloked

#endif