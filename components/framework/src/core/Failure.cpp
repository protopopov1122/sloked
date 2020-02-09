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

#include "sloked/core/Failure.h"

#if __has_include(<execinfo.h>)
#include <exception>
#include <csignal>
#include <cstdio>
#include <array>
#include <unistd.h>
#include <execinfo.h>

namespace sloked {
    void PrintStack() {
        fprintf(stdout, "Stack trace:\n");
        std::array<void *, 64> trace;
        const int count = backtrace(trace.data(), trace.size());
        backtrace_symbols_fd(trace.data(), count, STDOUT_FILENO);
    }

    void SlokedTerminateHandler() {
        fprintf(stdout, "Program terminated ");
        if (std::current_exception()) {
            fprintf(stdout, "after throwing ");
            try {
                std::rethrow_exception(std::current_exception());
            } catch (const std::exception &ex) {
                fprintf(stdout, "\'%s\'. ", ex.what());
            } catch (...) {
                fprintf(stdout, "unknown exception. ");
            }
        } else {
            fprintf(stdout, "without throwing exception. ");
        }
        PrintStack();
        std::abort();
    }

    void SlokedSignalHandler(int sig) {
        fprintf(stdout, "Program terminated after receiving ");
        switch (sig) {
            case SIGSEGV:
                fprintf(stdout, "SIGSEGV. ");
                break;

            case SIGTERM:
                fprintf(stdout, "SIGTERM. ");
                break;

            default:
                fprintf(stdout, "signal %i. ", sig);
                break;
        }
        PrintStack();
        std::abort();
    }

    void SlokedFailure::SetupHandler() {
        // Force libgcc preload (man execinfo.h)
        void *ptr;
        backtrace(&ptr, 0);
        // Setup handlers
        std::set_terminate(SlokedTerminateHandler);
        std::signal(SIGSEGV, SlokedSignalHandler);
        std::signal(SIGTERM, SlokedSignalHandler);
#ifdef SIGPIPE
        std::signal(SIGPIPE, SIG_IGN);
#endif
    }
}
#else
namespace sloked {

    void SlokedFailure::SetupHandler() {
#ifdef SIGPIPE
        std::signal(SIGPIPE, SIG_IGN);
#endif
    }
}
#endif
