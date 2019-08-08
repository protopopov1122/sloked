#include "sloked/filesystem/posix/File.h"
#include "sloked/filesystem/posix/Reader.h"
#include "sloked/filesystem/posix/View.h"
#include "sloked/filesystem/posix/Writer.h"
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <libgen.h>
#include <ftw.h>
#include <sys/stat.h>

namespace sloked {

    SlokedPosixFile::SlokedPosixFile(const std::string &path)
        : path(path) {}

    bool SlokedPosixFile::IsFile() const {
        struct stat stats;
        return stat(this->path.c_str(), &stats) == 0 && S_ISREG(stats.st_mode);
    }

    bool SlokedPosixFile::IsDirectory() const {
        struct stat stats;
        return stat(this->path.c_str(), &stats) == 0 && S_ISDIR(stats.st_mode);
    }

    const std::string &SlokedPosixFile::GetPath() const {
        return this->path;
    }

    std::string SlokedPosixFile::GetName() const {
        std::unique_ptr<char[]> buffer(new char[this->path.size() + 1]);
        strcpy(buffer.get(), this->path.c_str());
        return std::string{basename(buffer.get())};
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

    std::string SlokedPosixFile::GetParent() const {
        std::unique_ptr<char[]> buffer(new char[this->path.size() + 1]);
        strcpy(buffer.get(), this->path.c_str());
        return std::string{dirname(buffer.get())};
    }


    static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
        int rv = remove(fpath);
        return rv;
    }

    void SlokedPosixFile::Delete() const {
        if (this->IsFile()) {
            remove(this->path.c_str());
        } else {
            nftw(this->path.c_str(), unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
        }
    }

    void SlokedPosixFile::Rename(const std::string &name) const {
        rename(this->path.c_str(), name.c_str());
    }

    void SlokedPosixFile::Mkdir() const {
        if (!this->Exists()) {
            mkdir(this->path.c_str(), 0755);
        }
    }

    std::unique_ptr<SlokedIOReader> SlokedPosixFile::Reader() const {
        FILE *fp = fopen(this->path.c_str(), "r");
        if (fp) {
            return std::make_unique<SlokedPosixFileReader>(fp);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedIOWriter> SlokedPosixFile::Writer() const {
        FILE *fp = fopen(this->path.c_str(), "w");
        if (fp) {
            return std::make_unique<SlokedPosixFileWriter>(fp);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedIOView> SlokedPosixFile::View() const {
        FILE *fp = fopen(this->path.c_str(), "r");
        if (fp) {
            return std::make_unique<SlokedPosixFileView>(fp);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedFile> SlokedPosixFile::GetFile(std::string_view name) const {
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
            std::string str(entry->d_name);
            if (str != ".." && str != ".") {
                visitor(str);
            }
        }
    }
}