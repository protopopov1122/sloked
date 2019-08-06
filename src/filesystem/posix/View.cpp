#include "sloked/filesystem/posix/View.h"
#include <sys/stat.h>
#include <sys/mman.h>

namespace sloked {

    SlokedPosixFileView::SlokedPosixFileView(FILE *file)
        : file(file) {
        struct stat file_stats;
        fstat(fileno(this->file), &file_stats);
        this->length = file_stats.st_size;
        this->data = mmap(nullptr, this->length, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fileno(this->file), 0);
    }
    
    SlokedPosixFileView::~SlokedPosixFileView() {
        munmap(this->data, this->length);
        fclose(this->file);
    }

    std::string_view SlokedPosixFileView::GetView() const {
        return std::string_view(static_cast<const char *>(this->data), this->length);
    }
}