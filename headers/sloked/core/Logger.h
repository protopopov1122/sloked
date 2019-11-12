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

#include "sloked/core/RangeMap.h"
#include <memory>
#include <sstream>
#include <chrono>
#include <functional>

#define SlokedLoggerTag __FILE__

namespace sloked {

    class SlokedLoggingSink {
     public:
        using Formatter = std::function<std::string(const std::string &, const std::string &, std::chrono::time_point<std::chrono::system_clock>, const std::string &)>;
        virtual ~SlokedLoggingSink() = default;
        virtual void Log(const std::string &, const std::string &, std::chrono::time_point<std::chrono::system_clock>, const std::string &) = 0;

        static std::unique_ptr<SlokedLoggingSink> TextFile(const std::string &, Formatter);
        static std::unique_ptr<SlokedLoggingSink> Null();
        static Formatter TabularFormat(uint8_t, uint8_t, uint8_t);
    };

    class SlokedLogging {
     public:
        using Level = uint8_t;
        virtual ~SlokedLogging() = default;
        virtual std::string GetLevel(Level) const = 0;
        virtual SlokedLoggingSink &Sink(Level) const = 0;
    };

    class SlokedLoggingManager : public SlokedLogging {
     public:
        SlokedLoggingManager(std::shared_ptr<SlokedLoggingSink> = nullptr);
        void DefineLevel(Level, const std::string &);
        void SetSink(Level, std::shared_ptr<SlokedLoggingSink>);
        void SetSink(Level, Level, std::shared_ptr<SlokedLoggingSink>);
        std::string GetLevel(Level) const final;
        SlokedLoggingSink &Sink(Level) const final;

        static SlokedLoggingManager Global;
        
     private:
        RangeMap<Level, std::shared_ptr<SlokedLoggingSink>> logLevels;
        std::map<Level, std::string> levelNames;
    };

    class SlokedLogLevel {
     public:
        static constexpr SlokedLogging::Level Debug = 0;
        static constexpr SlokedLogging::Level Info = 10;
        static constexpr SlokedLogging::Level Warning = 20;
        static constexpr SlokedLogging::Level Error = 30;
        static constexpr SlokedLogging::Level Critical = 40;
    };

    class SlokedLogger {
     public:
        class Entry {
         public:
            Entry(std::string, const std::string &, SlokedLoggingSink &);
            Entry(const Entry &) = delete;
            Entry(Entry &&) = default;

            ~Entry();

            Entry &operator=(const Entry &) = delete;
            Entry &operator=(Entry &&) = delete;

            template <typename T>
            Entry &operator<<(const T &value) {
                this->content << value;
                return *this;
            }

         private:
            std::string level;
            const std::string &tag;
            SlokedLoggingSink &sink;
            std::chrono::time_point<std::chrono::system_clock> timestamp;
            std::stringstream content;
        };

        SlokedLogger(const std::string &, SlokedLogging::Level = SlokedLogLevel::Info, SlokedLogging & = SlokedLoggingManager::Global);

        Entry To(SlokedLogging::Level) const;
        Entry Debug() const;
        Entry Info() const;
        Entry Warning() const;
        Entry Error() const;
        Entry Critical() const;

        template <typename T>
        Entry operator<<(const T &value) const {
            Entry entry(this->logging.GetLevel(this->defaultLevel), this->tag, this->logging.Sink(this->defaultLevel));
            entry << value;
            return entry;
        }

     private:
        std::string tag;
        SlokedLogging::Level defaultLevel;
        SlokedLogging &logging;
    };
}

#endif