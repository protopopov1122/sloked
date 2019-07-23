#include "sloked/text/posix/TextFile.h"
#include "sloked/text/TextBlockHandle.h"
#include "sloked/core/NewLine.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <cassert>

namespace sloked {

    PosixTextView::PosixTextView(int fd)
        : fd(fd) {
        struct stat file_stats;
        fstat(fd, &file_stats);
        this->length = file_stats.st_size;
        this->data =  mmap(nullptr, this->length, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    }
    
    PosixTextView::~PosixTextView() {
        munmap(this->data, this->length);
        close(this->fd);
    }

    std::string_view PosixTextView::GetView() const {
        return std::string_view(static_cast<const char *>(this->data), this->length);
    }

    PosixTextView::operator std::string_view() const {
        return this->GetView();
    }
}