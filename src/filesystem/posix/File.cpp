#include "sloked/filesystem/posix/File.h"
#include "sloked/filesystem/posix/Reader.h"
#include "sloked/filesystem/posix/View.h"
#include "sloked/filesystem/posix/Writer.h"
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>

namespace sloked {

    SlokedPosixFile::SlokedPosixFile(const std::string &path)
        : path(path) {}

    const std::string &SlokedPosixFile::GetPath() const {
        return this->path;
    }

    uint64_t SlokedPosixFile::GetSize() const {
        struct stat stats;
        if (!stat(this->path.c_str(), &stats)) {
            return stats.st_size;
        } else {
            return 0;
        }
    }

    bool SlokedPosixFile::Exists() const {
        struct stat stats;
        return stat(this->path.c_str(), &stats) == 0;
    }

    SlokedPosixFile::Type SlokedPosixFile::GetType() const {
        struct stat stats;
        if (stat(this->path.c_str(), &stats)) {
            return Type::Unsupported;
        }
        if (S_ISREG(stats.st_mode)) {
            return Type::RegularFile;
        } else if (S_IFDIR) {
            return Type::Directory;
        } else {
            return Type::Unsupported;
        }
    }

    std::unique_ptr<SlokedFileReader> SlokedPosixFile::Reader() const {
        FILE *fp = fopen(this->path.c_str(), "r");
        if (fp) {
            return std::make_unique<SlokedPosixFileReader>(fp);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedFileWriter> SlokedPosixFile::Writer() const {
        FILE *fp = fopen(this->path.c_str(), "w");
        if (fp) {
            return std::make_unique<SlokedPosixFileWriter>(fp);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedFileView> SlokedPosixFile::View() const {
        FILE *fp = fopen(this->path.c_str(), "r");
        if (fp) {
            return std::make_unique<SlokedPosixFileView>(fp);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedFSObject> SlokedPosixFile::GetFile(std::string_view name) const {
        std::string path {this->path};
        path.push_back('/');
        path.append(name);
        return std::make_unique<SlokedPosixFile>(path);
    }

    void SlokedPosixFile::ListFiles(FileVisitor visitor) const {
        DIR *directory = opendir(this->path.c_str());
        if (directory == nullptr) {
            return;
        }
        for (dirent *entry = readdir(directory); entry != nullptr; entry = readdir(directory)) {
            visitor(entry->d_name);
        }
    }
}