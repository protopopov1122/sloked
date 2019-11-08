/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

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

#ifndef SLOKED_CORE_LOGGER_H_
#define SLOKED_CORE_LOGGER_H_

#include "sloked/Base.h"
#include <memory>
#include <sstream>
#include <chrono>
#include <functional>
#include <map>

#define SlokedLoggerTag __FILE__

namespace sloked {

    class SlokedLoggingSink {
     public:
        using Formatter = std::function<std::string(uint8_t, const std::string &, std::chrono::time_point<std::chrono::system_clock>, const std::string &)>;
        virtual ~SlokedLoggingSink() = default;
        virtual void Log(uint8_t, const std::string &, std::chrono::time_point<std::chrono::system_clock>, const std::string &) = 0;

        static std::unique_ptr<SlokedLoggingSink> TextFile(const std::string &, Formatter);
        static std::unique_ptr<SlokedLoggingSink> Null();
        static Formatter TabularFormat(uint8_t, uint8_t);
    };

    class SlokedLogging {
     public:
        using Level = uint8_t;
        virtual ~SlokedLogging() = default;
        virtual std::shared_ptr<SlokedLoggingSink> Sink(Level) const = 0;
    };

    class SlokedLoggingManager : public SlokedLogging {
     public:
        SlokedLoggingManager(std::shared_ptr<SlokedLoggingSink> = nullptr);
        void SetDefaultSink(std::shared_ptr<SlokedLoggingSink>);
        void SetSink(Level, std::shared_ptr<SlokedLoggingSink>);
        std::shared_ptr<SlokedLoggingSink> Sink(Level) const final;

        static SlokedLoggingManager Global;
        
     private:
        std::map<Level, std::shared_ptr<SlokedLoggingSink>> logLevels;
        std::shared_ptr<SlokedLoggingSink> defaultSink;
    };

    class SlokedLogger {
     public:
        class Entry {
         public:
            Entry(SlokedLogging::Level, const std::string &, SlokedLoggingSink &);
            Entry(const Entry &) = delete;
            Entry(Entry &&) = default;

            ~Entry();

            Entry &operator=(const Entry &) = delete;
            Entry &operator=(Entry &&) = default;

            template <typename T>
            Entry &operator<<(const T &value) {
                this->content << value;
                return *this;
            }

         private:
            SlokedLogging::Level level;
            const std::string &tag;
            SlokedLoggingSink &sink;
            std::chrono::time_point<std::chrono::system_clock> timestamp;
            std::stringstream content;
        };

        SlokedLogger(const std::string &, SlokedLogging::Level, SlokedLogging & = SlokedLoggingManager::Global);

        Entry To(SlokedLogging::Level) const;

        template <typename T>
        Entry operator<<(const T &value) const {
            Entry entry(this->defaultLevel, this->tag, *this->logging.Sink(this->defaultLevel));
            entry << value;
            return entry;
        }

     private:
        std::string tag;
        SlokedLogging::Level defaultLevel;
        SlokedLogging &logging;
    };

    class SlokedLogLevel {
     public:
        static constexpr SlokedLogging::Level Info = 0;
    };
}

#endif