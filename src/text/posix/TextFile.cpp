#include "sloked/text/posix/TextFile.h"
#include "sloked/text/TextBlockHandle.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <cassert>

namespace sloked {

    PosixTextFile::PosixTextFile(int fd)
        : fd(fd) {
        struct stat file_stats;
        fstat(fd, &file_stats);
        this->length = file_stats.st_size;
        this->data =  mmap(nullptr, this->length, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
        this->Scan();
    }

    PosixTextFile::~PosixTextFile() {
        this->content = nullptr;
        munmap(this->data, this->length);
        close(this->fd);
    }

    std::size_t PosixTextFile::GetLastLine() const {
        return this->content->GetLastLine();
    }

    std::size_t PosixTextFile::GetTotalLength() const {
        return this->content->GetTotalLength();
    }

    const std::string_view PosixTextFile::GetLine(std::size_t line) const {
        return this->content->GetLine(line);
    }
    
    bool PosixTextFile::Empty() const {
        return this->content->Empty();
    }
    
    void PosixTextFile::SetLine(std::size_t line, const std::string &content) {
        this->content->SetLine(line, content);
    }

    void PosixTextFile::EraseLine(std::size_t line) {
        this->content->EraseLine(line);
    }

    void PosixTextFile::InsertLine(std::size_t line, const std::string &content) {
        this->content->InsertLine(line, content);
    }

    void PosixTextFile::Optimize() {
        this->content->Optimize();
    }

    std::ostream &operator<<(std::ostream &os, const PosixTextFile &file) {
        return os << *file.content;
    }

    void PosixTextFile::Scan() {
        constexpr std::size_t MAX_CHUNK = 65536;
        this->content = nullptr;
        std::string_view str(static_cast<const char *>(this->data), this->length);
        std::size_t chunk_offset = 0;
        std::size_t last_line_offset = 0;
        std::size_t line = 0;
        std::map<std::size_t, std::pair<std::size_t, std::size_t>> chunk_lines;

        for (std::size_t i = 0; i < this->length; i++) {
            if (str[i] == '\n') {
                std::size_t length = i - chunk_offset;
                if (length < MAX_CHUNK) {
                    chunk_lines[chunk_lines.size()] = std::make_pair(last_line_offset, i - last_line_offset);
                    last_line_offset = i + 1;
                } else {
                    auto proxyBlock = std::make_unique<TextBlockHandle>(std::string_view {
                        static_cast<const char *>(this->data) + chunk_offset,
                        last_line_offset > chunk_offset
                            ? last_line_offset - chunk_offset - 1
                            : 0
                    }, std::move(chunk_lines));
                    auto region = std::make_unique<TextRegion>(std::move(proxyBlock));
                    if (this->content) {
                        this->content->AppendRegion(std::move(region));
                    } else {
                        this->content = std::move(region);
                    }
                    chunk_offset = last_line_offset;
                    last_line_offset = i + 1;
                    chunk_lines.clear();
                    chunk_lines[chunk_lines.size()] = std::make_pair(last_line_offset, i - last_line_offset);
                }
                line++;
            }
        }
        chunk_lines[chunk_lines.size()] = std::make_pair(last_line_offset, this->length - last_line_offset);
        
        if (chunk_offset != this->length) {
            auto proxyBlock = std::make_unique<TextBlockHandle>(std::string_view {
                static_cast<const char *>(this->data) + chunk_offset,
                this->length - chunk_offset
            }, std::move(chunk_lines));
            auto region = std::make_unique<TextRegion>(std::move(proxyBlock));
            if (this->content) {
                this->content->AppendRegion(std::move(region));
            } else {
                this->content = std::move(region);
            }
        }
    }
}