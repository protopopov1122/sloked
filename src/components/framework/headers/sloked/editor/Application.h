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

#ifndef SLOKED_EDITOR_APPLICATION_H_
#define SLOKED_EDITOR_APPLICATION_H_

#include "sloked/core/CLI.h"
#include "sloked/core/Closeable.h"
#include "sloked/core/Logger.h"
#include "sloked/editor/BaseInterface.h"
#include "sloked/editor/Configuration.h"
#include "sloked/editor/Manager.h"
#include "sloked/editor/doc-mgr/Document.h"
#include "sloked/text/fragment/TaggedText.h"

namespace sloked {

    class SlokedApplication {
     public:
        virtual ~SlokedApplication() = default;
        virtual int Start(int, const char **, const SlokedBaseInterface &,
                          SlokedSharedContainerEnvironment &, SlokedEditorManager &) = 0;
    };

    using SlokedApplicationFactory = SlokedApplication *(*) ();

    extern "C" SlokedApplication *SlokedMakeApplication();
}  // namespace sloked

#endif
