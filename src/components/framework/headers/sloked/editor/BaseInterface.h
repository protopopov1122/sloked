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

#ifndef SLOKED_EDITOR_BASEINTERFACE_H_
#define SLOKED_EDITOR_BASEINTERFACE_H_

#include "sloked/core/Compression.h"
#include "sloked/core/Crypto.h"
#include "sloked/core/Library.h"
#include "sloked/core/awaitable/Awaitable.h"
#include "sloked/editor/Configuration.h"
#include "sloked/editor/EditorInstance.h"
#include "sloked/net/Socket.h"
#include "sloked/sched/Scheduler.h"
#include "sloked/screen/Manager.h"
#include "sloked/screen/graphics/GUI.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/TerminalSize.h"
#include "sloked/script/ScriptEngine.h"

namespace sloked {

    class SlokedBaseInterface {
     public:
        using OnTerminalResize = std::function<void()>;
        using UnbindResizeListener = std::function<void()>;

        virtual ~SlokedBaseInterface() = default;

        virtual bool SupportsCompression() const = 0;
        virtual SlokedCompression &GetCompression() const = 0;
        virtual std::unique_ptr<SlokedIOPoll> NewAwaitablePoll() const = 0;
        virtual bool HasCryptography() const = 0;
        virtual SlokedCrypto &GetCryptography() const = 0;
        virtual SlokedConfigurationLoader &GetConfigurationLoader() const = 0;
        virtual SlokedSocketFactory &GetNetwork() const = 0;
        virtual const SlokedDynamicLibraryLoader &GetDynamicLibraryLoader()
            const = 0;
        virtual bool HasGraphics() const = 0;
        virtual std::unique_ptr<SlokedGraphicalComponents> GetGraphics(
            SlokedScreenManager &) const = 0;
        virtual bool HasSystemTerminal() const = 0;
        virtual SlokedDuplexTerminal &GetSystemTerminal() const = 0;
        virtual UnbindResizeListener OnSystemTerminalResize(
            OnTerminalResize) const = 0;
        virtual bool HasScripting() const = 0;
        virtual std::unique_ptr<SlokedScriptEngine> NewScriptEngine(
            SlokedEditorInstanceContainer &, SlokedSharedEditorState &,
            const KgrValue &) const = 0;
    };
}  // namespace sloked

#endif