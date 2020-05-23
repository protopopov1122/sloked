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

#include "sloked/core/Failure.h"


#include <array>
#include <csignal>
#include <cstdio>
#include <exception>
#include <cstdlib>

namespace sloked {
    static void (*PrintStackFn)() = nullptr;

    void SlokedTerminateHandler() {
        fprintf(stdout, "Program terminated ");
        if (std::current_exception()) {
            fprintf(stdout, "after throwing ");
            try {
                std::rethrow_exception(std::current_exception());
            } catch (const std::exception &ex) {
                fprintf(stdout, "\'%s\'. ", ex.what());
            } catch (...) { fprintf(stdout, "unknown exception. "); }
        } else {
            fprintf(stdout, "without throwing exception. ");
        }
        if (PrintStackFn) {
            PrintStackFn();
        }
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
        if (PrintStackFn) {
            PrintStackFn();
        }
        std::abort();
    }
}

#if __has_include(<execinfo.h>)
#include <execinfo.h>
#include <unistd.h>

namespace sloked {

    void PrintStack() {
        fprintf(stdout, "Stack trace:\n");
        std::array<void *, 256> trace;
        const int count = backtrace(trace.data(), trace.size());
        backtrace_symbols_fd(trace.data(), count, STDOUT_FILENO);
    }

    void SlokedFailure::SetupHandler() {
        PrintStackFn = PrintStack;
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
#elif defined(SLOKED_PLATFORM_OS_WINDOWS)
#include <array>
#include <windows.h>
#include <DbgHelp.h>

namespace sloked {

    void PrintStack() {
        fprintf(stdout, "Stack trace:\n");
        HANDLE process = GetCurrentProcess();
        std::array<PVOID, 256> addresses;
        const std::size_t framesCaptures = CaptureStackBackTrace(0, addresses.size(), addresses.data(), 0);
        constexpr std::size_t SymInfoMaxNameLen = 255;
        char symInfo[sizeof(SYMBOL_INFO) + SymInfoMaxNameLen + 1];
        SYMBOL_INFO *info = reinterpret_cast<SYMBOL_INFO *>(symInfo);
        for (std::size_t i = 0; i < framesCaptures; ++i)
        {
            info->MaxNameLen = SymInfoMaxNameLen;
            info->SizeOfStruct = sizeof(SYMBOL_INFO);
            SymFromAddr(process, (DWORD64)addresses[i], 0, info);
            fprintf(stdout, "\t%016llx\t%s\n", info->Address, info->Name);
        }
    }

    LONG windows_exception_handler(EXCEPTION_POINTERS *ExceptionInfo) {
        fprintf(stdout, "Program terminated after unhandled exception %lx at %p. ", ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo->ExceptionRecord->ExceptionAddress);
        PrintStack();
        std::abort();
        return EXCEPTION_EXECUTE_HANDLER;
    }

    void SlokedFailure::SetupHandler() {
        PrintStackFn = PrintStack;
        SymInitialize(GetCurrentProcess(), nullptr, TRUE);
        std::set_terminate(SlokedTerminateHandler);
        std::signal(SIGSEGV, SlokedSignalHandler);
        std::signal(SIGTERM, SlokedSignalHandler);
#ifdef SIGPIPE
        std::signal(SIGPIPE, SIG_IGN);
#endif
        SetUnhandledExceptionFilter(windows_exception_handler);
    }
}
#else
namespace sloked {

    void SlokedFailure::SetupHandler() {
        std::set_terminate(SlokedTerminateHandler);
        std::signal(SIGSEGV, SlokedSignalHandler);
        std::signal(SIGTERM, SlokedSignalHandler);
#ifdef SIGPIPE
        std::signal(SIGPIPE, SIG_IGN);
#endif
    }
}  // namespace sloked
#endif
