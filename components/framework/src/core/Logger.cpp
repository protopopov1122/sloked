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

#include "sloked/core/Logger.h"
#include <ctime>
#include <mutex>
#include <fstream>

namespace sloked {

    static std::chrono::system_clock Clock;
    SlokedLoggingManager SlokedLoggingManager::Global;

    class SlokedTextStreamSink : public SlokedLoggingSink {
     public:
        SlokedTextStreamSink(const std::string &path, SlokedLoggingSink::Formatter formatter)
            : path(path), formatter(std::move(formatter)) {}

        void Log(const std::string &level, const std::string &tag, std::chrono::time_point<std::chrono::system_clock> timestamp, const std::string &content) override {
            std::unique_lock lock(this->mtx);
            if (!this->output.is_open()) {
                this->output.open(path, std::ios_base::out | std::ios_base::app);
            }
            this->output << this->formatter(level, tag, timestamp, content) << std::endl;
            this->output.flush();
        }

     private:
        std::mutex mtx;
        std::string path;
        std::ofstream output;
        SlokedLoggingSink::Formatter formatter;
    };

    class SlokedNullSink : public SlokedLoggingSink {
     public:
        void Log(const std::string &level, const std::string &tag, std::chrono::time_point<std::chrono::system_clock> timestamp, const std::string &content) override {}
    };

    std::unique_ptr<SlokedLoggingSink> SlokedLoggingSink::TextFile(const std::string &path, Formatter formatter) {
        return std::make_unique<SlokedTextStreamSink>(path, std::move(formatter));
    }

    static void PadString(std::string &base, const std::string &tail, uint8_t padWidth) {
        auto baseLen = base.size();
        base.append(tail);
        if (base.size() - baseLen < padWidth) {
            base.append(std::string(padWidth - (base.size() - baseLen), ' '));
        } else {
            base.push_back('\t');
        }
    }

    SlokedLoggingSink::Formatter SlokedLoggingSink::TabularFormat(uint8_t levelWidth, uint8_t tagWidth, uint8_t dateWidth) {
        return [levelWidth, tagWidth, dateWidth](const std::string &level, const std::string &tag, std::chrono::time_point<std::chrono::system_clock> timestamp, const std::string &content) {
            std::time_t t = std::chrono::system_clock::to_time_t(timestamp);
            std::string time{std::ctime(&t)};
            time.erase(std::prev(time.end()));

            std::string result = "";
            PadString(result, level, levelWidth);
            PadString(result, tag, tagWidth);
            PadString(result, time, dateWidth);
            result.append(content);
            
            return result;
        };
    }

    std::unique_ptr<SlokedLoggingSink> SlokedLoggingSink::Null() {
        return std::make_unique<SlokedNullSink>();
    }

    SlokedLogger::Entry::Entry(std::string level, const std::string &tag, SlokedLoggingSink &sink)
        : level(std::move(level)), tag(tag), sink(sink), timestamp(Clock.now()) {}

    SlokedLogger::Entry::~Entry() {
        this->sink.Log(this->level, this->tag, this->timestamp, this->content.str());
    }

    SlokedLogger::SlokedLogger(const std::string &tag, SlokedLogging::Level defaultLevel, SlokedLogging &logging)
        : tag(tag), defaultLevel(defaultLevel), logging(logging) {}

    SlokedLogger::Entry SlokedLogger::To(SlokedLogging::Level level) const {
        return Entry(this->logging.GetLevel(level), this->tag, this->logging.Sink(level));
    }
    
    SlokedLogger::Entry SlokedLogger::Debug() const {
        return this->To(SlokedLogLevel::Debug);
    }
    
    SlokedLogger::Entry SlokedLogger::Info() const {
        return this->To(SlokedLogLevel::Info);
    }
    
    SlokedLogger::Entry SlokedLogger::Warning() const {
        return this->To(SlokedLogLevel::Warning);
    }
    
    SlokedLogger::Entry SlokedLogger::Error() const {
        return this->To(SlokedLogLevel::Error);
    }
    
    SlokedLogger::Entry SlokedLogger::Critical() const {
        return this->To(SlokedLogLevel::Critical);
    }

    SlokedLoggingManager::SlokedLoggingManager(std::shared_ptr<SlokedLoggingSink> sink) {
        this->logLevels.Insert(std::numeric_limits<Level>::min(), std::numeric_limits<Level>::max(), sink != nullptr ? sink  : SlokedLoggingSink::Null());
        this->DefineLevel(SlokedLogLevel::Debug, "Debug");
        this->DefineLevel(SlokedLogLevel::Info, "Info");
        this->DefineLevel(SlokedLogLevel::Warning, "Warning");
        this->DefineLevel(SlokedLogLevel::Error, "Error");
        this->DefineLevel(SlokedLogLevel::Critical, "Critical");
    }

    void SlokedLoggingManager::DefineLevel(Level level, const std::string &descr) {
        this->levelNames[level] = descr;
    }

    void SlokedLoggingManager::SetSink(Level level, std::shared_ptr<SlokedLoggingSink> sink) {
        this->logLevels.Insert(level, std::numeric_limits<Level>::max(), sink);
    }

    void SlokedLoggingManager::SetSink(Level from, Level to, std::shared_ptr<SlokedLoggingSink> sink) {
        this->logLevels.Insert(from, to, sink);
    }

    std::string SlokedLoggingManager::GetLevel(Level level) const {
        if (this->levelNames.count(level) != 0) {
            return this->levelNames.at(level);
        }
        Level offsetLevel = level % 10;
        Level baseLevel = level - offsetLevel;
        if (this->levelNames.count(baseLevel) != 0) {
            return this->levelNames.at(baseLevel) + "+" + std::to_string(offsetLevel);
        } else {
            return "Custom+" + std::to_string(level);
        }
    }

    SlokedLoggingSink &SlokedLoggingManager::Sink(Level level) const {
        return *this->logLevels.At(level);
    }
}