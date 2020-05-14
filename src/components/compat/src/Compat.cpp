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

#include "sloked/compat/compression/Compat.h"

#include "sloked/compat/Interface.h"
#include "sloked/compat/core/Compat.h"
#include "sloked/compat/core/awaitable/Compat.h"
#include "sloked/compat/crypto/Compat.h"
#include "sloked/compat/editor/configuration/Compat.h"
#include "sloked/compat/net/Compat.h"
#include "sloked/compat/screen/graphics/Compat.h"
#include "sloked/compat/screen/terminal/Compat.h"
#include "sloked/compat/screen/terminal/TerminalResize.h"

namespace sloked {

    class SlokedCompatBaseInterface : public SlokedBaseInterface {
        bool SupportsCompression() const final {
            return SlokedCompressionCompat::IsSupported();
        }

        SlokedCompression &GetCompression() const final {
            return SlokedCompressionCompat::GetCompression();
        }

        std::unique_ptr<SlokedIOPoll> NewAwaitablePoll() const final {
            return SlokedIOPollCompat::NewPoll();
        }

        bool HasCryptography() const final {
            return SlokedCryptoCompat::IsSupported();
        }

        SlokedCrypto &GetCryptography() const final {
            return SlokedCryptoCompat::GetCrypto();
        }

        SlokedConfigurationLoader &GetConfigurationLoader() const final {
            return SlokedConfigurationLoaderCompat::GetLoader();
        }

        SlokedSocketFactory &GetNetwork() const final {
            return SlokedNetCompat::GetNetwork();
        }

        const SlokedDynamicLibraryLoader &GetDynamicLibraryLoader()
            const final {
            return SlokedDynamicLibraryCompat::GetLoader();
        }

        bool HasGraphics() const final {
            return SlokedGraphicsCompat::HasGraphics();
        }

        std::unique_ptr<SlokedGraphicalComponents> GetGraphics(
            SlokedScreenManager &screenManager) const final {
            return SlokedGraphicsCompat::GetGraphics(screenManager);
        }

        bool HasSystemTerminal() const final {
            return SlokedTerminalCompat::HasSystemTerminal();
        }

        SlokedDuplexTerminal &GetSystemTerminal() const final {
            return SlokedTerminalCompat::GetSystemTerminal();
        }

        UnbindResizeListener OnSystemTerminalResize(
            OnTerminalResize listener) const final {
            return SlokedTerminalResizeListener::Bind(std::move(listener));
        }
    };

    const SlokedBaseInterface &SlokedEditorCompat::GetBaseInterface() {
        static const SlokedCompatBaseInterface BaseInterface;
        return BaseInterface;
    }
}  // namespace sloked