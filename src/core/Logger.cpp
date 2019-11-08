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

#include "sloked/core/Logger.h"
#include <ctime>
#include <fstream>

namespace sloked {

    static std::chrono::system_clock Clock;
    SlokedLoggingManager SlokedLoggingManager::Global;

    class SlokedTextStreamSink : public SlokedLoggingSink {
     public:
        SlokedTextStreamSink(std::unique_ptr<std::ostream> output, SlokedLoggingSink::Formatter formatter)
            : output(std::move(output)), formatter(std::move(formatter)) {}

        void Log(uint8_t level, const std::string &tag, std::chrono::time_point<std::chrono::system_clock> timestamp, const std::string &content) override {
            *this->output << this->formatter(level, tag, timestamp, content) << std::endl;
            this->output->flush();
        }

     private:
        std::unique_ptr<std::ostream> output;
        SlokedLoggingSink::Formatter formatter;
    };

    class SlokedNullSink : public SlokedLoggingSink {
     public:
        void Log(uint8_t level, const std::string &tag, std::chrono::time_point<std::chrono::system_clock> timestamp, const std::string &content) override {}
    };

    std::unique_ptr<SlokedLoggingSink> SlokedLoggingSink::TextFile(const std::string &path, Formatter formatter) {
        return std::make_unique<SlokedTextStreamSink>(std::make_unique<std::ofstream>(path), std::move(formatter));
    }

    SlokedLoggingSink::Formatter SlokedLoggingSink::TabularFormat(uint8_t tagWidth, uint8_t dateWidth) {
        return [tagWidth, dateWidth](uint8_t level, const std::string &tag, std::chrono::time_point<std::chrono::system_clock> timestamp, const std::string &content) {
            std::time_t t = std::chrono::system_clock::to_time_t(timestamp);
            std::string time{std::ctime(&t)};
            time.erase(std::prev(time.end()));
            std::string result = std::to_string(level);
            constexpr uint8_t LevelWidth = 8;
            if (result.size() < LevelWidth) {
                result.append(std::string(LevelWidth - result.size(), ' '));
            } else {
                result.push_back('\t');
            }
            
            auto currentSize = result.size();
            result.append(tag);
            if (result.size() - currentSize < tagWidth) {
                result.append(std::string(tagWidth - (result.size() - currentSize), ' '));
            } else {
                result.push_back('\t');
            }

            currentSize = result.size();
            result.append(time);
            if (result.size() - currentSize < dateWidth) {
                result.append(std::string(tagWidth - (result.size() - currentSize), ' '));
            } else {
                result.push_back('\t');
            }
            result.append(content);
            return result;
        };
    }

    std::unique_ptr<SlokedLoggingSink> SlokedLoggingSink::Null() {
        return std::make_unique<SlokedNullSink>();
    }

    SlokedLogger::Entry::Entry(uint8_t level, const std::string &tag, SlokedLoggingSink &sink)
        : level(level), tag(tag), sink(sink), timestamp(Clock.now()) {}

    SlokedLogger::Entry::~Entry() {
        this->sink.Log(this->level, this->tag, this->timestamp, this->content.str());
    }

    SlokedLogger::SlokedLogger(const std::string &tag, SlokedLogging::Level defaultLevel, SlokedLogging &logging)
        : tag(tag), defaultLevel(defaultLevel), logging(logging) {}

    SlokedLogger::Entry SlokedLogger::To(SlokedLogging::Level level) const {
        return Entry(level, this->tag, *this->logging.Sink(level));
    }

    SlokedLoggingManager::SlokedLoggingManager(std::shared_ptr<SlokedLoggingSink> sink)
        : defaultSink(sink != nullptr ? std::move(sink) : SlokedLoggingSink::Null()) {}

    void SlokedLoggingManager::SetDefaultSink(std::shared_ptr<SlokedLoggingSink> sink) {
        this->defaultSink = sink;
    }

    void SlokedLoggingManager::SetSink(Level level, std::shared_ptr<SlokedLoggingSink> sink) {
        this->logLevels.emplace(level, sink);
    }

    std::shared_ptr<SlokedLoggingSink> SlokedLoggingManager::Sink(Level level) const {
        if (this->logLevels.count(level) != 0) {
            return this->logLevels.at(level);
        } else {
            return this->defaultSink;
        }
    }
}